#pragma once
// Character portraits decoded from the player's own data.cpk (bustup/b<char>_<expr>_<outfit>.bin)
// on a background thread, then uploaded as textures on the render thread. Nothing is cached on
// disk and nothing ships with the trainer.

#include <imgui.h>

namespace Portraits {

void Init(void* d3dDevice);   // render thread, once
void Shutdown();

// Texture for a bustup, or null while loading / if unavailable. uv0/uv1 receive the figure's
// bounding box on the sheet; `aspect` its width / height in pixels.
ImTextureID Get(int charId, int expression, int outfit, ImVec2* uv0, ImVec2* uv1, float* aspect = nullptr);

// Draws the figure filling `min..max` without distortion: scaled to cover the area, centred
// horizontally and anchored at the top (head and shoulders stay in view). Returns false if not ready.
bool DrawCover(ImDrawList* dl, int charId, int expression, int outfit, ImVec2 min, ImVec2 max, ImU32 tint);

}  // namespace Portraits
