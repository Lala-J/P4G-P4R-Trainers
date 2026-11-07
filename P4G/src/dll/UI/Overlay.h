#pragma once
// Owns the ImGui context and the trainer modal. Everything here runs on the game's render
// thread (inside Present) except IsOpen/Toggle, which are safe from any thread.

#include <windows.h>

struct IDXGISwapChain;

namespace Overlay {

bool IsOpen();
void SetOpen(bool open);
void Toggle();

void OnPresent(IDXGISwapChain* sc);
void OnResizeBegin();
void OnResizeEnd(unsigned w, unsigned h);

// Unload sequence: BeginUnload stops all rendering; Shutdown frees ImGui and D3D objects
// (call it only after hooks are disabled and idle).
void BeginUnload();
void Shutdown();

HWND Window();
float Scale();       // backbuffer-derived UI scale times the user's preference
void SetUserScale(float s);
float UserScale();

}  // namespace Overlay
