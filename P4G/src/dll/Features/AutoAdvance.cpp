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
using SetCue_t = void (*)(void* player, void* acb, int cueId);
using Status_t = int (*)(void* player);
Draw_t o_Draw = nullptr;
SetCue_t o_SetCue = nullptr;

// Last cue set on any player, for matching a voice to the page that opened with it.
std::atomic<void*> g_lastCuePlayer{nullptr};
std::atomic<ULONGLONG> g_lastCueTick{0};

// Every box DrawDialog reports, tracked separately (several can be drawn each frame; idle ones
// report state 0).
struct Box {
    uint32_t* info = nullptr;
    ULONGLONG lastSeen = 0;
    int lastState = -1;
    ULONGLONG pageOpened = 0;  // when the current page started typing
    ULONGLONG shownAt = 0;     // when it reached "fully shown"
    ULONGLONG voiceEnded = 0;
    void* voice = nullptr;
    bool waiting = false;      // state 4 and no choice, as of the last DrawDialog
    bool system = false;
};
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
    bool choice = (flags & 0x38) >= 0x18;
    Box* b = Slot(info, now);
    if (!b) return;
    if (Trace::Enabled() && state != b->lastState)
        LOGI("dialogue: box %p flags 0x%08X state %d choice %d", (void*)info, flags, state, choice);
    if (b->lastState < 0 || state < b->lastState || (state < 4 && b->lastState == 4)) {
        b->pageOpened = now;  // first page, or the next page of the same box
        b->shownAt = 0;
        b->voice = nullptr;
        b->voiceEnded = 0;
        b->system = (flags & 0x10000) != 0;
    }
    if (state == 4 && b->lastState != 4) {
        b->shownAt = now;
        ULONGLONG cue = g_lastCueTick.load();
        if (cue + 150 >= b->pageOpened && cue <= now) b->voice = g_lastCuePlayer.load();
    }
    b->lastState = state;
    b->lastSeen = now;
    b->waiting = state == 4 && !choice;
}

void SetCue_Hook(void* player, void* acb, int cueId) {
    Hooks::Guard g;
    o_SetCue(player, acb, cueId);
    g_lastCuePlayer = player;
    g_lastCueTick = GetTickCount64();
}

bool UserHoldingInput() {
    for (int vk : {VK_SPACE, VK_RETURN, VK_ESCAPE, VK_BACK, VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT})
        if (GetAsyncKeyState(vk) & 0x8000) return true;
    return false;
}

}  // namespace

Options& Opts() { return g_o; }
const char* Status() { return g_status; }

void SaveOptions() {
    Settings::SetInt("autoadvance", "enabled", g_o.enabled);
    Settings::SetInt("autoadvance", "voiced", g_o.voicedDelayMs);
    Settings::SetInt("autoadvance", "unvoiced", g_o.unvoicedDelayMs);
    Settings::SetInt("autoadvance", "system", g_o.systemDelayMs);
    Settings::SetInt("autoadvance", "perchar", g_o.perCharMs);
}

bool InstallHooks() {
    g_o.enabled = Settings::GetInt("autoadvance", "enabled", 0) != 0;
    g_o.voicedDelayMs = Settings::GetInt("autoadvance", "voiced", 1500);
    g_o.unvoicedDelayMs = Settings::GetInt("autoadvance", "unvoiced", 2000);
    g_o.systemDelayMs = Settings::GetInt("autoadvance", "system", 2000);
    g_o.perCharMs = Settings::GetInt("autoadvance", "perchar", 0);
    if (!HostCheck::GroupOk("dialogue")) return false;
    bool ok = Hooks::Create("MsgWindow::DrawDialog", reinterpret_cast<void*>(Addr::DrawDialog), &Draw_Hook, &o_Draw);
    ok &= Hooks::Create("criAtomExPlayer_SetCueId", reinterpret_cast<void*>(Addr::CriSetCueId), &SetCue_Hook, &o_SetCue);
    return ok;
}

void Tick() {
    ULONGLONG now = GetTickCount64();
    Box* b = nullptr;
    for (auto& x : g_boxes)
        if (x.info && now - x.lastSeen < 250 && (!b || x.waiting)) b = &x;
    if (!b) {
        g_status = "No dialogue";
        return;
    }
    if (!g_o.enabled) {
        g_status = "Off";
        return;
    }
    if (!b->waiting) {
        g_status = "Text appearing";
        return;
    }
    if (Overlay::IsOpen()) {
        g_status = "Paused while the trainer is open";
        return;
    }
    if (b->voice) {
        int st = reinterpret_cast<Status_t>(Addr::CriGetStatus)(b->voice);
        if (st == 1 || st == 2) {  // preparing / playing
            g_status = "Voice playing";
            b->voiceEnded = 0;
            return;
        }
        if (!b->voiceEnded) b->voiceEnded = now;
        // A cue that was over almost as soon as the page opened is a sound effect (the advance
        // blip), not a voice line.
        if (b->voiceEnded - b->pageOpened < 600) b->voice = nullptr;
    }
    ULONGLONG from = b->voice ? b->voiceEnded : b->shownAt;
    int delay = b->voice ? g_o.voicedDelayMs : (b->system ? g_o.systemDelayMs : g_o.unvoicedDelayMs);
    if (now < from || now - from < ULONGLONG(delay)) {
        g_status = b->voice ? "Voice finished — waiting" : "Waiting";
        return;
    }
    if (UserHoldingInput()) {
        g_status = "Waiting for you to let go of the keys";
        return;
    }
    if (Trace::Enabled()) LOGI("dialogue: pressing Confirm (box %p, voice %p)", (void*)b->info, b->voice);
    Input::InjectConfirm();
    g_status = "Advanced";
    b->shownAt = now + 100000;  // no second press until the box reports a new page
    b->voiceEnded = 0;
    b->waiting = false;
}

}  // namespace AutoAdvance
