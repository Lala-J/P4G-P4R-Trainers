#include "Features/Inventory.h"

#include <intrin.h>
#include <cstdint>

#include "Engine/Addresses.h"
#include "Engine/GameTypes.h"
#include "Engine/HostCheck.h"
#include "Engine/Natives.h"
#include "Hooks/Hook.h"
#include "Hooks/InputHooks.h"
#include "Util/Log.h"
#include "Util/Settings.h"

namespace InventoryFx {

namespace {

bool g_never = false;

using SetItem_t = void (*)(uint16_t, uint8_t);
SetItem_t o_SetItem = nullptr;

// Call sites (address of the `call SetItemCount`) that consume an item by using it.
// Each entry was confirmed by using an item in game with logging on. [DRV]
constexpr uintptr_t kUseSites[] = {
    0,  // filled in as each path is confirmed
};

bool IsUseSite(uintptr_t ret) {
    uintptr_t call = ret - 5;
    for (uintptr_t a : kUseSites)
        if (a && a == call) return true;
    return false;
}

void SetItem_Hook(uint16_t id, uint8_t count) {
    Hooks::Guard g;
    auto ret = reinterpret_cast<uintptr_t>(_ReturnAddress());
    uint8_t old = id < Game::kItems ? reinterpret_cast<const uint8_t*>(Addr::SaveBlock + Game::Off::Items)[id] : 0;
    if (Input::g_debug)
        LOGI("items: SetItemCount(0x%03X, %u) was %u, called from 0x%llX", id, count, old,
             (unsigned long long)(ret - 5));
    if (g_never && count < old && IsUseSite(ret)) return;  // the item isn't used up
    o_SetItem(id, count);
}

}  // namespace

bool& NeverRunOut() { return g_never; }

bool Available() {
    for (uintptr_t a : kUseSites)
        if (a) return o_SetItem != nullptr;
    return false;
}

void SaveOptions() { Settings::SetInt("items", "never_run_out", g_never ? 1 : 0); }

bool InstallHooks() {
    g_never = Settings::GetInt("items", "never_run_out", 0) != 0;
    if (!HostCheck::GroupOk("inventory")) return false;
    return Hooks::Create("SetItemCount", reinterpret_cast<void*>(Addr::SetItemCount), &SetItem_Hook,
                         &o_SetItem);
}

}  // namespace InventoryFx
