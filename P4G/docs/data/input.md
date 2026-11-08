# Game input

Verified live, Steam build 1.0.5. Each address is in `Engine/Addresses.h`.

P4G is a port of the PS Vita game. Its input layer (`CControllerSystem`, in the port's
emulation library) reads every device, then hands the game one Vita-style pad read per frame.

## Devices

| Device | How the game reads it |
|---|---|
| Keyboard | DirectInput8, `GetDeviceState` (256 bytes). Keys are mapped to pad buttons by the bindings in `%LOCALAPPDATA%\SEGA\P4G\P4G.ini` `[ActionConfig]`. |
| Mouse | DirectInput8, `GetDeviceState` (`DIMOUSESTATE2`) for the buttons; `GetCursorPos` for the position. The menus follow the cursor, and a left click is Confirm. |
| Pads | DirectInput8 (`DIJOYSTATE`) for plain HID pads. Pads with an XInput id (`IG_`) and pads managed by Steam go through the Steam Input API (`SteamController007`), which reads them inside the Steam client. Those never pass through XInput or DirectInput in the game's process. |

The Steam overlay wraps every DirectInput device the process creates, with a separate wrapper
class per device type. Each wrapper has its own `GetDeviceState`/`GetDeviceData` functions, so
hooking the keyboard's pair doesn't cover the mouse's.

## Merged pad read: `CtrlRead` `0x1404C1E70`

`int (?, SceCtrlData* out)`. It runs the controller-system update (`0x1404A9720`), which fills
the pad-state block at `0x145248348` (XInput button layout, triggers in bits 16–17). It then
builds `out`:

| Offset | Field |
|---|---|
| +0x00 | u64 timestamp |
| +0x08 | u32 buttons, Vita layout: Select 0x1, L3 0x2, R3 0x4, Start 0x8, Up 0x10, Right 0x20, Down 0x40, Left 0x80, L2 0x100, R2 0x200, L1 0x400, R1 0x800, Triangle 0x1000, Circle 0x2000, Cross 0x4000, Square 0x8000 |
| +0x0C | u8 lx, ly, rx, ry; 0x80 = centred |

Keyboard bindings (`0x1404A0080`) and left clicks (Cross) are merged in here.

Its only caller is the game's per-frame pad update (`PadUpdate`, `0x140505D50`). That copies
the result into the game's own pad globals at `0x15E3FD660`, maps the sticks onto the D-pad,
and uses neutral input (buttons 0, sticks 0x80) if the read fails.

The trainer returns neutral input from this read while it is open. Buttons and sticks still
held when it closes stay hidden until they are released.

`0x1404A20A8` also reads `0x145248348`. It is the key-config screen's "press a button to
assign" check.
