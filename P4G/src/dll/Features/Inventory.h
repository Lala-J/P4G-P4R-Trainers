#pragma once
// "Items never run out": every inventory change goes through the game's SetItemCount; a
// decrease is skipped only when it comes from a confirmed item-*use* call site (battle, camp
// menu, dungeon items). Shops, gifts, quest hand-ins, fusion and scripted removals still work.

namespace InventoryFx {

bool InstallHooks();
bool& NeverRunOut();
bool Available();   // false until at least one item-use call site is confirmed
void SaveOptions();

}  // namespace InventoryFx
