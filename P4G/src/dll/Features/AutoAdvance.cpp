#include "Features/AutoAdvance.h"

#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstring>

#include "Engine/Addresses.h"
#include "Engine/HostCheck.h"
#include "Engine/SigScan.h"
#include "Hooks/Hook.h"
#include "Features/Trace.h"
#include "Hooks/InputHooks.h"
#include "UI/Overlay.h"
#include "Util/Log.h"
#include "Util/Settings.h"

namespace AutoAdvance {

namespace {

Options g_o;

using Draw_t = void (*)(uint32_t* info);
Draw_t o_Draw = nullptr;

// Voices. Event voices are started several ways (cue ids, wave ids, streamed files; Igor's in
// the Velvet Room through none of the player API's entry points the game is seen calling), so
// instead of hooking a start function the trainer watches every CRI player each frame. CRI
// moves a player onto its active list while it has a sound going, so a sound is "playing" while
// its player is on that list. A voice line starts with a page (from just before the page opens
// until it has been typing for a second), and it can run on across later pages of the same
// box, so each sound is marked a voice when it starts with a page and stays one until it ends.
// The game plays voices and sound effects on a small set of shared players (the page-advance
// sound starts with the page too), so the mark belongs to the sound, not the player. Music and
// ambient loops that happen to start with a page are recognised by running longer than any
// voice line; sound effects by being over almost at once.
constexpr ULONGLONG kVoiceCapMs = 30000;  // no voice line runs longer than this
constexpr ULONGLONG kSfxMs = 600;         // a sound shorter than this isn't a voice line
// A page's voice starts within this long of the page opening (seen: up to 0.6 s after a new
// box starts opening, which is a little after its text is fully shown). No page is advanced
// as unvoiced before then.
constexpr ULONGLONG kVoiceStartMs = 1500;

struct Player {
    void* p = nullptr;
    uint32_t cue = 0;       // CRI's current cue for it
    uint32_t played = 0;    // ms of that cue played
    bool playing = false;   // on CRI's active list
    ULONGLONG started = 0;  // when its current sound started
    ULONGLONG seen = 0;     // last frame it was on one of CRI's lists
    bool voice = false;     // its current sound started with a page
    bool music = false;     // played past kVoiceCapMs once: never a voice
};
Player g_players[32];
bool g_ready = false;  // the dialogue hooks and CRI's player lists were recognised

struct Raw {
    void* p;
    uint32_t status, cue, played;
    bool active;
};

// Appends one of CRI's player lists to `out`. The lists change under us (CRI's threads move
// players between them as sounds start and end), so every node is checked against the list it
// claims to be on, the walk is bounded, and a fault just ends it; a player missed for a frame
// costs nothing.
int WalkList(uintptr_t list, bool active, Raw* out, int n, int max) {
    __try {
        auto* node = *reinterpret_cast<uint8_t**>(list + 8);  // head
        for (int guard = 0; node && n < max && guard < 64; ++guard) {
            if (*reinterpret_cast<uintptr_t*>(node + 0x18) != list) break;
            out[n].p = node;
            out[n].status = *reinterpret_cast<uint32_t*>(node + 0x20);
            out[n].cue = *reinterpret_cast<uint32_t*>(node + 0x88);
            out[n].played = *reinterpret_cast<uint32_t*>(node + 0x1C8);
            out[n].active = active;
            ++n;
            node = *reinterpret_cast<uint8_t**>(node + 0x10);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return n;
}

void PollPlayers(ULONGLONG now) {
    Raw raw[64];
    // Active first: a player moving back to the idle list mid-read is then seen as idle last.
    int n = WalkList(Addr::CriActivePlayers, true, raw, 0, 64);
    n = WalkList(Addr::CriPlayerList, false, raw, n, 64);
    for (int i = 0; i < n; ++i) {
        const Raw& r = raw[i];
        Player* w = nullptr;
        for (auto& x : g_players)
            if (x.p == r.p) w = &x;
        if (!w) {
            w = &g_players[0];
            for (auto& x : g_players)
                if (!x.p || x.seen < w->seen) w = &x;  // free, else the longest gone
            *w = Player{r.p};
        }
        bool playing = r.active && (r.status == 1 || r.status == 2);  // preparing / playing
        // A new sound: the player became active, or (if the next one started before we saw
        // the last end) its cue changed or its play time went back.
        if (playing && (!w->playing || r.cue != w->cue || r.played + 100 < w->played)) {
            w->started = now;
            w->voice = false;
        }
        w->playing = playing;
        w->cue = r.cue;
        w->played = r.played;
        w->seen = now;
    }
}

// Marks the sounds that started as this page opened as voices.
void LearnVoices(ULONGLONG pageOpened, ULONGLONG now) {
    for (auto& w : g_players) {
        if (!w.p || !w.playing || w.voice || w.music || now - w.seen > 100) continue;
        if (w.started + 150 >= pageOpened && w.started <= pageOpened + kVoiceStartMs) {
            w.voice = true;
            if (Trace::Enabled()) LOGI("dialogue: player %p cue 0x%X started with the page", w.p, w.cue);
        }
    }
}

// Whether a voice is playing; `longEnough` once one has played past a sound effect's length.
bool VoicePlaying(ULONGLONG now, bool* longEnough) {
    bool playing = false;
    for (auto& w : g_players) {
        if (!w.p || !w.voice || w.music || !w.playing || now - w.seen > 100) continue;
        if (now - w.started > kVoiceCapMs) {
            w.music = true;
            if (Trace::Enabled()) LOGI("dialogue: player %p still playing after 30 s; not a voice", w.p);
            continue;
        }
        playing = true;
        if (now - w.started >= kSfxMs) *longEnough = true;
    }
    return playing;
}

// Every box DrawDialog reports, tracked separately (several can be drawn each frame; idle ones
// report state 0).
struct Box {
    uint32_t* info = nullptr;
    ULONGLONG lastSeen = 0;
    int lastState = -1;
    ULONGLONG pageOpened = 0;  // when the current page started typing
    ULONGLONG shownAt = 0;     // when it reached "fully shown"
    ULONGLONG voiceEnded = 0;  // when the voice heard on this page stopped
    bool heardVoice = false;   // a voice played on this page
    ULONGLONG pressedAt = 0;   // when Confirm was pressed for this page (0: not yet)
    bool waiting = false;      // a conversation page, fully shown, no choice, as of the last DrawDialog
    bool system = false;
    bool conversation = false; // the yellow conversation box (not a tutorial or system window)
    uint32_t lastFlags = 0;
};

// Bits 20..23 of the box flags: the conversation box (speech and narration) has only bit 21
// set. Tutorials and other system screens, whose text appears all at once, add bit 22; battle
// message boxes add bit 20 (and 22), or have bit 23 alone.
bool IsConversation(uint32_t flags) { return (flags & 0x00F00000) == 0x00200000; }
Box g_boxes[4];
const char* g_status = "No dialogue";

Box* Slot(uint32_t* info, ULONGLONG now) {
    Box* free = nullptr;
    for (auto& b : g_boxes) {
        if (b.info == info) return &b;
        if (!free && (!b.info || now - b.lastSeen > 500)) free = &b;
    }
    if (free) *free = Box{info};
    return free;
}

void Draw_Hook(uint32_t* info) {
    Hooks::Guard g;
    o_Draw(info);
    if (!info) return;
    ULONGLONG now = GetTickCount64();
    uint32_t flags = info[0];
    int state = flags & 7;
    if (state == 0) return;  // idle box
    // Bits 3..5: a choice list. 0x10 while it opens (for a few frames after Confirm on the
    // question page, still in state 4), 0x18/0x20 while it is up, 0x28 once one is picked.
    bool choice = (flags & 0x38) != 0;
    Box* b = Slot(info, now);
    if (!b) return;
    if (Trace::Enabled() && (state != b->lastState || flags != b->lastFlags))
        LOGI("dialogue: box %p flags 0x%08X state %d choice %d", (void*)info, flags, state, choice);
    b->lastFlags = flags;
    if (b->lastState < 0 || state < b->lastState || (state < 4 && b->lastState == 4)) {
        b->pageOpened = now;  // first page, or the next page of the same box
        b->shownAt = 0;
        b->heardVoice = false;
        b->voiceEnded = 0;
        b->pressedAt = 0;
        b->system = (flags & 0x10000) != 0;
    }
    if (state == 4 && b->lastState != 4) b->shownAt = now;
    b->lastState = state;
    b->lastSeen = now;
    b->conversation = IsConversation(flags);
    b->waiting = state == 4 && !choice && b->conversation;
}

bool KeysHeld() {
    for (int vk : {VK_SPACE, VK_RETURN, VK_ESCAPE, VK_BACK, VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT})
        if (GetAsyncKeyState(vk) & 0x8000) return true;
    return false;
}

bool UserHoldingInput() {
    return KeysHeld() || Input::GamePadHeld() != 0;  // any button, on any controller the game reads
}

// Diagnostics: what autoplay was doing before the trainer opened, with details, so a stall can
// be read off the Field & Dungeon page; and a log line when a page sits for a while.
const char* g_statusLive = "No dialogue";  // the last status while the trainer was closed
char g_detail[200] = "";
const void* g_stallLoggedBox = nullptr;
ULONGLONG g_stallLoggedPage = 0;
constexpr ULONGLONG kStallLogMs = 12000;

void Describe(const Box& b, ULONGLONG now) {
    char voice[96] = "none";
    for (auto& w : g_players)
        if (w.p && w.voice && w.playing && now - w.seen <= 100) {
            snprintf(voice, sizeof voice, "player %p cue 0x%X for %.1f s%s", w.p, w.cue, (now - w.started) / 1000.0,
                     w.music ? " (music)" : "");
            break;
        }
    snprintf(g_detail, sizeof g_detail, "box flags 0x%08X, page up %.1f s, voice: %s, held: pad 0x%X%s",
             b.lastFlags, (now - b.pageOpened) / 1000.0, voice, Input::GamePadHeld(), KeysHeld() ? " + keys" : "");
}

// The on/off key. Keys the game reads during conversations (Up/Down for choices, Confirm,
// Cancel) and the trainer's own open/close key are refused by the picker.
const KeyChoice kKeyChoices[] = {
    {0, "None"},
    {0x1E, "A"}, {0x30, "B"}, {0x2E, "C"}, {0x20, "D"}, {0x12, "E"}, {0x21, "F"}, {0x22, "G"},
    {0x23, "H"}, {0x17, "I"}, {0x24, "J"}, {0x25, "K"}, {0x26, "L"}, {0x32, "M"}, {0x31, "N"},
    {0x18, "O"}, {0x19, "P"}, {0x10, "Q"}, {0x13, "R"}, {0x1F, "S"}, {0x14, "T"}, {0x16, "U"},
    {0x2F, "V"}, {0x11, "W"}, {0x2D, "X"}, {0x15, "Y"}, {0x2C, "Z"},
    {0x02, "1"}, {0x03, "2"}, {0x04, "3"}, {0x05, "4"}, {0x06, "5"}, {0x07, "6"}, {0x08, "7"},
    {0x09, "8"}, {0x0A, "9"}, {0x0B, "0"}, {0x0F, "Tab"}, {0x3A, "Caps Lock"},
    {0x3B, "F1"}, {0x3C, "F2"}, {0x3D, "F3"}, {0x3E, "F4"}, {0x3F, "F5"}, {0x40, "F6"},
    {0x41, "F7"}, {0x42, "F8"}, {0x43, "F9"}, {0x44, "F10"}, {0x57, "F11"}, {0x58, "F12"},
    {0xD2, "Insert"}, {0xD3, "Delete"}, {0xC7, "Home"}, {0xCF, "End"}, {0xC9, "Page Up"},
    {0xD1, "Page Down"},
};

// The on/off controller button, as a bit of the game's pad read (PS Vita layout; the game
// maps an Xbox pad's A/B/X/Y to Cross/Circle/Square/Triangle). Buttons the game uses in
// conversations (Confirm, Cancel, D-pad Up/Down for choices) are refused by the picker, as is
// any button of the trainer's own open/close combo.
const KeyChoice kPadChoices[] = {
    {0, "None"},
    {0x8000, "X / Square"},     {0x1000, "Y / Triangle"},  {0x4000, "A / Cross"},   {0x2000, "B / Circle"},
    {0x400, "LB / L1"},         {0x800, "RB / R1"},        {0x100, "LT / L2"},      {0x200, "RT / R2"},
    {0x2, "L3"},                {0x4, "R3"},               {0x1, "Back / Select"},  {0x8, "Start"},
    {0x80, "D-pad Left"},       {0x20, "D-pad Right"},     {0x10, "D-pad Up"},      {0x40, "D-pad Down"},
};

ULONGLONG g_indicatorShown = 0;  // when the badge last had a reason to show
float g_indicatorAlpha = 0;
ULONGLONG g_indicatorFrame = 0;

}  // namespace

Options& Opts() { return g_o; }
const char* Status() { return g_status; }

const KeyChoice* KeyChoices(int* count) {
    *count = int(sizeof kKeyChoices / sizeof kKeyChoices[0]);
    return kKeyChoices;
}

const char* KeyName(int dik) {
    for (const auto& k : kKeyChoices)
        if (k.dik == dik) return k.name;
    return "None";
}

void SetKey(int dik) {
    g_o.keyDik = dik;
    Input::SetAutoAdvanceKey(dik);
    SaveOptions();
}

const KeyChoice* PadChoices(int* count) {
    *count = int(sizeof kPadChoices / sizeof kPadChoices[0]);
    return kPadChoices;
}

const char* PadName(int button) {
    for (const auto& k : kPadChoices)
        if (k.dik == button) return k.name;
    return "None";
}

void SetPad(int button) {
    g_o.padButton = button;
    Input::SetAutoAdvancePad(uint32_t(button));
    SaveOptions();
}

void SaveOptions() {
    Settings::SetInt("autoadvance", "enabled", g_o.enabled);
    Settings::SetInt("autoadvance", "voiced", g_o.voicedDelayMs);
    Settings::SetInt("autoadvance", "unvoiced", g_o.unvoicedDelayMs);
    Settings::SetInt("autoadvance", "system", g_o.systemDelayMs);
    Settings::SetInt("autoadvance", "perchar", g_o.perCharMs);
    Settings::SetInt("autoadvance", "key", g_o.keyDik);
    Settings::SetInt("autoadvance", "pad", g_o.padButton);
}

bool InstallHooks() {
    g_o.enabled = Settings::GetInt("autoadvance", "enabled", 0) != 0;
    g_o.voicedDelayMs = Settings::GetInt("autoadvance", "voiced", 1500);
    g_o.unvoicedDelayMs = Settings::GetInt("autoadvance", "unvoiced", 2800);
    g_o.systemDelayMs = Settings::GetInt("autoadvance", "system", 2800);
    g_o.perCharMs = Settings::GetInt("autoadvance", "perchar", 0);
    int key = Settings::GetInt("autoadvance", "key", 0x1E);
    g_o.keyDik = 0;
    for (const auto& k : kKeyChoices)
        if (k.dik == key) g_o.keyDik = key;
    int pad = Settings::GetInt("autoadvance", "pad", 0x8000);
    g_o.padButton = 0;
    for (const auto& k : kPadChoices)
        if (k.dik == pad) g_o.padButton = pad;
    // 1.0.1 raised the unvoiced defaults from 2 s to 2.8 s; settings saved by 1.0.0 still hold
    // the old default, so move those along once.
    if (Settings::GetInt("autoadvance", "rev", 1) < 2) {
        if (g_o.unvoicedDelayMs == 2000) g_o.unvoicedDelayMs = 2800;
        if (g_o.systemDelayMs == 2000) g_o.systemDelayMs = 2800;
        Settings::SetInt("autoadvance", "rev", 2);
        SaveOptions();
    }
    Input::SetAutoAdvanceKey(g_o.keyDik);
    Input::SetAutoAdvancePad(uint32_t(g_o.padButton));
    if (!HostCheck::GroupOk("dialogue")) return false;
    g_ready = Hooks::Create("MsgWindow::DrawDialog", reinterpret_cast<void*>(Addr::DrawDialog), &Draw_Hook, &o_Draw);
    return g_ready;
}

float IndicatorAlpha() { return g_indicatorAlpha; }

namespace {

// Turning auto-advance on (or off) partway through a page: the page gets its full wait again.
void RestartWaits(ULONGLONG now) {
    for (auto& x : g_boxes) {
        if (x.waiting) x.shownAt = now;
        if (x.voiceEnded) x.voiceEnded = now;
        x.pressedAt = 0;
    }
}

void UpdateIndicator(bool boxShown, bool otherShown, ULONGLONG now) {
    if (boxShown && g_o.enabled && !Overlay::IsOpen()) g_indicatorShown = now;
    if (otherShown && !boxShown) g_indicatorShown = 0;  // a tutorial or system screen: go now
    // Fade in over 0.2 s; fade out over 0.25 s once the box has gone (boxes close and reopen
    // between speakers, so it lingers a moment rather than flickering).
    float dt = g_indicatorFrame ? float(now - g_indicatorFrame) / 1000.0f : 0;
    g_indicatorFrame = now;
    bool want = g_indicatorShown && now - g_indicatorShown < 400;
    if (!g_o.enabled || Overlay::IsOpen()) want = false;
    g_indicatorAlpha += want ? dt / 0.2f : -dt / 0.25f;
    g_indicatorAlpha = g_indicatorAlpha < 0 ? 0 : (g_indicatorAlpha > 1 ? 1 : g_indicatorAlpha);
}

void TickInner();

}  // namespace

void Tick() {
    ULONGLONG now = GetTickCount64();
    if (g_ready) PollPlayers(now);  // every frame, box or not, so a voice is known before it's needed
    bool boxShown = false, otherShown = false;
    for (auto& x : g_boxes)
        if (x.info && now - x.lastSeen < 150) (x.conversation ? boxShown : otherShown) = true;
    Input::SetAutoAdvanceKeyLive(g_ready && boxShown && !Overlay::IsOpen());
    if (int presses = Input::TakeAutoAdvanceKeyPresses(); presses & 1) {
        g_o.enabled = !g_o.enabled;
        SaveOptions();
        RestartWaits(now);
        LOGI("dialogue: auto-advance turned %s (key %s / button %s); it was: %s; %s", g_o.enabled ? "on" : "off",
             KeyName(g_o.keyDik), PadName(g_o.padButton), g_statusLive, g_detail);
    }
    UpdateIndicator(boxShown, otherShown, now);
    const char* before = g_status;
    TickInner();
    if (g_status != before && Trace::Enabled()) LOGI("dialogue: status %s", g_status);
    if (!Overlay::IsOpen()) g_statusLive = g_status;

    // A conversation page autoplay has sat on for a while (not a choice being made, not the
    // trainer being open): log once per page why, so a stall can be diagnosed from the log.
    for (auto& x : g_boxes) {
        bool choiceUp = (x.lastFlags & 0x38) >= 0x18;
        if (!x.info || !x.conversation || choiceUp || now - x.lastSeen > 250 || !g_o.enabled || Overlay::IsOpen())
            continue;
        if (now - x.pageOpened > kStallLogMs && !(g_stallLoggedBox == x.info && g_stallLoggedPage == x.pageOpened)) {
            g_stallLoggedBox = x.info;
            g_stallLoggedPage = x.pageOpened;
            LOGI("dialogue: autoplay hasn't moved on from this page in %llu s: %s; %s", kStallLogMs / 1000,
                 g_status, g_detail);
        }
    }
}

const char* StatusBeforeTrainer() { return g_statusLive; }
const char* StatusDetail() { return g_detail; }

namespace {

void TickInner() {
    ULONGLONG now = GetTickCount64();
    Box* b = nullptr;
    for (auto& x : g_boxes)
        if (x.info && now - x.lastSeen < 250 && (!b || x.waiting)) b = &x;
    if (!b) {
        g_status = "No dialogue";
        return;
    }
    LearnVoices(b->pageOpened, now);
    bool longEnough = false;
    bool playing = VoicePlaying(now, &longEnough);
    if (playing) {
        if (longEnough) b->heardVoice = true;
        b->voiceEnded = 0;
    } else if (b->heardVoice && !b->voiceEnded) {
        b->voiceEnded = now;
    }
    if (!Overlay::IsOpen()) Describe(*b, now);
    if (!g_o.enabled) {
        g_status = "Off";
        return;
    }
    if (Overlay::IsOpen()) {
        // Paused, and the wait starts over once the trainer closes, so the page still gets its
        // full delay after you've been looking at the trainer.
        // (Opening the trainer also drops a press that hadn't reached the game yet.)
        g_status = "Paused while the trainer is open";
        if (b->waiting) b->shownAt = now;
        if (b->voiceEnded) b->voiceEnded = now;
        b->pressedAt = 0;
        return;
    }
    if (!b->conversation) {
        g_status = "Tutorial or system screen (left to you)";
        return;
    }
    if (!b->waiting) {
        g_status = "Text appearing";
        return;
    }
    if (b->pressedAt) {
        // Pressed; the box reports a new page within a frame or two. If it hasn't after a few
        // seconds the press was lost, so allow another.
        if (now - b->pressedAt < 3000) {
            g_status = "Advanced";
            return;
        }
        b->pressedAt = 0;
    }
    if (playing) {
        g_status = "Voice playing";
        return;
    }
    ULONGLONG from = b->heardVoice ? b->voiceEnded : b->shownAt;
    int delay = b->heardVoice ? g_o.voicedDelayMs : (b->system ? g_o.systemDelayMs : g_o.unvoicedDelayMs);
    bool voiceMayStart = !b->heardVoice && now - b->pageOpened < kVoiceStartMs;
    if (now < from || now - from < ULONGLONG(delay) || voiceMayStart) {
        g_status = b->heardVoice ? "Voice finished — waiting" : "Waiting";
        return;
    }
    if (UserHoldingInput()) {
        g_status = "Waiting for you to let go of the keys";
        return;
    }
    if (Trace::Enabled()) LOGI("dialogue: pressing Confirm (box %p, voiced %d)", (void*)b->info, b->heardVoice);
    Input::InjectConfirm();
    g_status = "Advanced";
    b->pressedAt = now;
}

}  // namespace

}  // namespace AutoAdvance
