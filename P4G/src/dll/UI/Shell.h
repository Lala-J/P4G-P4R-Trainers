#pragma once
// The trainer modal: header, category rail, page area, footer hints.

namespace Shell {

void Draw(bool open);   // called inside an ImGui frame
void NotifyOpened();    // the modal just opened (focus goes to the page rail)
bool WantsHud();        // true when something must render while the modal is closed

}  // namespace Shell
