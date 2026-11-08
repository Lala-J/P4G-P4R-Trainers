#pragma once
// Game input routing.
//   P4G reads the keyboard through DirectInput8 and pads through XInput (loaded at run time),
//   and clips / recentres the cursor. While the trainer is open, all of that is withheld
//   from the game and given to the overlay instead. Keys and buttons used inside the
//   overlay are latched so they don't leak into the game when it closes. Key releases are
//   always passed through, so a key held when the overlay opened can never stay stuck.

#include <windows.h>
#include <Xinput.h>

namespace Input {

bool Install();                 // creates the DInput8 / XInput / cursor hooks
void PollLateModules();         // hooks XInput DLLs the game loads later (render thread)
bool InstallWndProc(HWND hwnd); // subclass the game window
bool RestoreWndProc();          // false if someone subclassed after us (unload unsafe)

// Our own pad read (bypasses the game-facing filter). Returns false if no pad.
bool ReadPad(XINPUT_STATE* out);

// Replays window messages captured on the window thread into ImGui (render thread).
void DrainToImGui();

// Mouse wheel. The game's message pump doesn't deliver WM_MOUSEWHEEL reliably, so a low-level
// mouse hook on the trainer's own thread collects wheel turns over the game window while the
// overlay is open. TakeWheel returns the notches since the last call (render thread).
void StartWheel();
void StopWheel();               // before unloading
float TakeWheel(float* horizontal);

// Presses the game's Confirm key (from its own key bindings) for the next keyboard poll.
void InjectConfirm();

void OnOverlayOpened();
void OnOverlayClosed();

// Verbose input logging, enabled when %LOCALAPPDATA%\P4GTrainer\debug-input exists.
extern bool g_debug;

// Virtual key that toggles the overlay (default VK_INSERT).
void SetToggleKey(int vk);
int ToggleKey();

}  // namespace Input
