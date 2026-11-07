#pragma once
// IDXGISwapChain::Present / Present1 / ResizeBuffers detours. The method addresses are read
// from a throwaway swap chain, so the hooks sit on DXGI's own code and chain cleanly with
// other overlays (Steam, third-party trainers) that hook the same functions.

namespace D3DHook {
bool Install();
}
