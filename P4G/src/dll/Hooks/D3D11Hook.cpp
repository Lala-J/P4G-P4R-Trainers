#include "Hooks/D3D11Hook.h"

#include <d3d11.h>
#include <dxgi1_2.h>

#include "Hooks/Hook.h"
#include "UI/Overlay.h"
#include "Util/Log.h"

namespace {

using Present_t = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
using Present1_t = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain1*, UINT, UINT,
                                                const DXGI_PRESENT_PARAMETERS*);
using Resize_t = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

Present_t o_Present = nullptr;
Present1_t o_Present1 = nullptr;
Resize_t o_Resize = nullptr;

HRESULT STDMETHODCALLTYPE Present_Hook(IDXGISwapChain* sc, UINT sync, UINT flags) {
    Hooks::Guard g;
    if (!(flags & DXGI_PRESENT_TEST)) Overlay::OnPresent(sc);
    return o_Present(sc, sync, flags);
}

HRESULT STDMETHODCALLTYPE Present1_Hook(IDXGISwapChain1* sc, UINT sync, UINT flags,
                                        const DXGI_PRESENT_PARAMETERS* p) {
    Hooks::Guard g;
    if (!(flags & DXGI_PRESENT_TEST)) Overlay::OnPresent(sc);
    return o_Present1(sc, sync, flags, p);
}

HRESULT STDMETHODCALLTYPE Resize_Hook(IDXGISwapChain* sc, UINT n, UINT w, UINT h, DXGI_FORMAT f,
                                      UINT fl) {
    Hooks::Guard g;
    Overlay::OnResizeBegin();
    HRESULT hr = o_Resize(sc, n, w, h, f, fl);
    Overlay::OnResizeEnd(w, h);
    return hr;
}

}  // namespace

namespace D3DHook {

bool Install() {
    WNDCLASSEXW wc{sizeof wc};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"P4GTrainerDummyD3D";
    RegisterClassExW(&wc);
    HWND w = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr,
                             nullptr, wc.hInstance, nullptr);
    if (!w) {
        LOGE("D3D hook: dummy window failed (%lu)", GetLastError());
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 64;
    sd.BufferDesc.Height = 64;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = w;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* sc = nullptr;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    HRESULT hr = E_FAIL;
    for (D3D_DRIVER_TYPE type : {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP}) {
        hr = D3D11CreateDeviceAndSwapChain(nullptr, type, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                           &sd, &sc, &dev, nullptr, &ctx);
        if (SUCCEEDED(hr)) break;
    }
    bool ok = false;
    if (SUCCEEDED(hr)) {
        void** vt = *reinterpret_cast<void***>(sc);
        ok = Hooks::Create("IDXGISwapChain::Present", vt[8], &Present_Hook, &o_Present) &&
             Hooks::Create("IDXGISwapChain::ResizeBuffers", vt[13], &Resize_Hook, &o_Resize);
        IDXGISwapChain1* sc1 = nullptr;
        if (SUCCEEDED(sc->QueryInterface(__uuidof(IDXGISwapChain1), reinterpret_cast<void**>(&sc1)))) {
            void** vt1 = *reinterpret_cast<void***>(sc1);
            if (vt1[22] != vt[8])
                Hooks::Create("IDXGISwapChain1::Present1", vt1[22], &Present1_Hook, &o_Present1);
            sc1->Release();
        }
        sc->Release();
        dev->Release();
        ctx->Release();
    } else {
        LOGE("D3D hook: dummy device failed (0x%08lX)", hr);
    }
    DestroyWindow(w);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return ok;
}

}  // namespace D3DHook
