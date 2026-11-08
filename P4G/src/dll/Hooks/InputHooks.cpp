#include "Hooks/InputHooks.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <imgui.h>
#include <atomic>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>

#include "Hooks/Hook.h"
#include "UI/Overlay.h"
#include "Util/Log.h"
#include "Util/Paths.h"

namespace Input {
extern bool g_debug;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace {

// ---------------------------------------------------------------- DirectInput8
using GetDeviceState_t = HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8W*, DWORD, LPVOID);
using GetDeviceData_t = HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8W*, DWORD,
                                                     LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
GetDeviceState_t o_GetDeviceState = nullptr;
GetDeviceData_t o_GetDeviceData = nullptr;

std::mutex g_diMu;
uint8_t g_kbLatch[256] = {};                 // keys pressed while open, held since
std::map<void*, DIJOYSTATE2> g_joyRest;      // last joystick state before opening
std::map<void*, bool> g_isKeyboard;          // device type cache

bool IsKeyboard(IDirectInputDevice8W* dev) {
    std::lock_guard<std::mutex> lk(g_diMu);
    auto it = g_isKeyboard.find(dev);
    if (it != g_isKeyboard.end()) return it->second;
    DIDEVICEINSTANCEW inst{};
    inst.dwSize = sizeof inst;
    bool kb = SUCCEEDED(dev->GetDeviceInfo(&inst)) && GET_DIDEVICE_TYPE(inst.dwDevType) == DI8DEVTYPE_KEYBOARD;
    g_isKeyboard[dev] = kb;
    return kb;
}

HRESULT STDMETHODCALLTYPE GetDeviceState_Hook(IDirectInputDevice8W* self, DWORD cb, LPVOID data) {
    Hooks::Guard g;
    HRESULT hr = o_GetDeviceState(self, cb, data);
    if (FAILED(hr) || !data) return hr;
    bool open = Overlay::IsOpen();

    if (cb == 256) {  // keyboard
        auto* k = static_cast<uint8_t*>(data);
        std::lock_guard<std::mutex> lk(g_diMu);
        if (Input::g_debug) {
            static uint8_t prev[256];
            for (int i = 0; i < 256; ++i)
                if ((k[i] & 0x80) != (prev[i] & 0x80)) {
                    LOGI("input: dinput state dik 0x%02X %s (open=%d latch=%d) dev=%p", i,
                         (k[i] & 0x80) ? "down" : "up", open, g_kbLatch[i], (void*)self);
                    prev[i] = k[i];
                }
        }
        for (int i = 0; i < 256; ++i) {
            bool down = (k[i] & 0x80) != 0;
            if (open) {
                if (down) g_kbLatch[i] = 1;
                k[i] = 0;
            } else if (g_kbLatch[i]) {
                if (down) k[i] = 0;
                else g_kbLatch[i] = 0;
            }
        }
    } else if (cb == sizeof(DIMOUSESTATE) || cb == sizeof(DIMOUSESTATE2)) {
        if (open) std::memset(data, 0, cb);
    } else if (cb == sizeof(DIJOYSTATE) || cb == sizeof(DIJOYSTATE2)) {
        std::lock_guard<std::mutex> lk(g_diMu);
        auto* js = static_cast<DIJOYSTATE*>(data);
        if (!open) {
            DIJOYSTATE2 rest{};
            std::memcpy(&rest, data, cb);
            g_joyRest[self] = rest;
        } else {
            auto it = g_joyRest.find(self);
            if (it != g_joyRest.end()) std::memcpy(data, &it->second, cb);  // axes at rest
            std::memset(js->rgbButtons, 0, sizeof js->rgbButtons);
            for (auto& pov : js->rgdwPOV) pov = 0xFFFFFFFF;
            if (cb == sizeof(DIJOYSTATE2))
                std::memset(static_cast<DIJOYSTATE2*>(data)->rgbButtons, 0, 128);
        }
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE GetDeviceData_Hook(IDirectInputDevice8W* self, DWORD cbObj,
                                             LPDIDEVICEOBJECTDATA rg, LPDWORD inout, DWORD flags) {
    Hooks::Guard g;
    HRESULT hr = o_GetDeviceData(self, cbObj, rg, inout, flags);
    if (Input::g_debug && SUCCEEDED(hr) && inout && *inout && rg) {
        for (DWORD i = 0; i < *inout && i < 8; ++i) {
            auto* e = reinterpret_cast<DIDEVICEOBJECTDATA*>(reinterpret_cast<uint8_t*>(rg) + i * cbObj);
            LOGI("input: dinput data ofs 0x%lX data 0x%lX dev=%p", e->dwOfs, e->dwData, (void*)self);
        }
    }
    if (FAILED(hr) || !inout || !Overlay::IsOpen()) return hr;
    if (!rg || (flags & DIGDD_PEEK)) {
        if (!IsKeyboard(self)) *inout = 0;
        return hr;
    }
    // Keep only key releases for keyboards; drop everything else while open.
    DWORD kept = 0;
    if (IsKeyboard(self)) {
        auto* base = reinterpret_cast<uint8_t*>(rg);
        for (DWORD i = 0; i < *inout; ++i) {
            auto* e = reinterpret_cast<DIDEVICEOBJECTDATA*>(base + i * cbObj);
            if ((e->dwData & 0x80) == 0) {
                if (kept != i) std::memmove(base + kept * cbObj, e, cbObj);
                ++kept;
            }
        }
    }
    *inout = kept;
    return hr;
}

// ---------------------------------------------------------------- XInput
using XInputGetState_t = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
struct XMod {
    const wchar_t* name;
    XInputGetState_t o_get = nullptr;
    XInputGetState_t o_getEx = nullptr;  // ordinal 100 (includes the Guide button)
    bool hooked = false;
};
XMod g_xmods[] = {{L"xinput1_4.dll"}, {L"xinput1_3.dll"}, {L"xinput9_1_0.dll"}};
XInputGetState_t g_readPad = nullptr;  // unfiltered reader for our own use
std::mutex g_padMu;
WORD g_padLatch[4] = {};

void FilterPad(DWORD idx, XINPUT_STATE* st) {
    if (idx >= 4) return;
    std::lock_guard<std::mutex> lk(g_padMu);
    WORD b = st->Gamepad.wButtons;
    if (Overlay::IsOpen()) {
        g_padLatch[idx] |= b;
        std::memset(&st->Gamepad, 0, sizeof st->Gamepad);
    } else if (g_padLatch[idx]) {
        g_padLatch[idx] &= b;
        st->Gamepad.wButtons &= ~g_padLatch[idx];
    }
}

template <int N>
DWORD WINAPI XGet_Hook(DWORD idx, XINPUT_STATE* st) {
    Hooks::Guard g;
    DWORD r = g_xmods[N].o_get(idx, st);
    if (r == ERROR_SUCCESS && st) FilterPad(idx, st);
    return r;
}
template <int N>
DWORD WINAPI XGetEx_Hook(DWORD idx, XINPUT_STATE* st) {
    Hooks::Guard g;
    DWORD r = g_xmods[N].o_getEx(idx, st);
    if (r == ERROR_SUCCESS && st) FilterPad(idx, st);
    return r;
}

template <int N>
bool HookXMod(bool enableNow) {
    XMod& m = g_xmods[N];
    if (m.hooked) return true;
    HMODULE h = GetModuleHandleW(m.name);
    if (!h) return false;
    void* get = reinterpret_cast<void*>(GetProcAddress(h, "XInputGetState"));
    void* getEx = reinterpret_cast<void*>(GetProcAddress(h, MAKEINTRESOURCEA(100)));
    if (!get) return false;
    char nm[64];
    snprintf(nm, sizeof nm, "%ls!XInputGetState", m.name);
    if (!Hooks::Create(nm, get, &XGet_Hook<N>, &m.o_get)) return false;
    if (getEx && getEx != get) {
        snprintf(nm, sizeof nm, "%ls!XInputGetStateEx", m.name);
        Hooks::Create(nm, getEx, &XGetEx_Hook<N>, &m.o_getEx);
    }
    m.hooked = true;
    if (!g_readPad) g_readPad = m.o_get;
    if (enableNow) Hooks::EnableAll();
    return true;
}

void HookAllXMods(bool enableNow) {
    HookXMod<0>(enableNow);
    HookXMod<1>(enableNow);
    HookXMod<2>(enableNow);
}

// ---------------------------------------------------------------- cursor
using SetCursorPos_t = BOOL(WINAPI*)(int, int);
using ClipCursor_t = BOOL(WINAPI*)(const RECT*);
SetCursorPos_t o_SetCursorPos = nullptr;
ClipCursor_t o_ClipCursor = nullptr;
std::mutex g_clipMu;
bool g_gameClipSet = false;
RECT g_gameClip{};

BOOL WINAPI SetCursorPos_Hook(int x, int y) {
    Hooks::Guard g;
    if (Overlay::IsOpen()) return TRUE;  // don't let the game yank the cursor away
    return o_SetCursorPos(x, y);
}

BOOL WINAPI ClipCursor_Hook(const RECT* r) {
    Hooks::Guard g;
    {
        std::lock_guard<std::mutex> lk(g_clipMu);
        g_gameClipSet = r != nullptr;
        if (r) g_gameClip = *r;
    }
    if (Overlay::IsOpen()) return o_ClipCursor(nullptr);
    return o_ClipCursor(r);
}

// ---------------------------------------------------------------- window
HWND g_hwnd = nullptr;
WNDPROC g_origProc = nullptr;
bool g_unicode = true;
std::atomic<int> g_toggleVk{VK_INSERT};

struct Msg {
    HWND h;
    UINT m;
    WPARAM w;
    LPARAM l;
};
std::mutex g_qMu;
std::vector<Msg> g_queue;

bool IsInputMessage(UINT m) {
    return (m >= WM_KEYFIRST && m <= WM_KEYLAST) || (m >= WM_MOUSEFIRST && m <= WM_MOUSELAST) ||
           m == WM_INPUT || m == WM_MOUSELEAVE || m == WM_NCMOUSEMOVE;
}

bool ImGuiWantsMessage(UINT m) {
    return IsInputMessage(m) || m == WM_SETFOCUS || m == WM_KILLFOCUS || m == WM_ACTIVATEAPP ||
           m == WM_SETCURSOR || m == WM_DEVICECHANGE;
}

LRESULT CALLBACK WndProc_Hook(HWND h, UINT m, WPARAM w, LPARAM l) {
    Hooks::Guard g;
    bool repeat = (l & (1 << 30)) != 0;
    if (Input::g_debug && m != WM_MOUSEMOVE && m != WM_SETCURSOR && m != WM_NCHITTEST && m != WM_TIMER &&
        m != WM_PAINT && m != WM_INPUT && m != 0x0118 /*WM_SYSTIMER*/ && m != WM_NCMOUSEMOVE)
        LOGI("input: wndproc msg 0x%X vk 0x%llX open=%d", m, (unsigned long long)w, Overlay::IsOpen());
    if ((m == WM_KEYDOWN || m == WM_SYSKEYDOWN) && (int)w == g_toggleVk.load()) {
        if (!repeat) Overlay::Toggle();
        return 0;
    }
    if ((m == WM_KEYUP || m == WM_SYSKEYUP) && (int)w == g_toggleVk.load()) return 0;

    if (Overlay::IsOpen() && ImGuiWantsMessage(m)) {
        // A lone Alt (e.g. Alt+Tab, or a focus change) would toggle ImGui's menu layer and close
        // popups; the trainer has no use for Alt, so it never reaches ImGui.
        bool alt = (m == WM_SYSKEYDOWN || m == WM_SYSKEYUP || m == WM_KEYDOWN || m == WM_KEYUP) &&
                   (w == VK_MENU || w == VK_LMENU || w == VK_RMENU);
        if (alt) return 0;
        {
            std::lock_guard<std::mutex> lk(g_qMu);
            if (g_queue.size() < 4096) g_queue.push_back({h, m, w, l});
        }
        if (m == WM_SETCURSOR) return TRUE;  // ImGui draws its own cursor
        if (IsInputMessage(m)) return 0;      // withheld from the game
    }
    return g_unicode ? CallWindowProcW(g_origProc, h, m, w, l)
                     : CallWindowProcA(g_origProc, h, m, w, l);
}

}  // namespace

namespace Input {

bool Install() {
    bool ok = true;
    g_debug = GetFileAttributesW((Paths::DataDir() + L"\\debug-input").c_str()) != INVALID_FILE_ATTRIBUTES;
    if (g_debug) LOGI("input: verbose logging on");

    // DirectInput8: borrow the method addresses from a throwaway keyboard device.
    if (HMODULE di = GetModuleHandleW(L"dinput8.dll")) {
        using Create_t = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
        auto create = reinterpret_cast<Create_t>(GetProcAddress(di, "DirectInput8Create"));
        IDirectInput8W* dinput = nullptr;
        if (create && SUCCEEDED(create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION,
                                       IID_IDirectInput8W, reinterpret_cast<void**>(&dinput), nullptr))) {
            IDirectInputDevice8W* kb = nullptr;
            if (SUCCEEDED(dinput->CreateDevice(GUID_SysKeyboard, &kb, nullptr))) {
                void** vt = *reinterpret_cast<void***>(kb);
                ok &= Hooks::Create("IDirectInputDevice8::GetDeviceState", vt[9], &GetDeviceState_Hook,
                                    &o_GetDeviceState);
                ok &= Hooks::Create("IDirectInputDevice8::GetDeviceData", vt[10], &GetDeviceData_Hook,
                                    &o_GetDeviceData);
                kb->Release();
            }
            dinput->Release();
        } else {
            LOGW("DirectInput8 unavailable; keyboard blocking falls back to window messages only");
        }
    }

    // XInput. Load 1.4 ourselves so the hook is in place before the game polls a pad.
    LoadLibraryW(L"xinput1_4.dll");
    HookAllXMods(false);

    if (HMODULE u = GetModuleHandleW(L"user32.dll")) {
        Hooks::Create("SetCursorPos", reinterpret_cast<void*>(GetProcAddress(u, "SetCursorPos")),
                      &SetCursorPos_Hook, &o_SetCursorPos);
        Hooks::Create("ClipCursor", reinterpret_cast<void*>(GetProcAddress(u, "ClipCursor")),
                      &ClipCursor_Hook, &o_ClipCursor);
    }
    return ok;
}

void PollLateModules() { HookAllXMods(true); }

bool InstallWndProc(HWND hwnd) {
    if (g_hwnd) return true;
    g_hwnd = hwnd;
    g_unicode = IsWindowUnicode(hwnd) != FALSE;
    auto mine = reinterpret_cast<LONG_PTR>(&WndProc_Hook);
    g_origProc = reinterpret_cast<WNDPROC>(g_unicode ? SetWindowLongPtrW(hwnd, GWLP_WNDPROC, mine)
                                                     : SetWindowLongPtrA(hwnd, GWLP_WNDPROC, mine));
    LOGI("window subclassed (%s)", g_unicode ? "unicode" : "ansi");
    return g_origProc != nullptr;
}

bool RestoreWndProc() {
    if (!g_hwnd || !g_origProc) return true;
    auto cur = g_unicode ? GetWindowLongPtrW(g_hwnd, GWLP_WNDPROC)
                         : GetWindowLongPtrA(g_hwnd, GWLP_WNDPROC);
    if (cur != reinterpret_cast<LONG_PTR>(&WndProc_Hook)) {
        LOGE("another module subclassed the window after the trainer; unloading would crash it");
        return false;
    }
    auto orig = reinterpret_cast<LONG_PTR>(g_origProc);
    if (g_unicode) SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, orig);
    else SetWindowLongPtrA(g_hwnd, GWLP_WNDPROC, orig);
    g_hwnd = nullptr;
    return true;
}

bool ReadPad(XINPUT_STATE* out) {
    if (!g_readPad) return false;
    for (DWORD i = 0; i < 4; ++i)
        if (g_readPad(i, out) == ERROR_SUCCESS) return true;
    return false;
}

void DrainToImGui() {
    std::vector<Msg> q;
    {
        std::lock_guard<std::mutex> lk(g_qMu);
        q.swap(g_queue);
    }
    for (const Msg& m : q) {
        // Mouse buttons and position are polled every frame instead (Overlay::PollMouse): the game
        // pumps its message queue irregularly, which delayed clicks by seconds.
        if ((m.m >= WM_LBUTTONDOWN && m.m <= WM_MBUTTONDBLCLK) || m.m == WM_MOUSEMOVE ||
            (m.m >= WM_XBUTTONDOWN && m.m <= WM_XBUTTONDBLCLK))
            continue;
        if (g_debug && (m.m == WM_KEYDOWN || m.m == WM_KEYUP))
            LOGI("input: to imgui msg 0x%X vk 0x%llX", m.m, (unsigned long long)m.w);
        ImGui_ImplWin32_WndProcHandler(m.h, m.m, m.w, m.l);
    }
}

bool g_debug = false;

void OnOverlayOpened() {
    if (o_ClipCursor) o_ClipCursor(nullptr);
}

void OnOverlayClosed() {
    std::lock_guard<std::mutex> lk(g_clipMu);
    if (g_gameClipSet && o_ClipCursor) o_ClipCursor(&g_gameClip);
    std::lock_guard<std::mutex> lq(g_qMu);
    g_queue.clear();
}

void SetToggleKey(int vk) { g_toggleVk = vk; }
int ToggleKey() { return g_toggleVk.load(); }

}  // namespace Input
