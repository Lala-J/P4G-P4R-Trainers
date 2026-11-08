#pragma once
// Character portraits decoded from the player's own data.cpk (bustup/b<char>_<expr>_<outfit>.bin)
// on a background thread, then uploaded as textures on the render thread. Nothing is cached on
// disk and nothing ships with the trainer.

#include <imgui.h>

namespace Portraits {

void Init(void* d3dDevice);   // render thread, once
void Shutdown();

// Texture for a bustup, or null while loading / if unavailable. `uv1` receives the crop of the
// figure (the art occupies the upper part of a 1024x1024 sheet).
ImTextureID Get(int charId, int expression, int outfit, ImVec2* uv0, ImVec2* uv1);

}  // namespace Portraits
