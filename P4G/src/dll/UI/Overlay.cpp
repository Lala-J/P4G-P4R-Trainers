#include "UI/Overlay.h"

#include <d3d11.h>
#include <dxgi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <atomic>

#include "Engine/GameThread.h"
#include "Features/BattleOverrides.h"
#include "Features/Trace.h"
#include "Hooks/InputHooks.h"
#include "UI/Shell.h"
#include "UI/Theme.h"
#include "Util/Log.h"
#include "Util/Settings.h"

namespace {

std::atomic<bool> g_open{false};
std::atomic<bool> g_unloading{false};
std::atomic<ULONGLONG> g_lastToggle{0};

bool g_ready = false;
bool g_initFailed = false;
bool g_wasOpen = false;
bool g_rebuild = false;
ID3D11Device* g_dev = nullptr;
ID3D11DeviceContext* g_ctx = nullptr;
ID3D11RenderTargetView* g_rtv = nullptr;
HWND g_hwnd = nullptr;
unsigned g_bbHeight = 1080;
float g_userScale = 1.0f;
WORD g_prevButtons = 0;
unsigned g_frame = 0;

float BaseScale() {
    float s = g_bbHeight / 1080.0f;
    return s < 0.6f ? 0.6f : (s > 3.0f ? 3.0f : s);
}

void CreateRtv(IDXGISwapChain* sc) {
    ID3D11Texture2D* bb = nullptr;
    if (SUCCEEDED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&bb)))) {
        g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv);
        bb->Release();
    }
}

bool Init(IDXGISwapChain* sc) {
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_dev)))) {
        LOGE("overlay: swap chain has no D3D11 device");
        g_initFailed = true;
        return false;
    }
    g_dev->GetImmediateContext(&g_ctx);
    DXGI_SWAP_CHAIN_DESC d{};
    sc->GetDesc(&d);
    g_hwnd = d.OutputWindow;
    if (d.BufferDesc.Height) g_bbHeight = d.BufferDesc.Height;

    g_userScale = Settings::GetFloat("ui", "scale", 1.0f);
    if (g_userScale < 0.6f || g_userScale > 2.0f) g_userScale = 1.0f;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    Theme::LoadFonts(Overlay::Scale());
    Theme::Apply(Overlay::Scale());
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);
    Input::InstallWndProc(g_hwnd);

    g_ready = true;
    DWORD winTid = GetWindowThreadProcessId(g_hwnd, nullptr);
    LOGI("threads: present %lu, window owner %lu (%s)", GetCurrentThreadId(), winTid,
         winTid == GetCurrentThreadId() ? "same thread" : "different threads");
    LOGI("overlay ready: backbuffer %ux%u, UI scale %.2f", d.BufferDesc.Width, d.BufferDesc.Height,
         Overlay::Scale());
    return true;
}

#define SATURATE(v) ((v) < 0.0f ? 0.0f : (v) > 1.0f ? 1.0f : (v))

void FeedPad(const XINPUT_GAMEPAD& gp) {
    ImGuiIO& io = ImGui::GetIO();
    auto btn = [&](ImGuiKey k, WORD b) { io.AddKeyEvent(k, (gp.wButtons & b) != 0); };
    auto ana = [&](ImGuiKey k, float v, float v0, float v1) {
        float n = (v - v0) / (v1 - v0);
        io.AddKeyAnalogEvent(k, n > 0.10f, SATURATE(n));
    };
    btn(ImGuiKey_GamepadStart, XINPUT_GAMEPAD_START);
    btn(ImGuiKey_GamepadBack, XINPUT_GAMEPAD_BACK);
    btn(ImGuiKey_GamepadFaceLeft, XINPUT_GAMEPAD_X);
    btn(ImGuiKey_GamepadFaceRight, XINPUT_GAMEPAD_B);
    btn(ImGuiKey_GamepadFaceUp, XINPUT_GAMEPAD_Y);
    btn(ImGuiKey_GamepadFaceDown, XINPUT_GAMEPAD_A);
    btn(ImGuiKey_GamepadDpadLeft, XINPUT_GAMEPAD_DPAD_LEFT);
    btn(ImGuiKey_GamepadDpadRight, XINPUT_GAMEPAD_DPAD_RIGHT);
    btn(ImGuiKey_GamepadDpadUp, XINPUT_GAMEPAD_DPAD_UP);
    btn(ImGuiKey_GamepadDpadDown, XINPUT_GAMEPAD_DPAD_DOWN);
    btn(ImGuiKey_GamepadL1, XINPUT_GAMEPAD_LEFT_SHOULDER);
    btn(ImGuiKey_GamepadR1, XINPUT_GAMEPAD_RIGHT_SHOULDER);
    btn(ImGuiKey_GamepadL3, XINPUT_GAMEPAD_LEFT_THUMB);
    btn(ImGuiKey_GamepadR3, XINPUT_GAMEPAD_RIGHT_THUMB);
    ana(ImGuiKey_GamepadL2, gp.bLeftTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD, 255);
    ana(ImGuiKey_GamepadR2, gp.bRightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD, 255);
    const float dzL = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, dzR = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    ana(ImGuiKey_GamepadLStickLeft, gp.sThumbLX, -dzL, -32768);
    ana(ImGuiKey_GamepadLStickRight, gp.sThumbLX, +dzL, +32767);
    ana(ImGuiKey_GamepadLStickUp, gp.sThumbLY, +dzL, +32767);
    ana(ImGuiKey_GamepadLStickDown, gp.sThumbLY, -dzL, -32768);
    ana(ImGuiKey_GamepadRStickLeft, gp.sThumbRX, -dzR, -32768);
    ana(ImGuiKey_GamepadRStickRight, gp.sThumbRX, +dzR, +32767);
    ana(ImGuiKey_GamepadRStickUp, gp.sThumbRY, +dzR, +32767);
    ana(ImGuiKey_GamepadRStickDown, gp.sThumbRY, -dzR, -32768);
}

void Rebuild() {
    Theme::LoadFonts(Overlay::Scale());
    Theme::Apply(Overlay::Scale());
    ImGui_ImplDX11_InvalidateDeviceObjects();  // font atlas is re-uploaded by the next NewFrame
    g_rebuild = false;
}

}  // namespace

namespace Overlay {

bool IsOpen() { return g_open.load(std::memory_order_acquire); }

void SetOpen(bool open) {
    if (g_unloading && open) return;
    g_open.store(open, std::memory_order_release);
}

void Toggle() {
    // Debounced: the same press can arrive through more than one input path.
    ULONGLONG now = GetTickCount64();
    if (now - g_lastToggle.load() < 250) return;
    g_lastToggle = now;
    SetOpen(!IsOpen());
}

void OnPresent(IDXGISwapChain* sc) {
    if (g_unloading.load()) return;
    // Queued game changes apply here, at the top of the frame, on the game's main thread.
    GameThread::BindToCurrent();
    GameThread::Drain();
    BattleFx::Tick();
    Trace::Tick();
    if (!g_ready && (g_initFailed || !Init(sc))) return;

    if ((++g_frame % 120) == 0) Input::PollLateModules();

    XINPUT_STATE ps{};
    bool pad = Input::ReadPad(&ps);
    WORD b = pad ? ps.Gamepad.wButtons : 0;
    auto combo = [](WORD x) {
        return (x & XINPUT_GAMEPAD_BACK) && (x & XINPUT_GAMEPAD_RIGHT_THUMB);
    };
    if (combo(b) && !combo(g_prevButtons)) Toggle();
    g_prevButtons = b;

    bool open = IsOpen();
    if (open != g_wasOpen) {
        if (open) {
            Input::OnOverlayOpened();
            Shell::NotifyOpened();
        } else {
            Input::OnOverlayClosed();
        }
        ImGui::GetIO().MouseDrawCursor = open;
        g_wasOpen = open;
    }
    if (!open && !Shell::WantsHud()) return;

    if (g_rebuild) Rebuild();
    if (!g_rtv) CreateRtv(sc);
    if (!g_rtv) return;

    Input::DrainToImGui();
    if (open) FeedPad(ps.Gamepad);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    Shell::Draw(open);
    ImGui::Render();

    ID3D11RenderTargetView* prevRtv[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11DepthStencilView* prevDsv = nullptr;
    g_ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, prevRtv, &prevDsv);
    g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, prevRtv, prevDsv);
    for (auto* r : prevRtv)
        if (r) r->Release();
    if (prevDsv) prevDsv->Release();
}

void OnResizeBegin() {
    if (g_rtv) {
        g_rtv->Release();
        g_rtv = nullptr;
    }
}

void OnResizeEnd(unsigned, unsigned h) {
    if (h && h != g_bbHeight) {
        g_bbHeight = h;
        g_rebuild = true;
    }
}

void BeginUnload() {
    g_unloading = true;
    g_open = false;
    Input::OnOverlayClosed();
}

void Shutdown() {
    if (g_ready) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_ready = false;
    }
    if (g_rtv) g_rtv->Release();
    if (g_ctx) g_ctx->Release();
    if (g_dev) g_dev->Release();
    g_rtv = nullptr;
    g_ctx = nullptr;
    g_dev = nullptr;
}

HWND Window() { return g_hwnd; }
float Scale() { return BaseScale() * g_userScale; }
float UserScale() { return g_userScale; }

void SetUserScale(float s) {
    s = s < 0.6f ? 0.6f : (s > 2.0f ? 2.0f : s);
    if (s != g_userScale) {
        g_userScale = s;
        g_rebuild = true;
        Settings::SetFloat("ui", "scale", s);
    }
}

}  // namespace Overlay
