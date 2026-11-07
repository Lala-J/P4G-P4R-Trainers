#include "Engine/Natives.h"

#include "Engine/Addresses.h"
#include "Engine/HostCheck.h"

namespace Natives {

thread_local bool g_trainerCall = false;

namespace {
template <typename F>
F At(uintptr_t a) {
    return reinterpret_cast<F>(a);
}
struct Scope {
    Scope() { g_trainerCall = true; }
    ~Scope() { g_trainerCall = false; }
};
}  // namespace

void SetItemCount(uint16_t id, uint8_t count) {
    if (!HostCheck::GroupOk("inventory") || id >= Game::kItems) return;
    At<void (*)(uint16_t, uint8_t)>(Addr::SetItemCount)(id, count);
}

uint16_t GetMaxHP(const Game::Unit* u) {
    if (!HostCheck::GroupOk("battle")) return 0;
    return At<uint16_t (*)(const Game::Unit*)>(Addr::GetMaxHP)(u);
}

uint16_t GetMaxSP(const Game::Unit* u) {
    if (!HostCheck::GroupOk("battle")) return 0;
    return At<uint16_t (*)(const Game::Unit*)>(Addr::GetMaxSP)(u);
}

void AddHP(Game::Unit* u, int delta) {
    if (!HostCheck::GroupOk("battle")) return;
    Scope s;
    At<void (*)(Game::Unit*, int)>(Addr::AddHP)(u, delta);
}

void AddSP(Game::Unit* u, int delta) {
    if (!HostCheck::GroupOk("battle")) return;
    Scope s;
    At<void (*)(Game::Unit*, int)>(Addr::AddSP)(u, delta);
}

}  // namespace Natives
