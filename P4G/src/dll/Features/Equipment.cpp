#include "Features/Equipment.h"

#include <cstdarg>
#include <cstdio>
#include <string>

#include "Engine/Addresses.h"
#include "Engine/FlowCaller.h"
#include "Engine/GameState.h"
#include "Engine/GameThread.h"
#include "Engine/GameTypes.h"
#include "Engine/HostCheck.h"
#include "Engine/Names.h"
#include "Engine/Natives.h"
#include "Engine/SigScan.h"
#include "Features/Journal.h"
#include "Util/Log.h"

namespace Equipment {

namespace {

uint8_t* Block() { return reinterpret_cast<uint8_t*>(Addr::SaveBlock); }
Game::Unit* Units() { return reinterpret_cast<Game::Unit*>(Block() + Game::Off::Units); }
uint8_t Count(uint16_t item) { return item < Game::kItems ? Block()[Game::Off::Items + item] : 0; }

const uint8_t* Entry(uint16_t item) {
    if (!Sig::Readable(Addr::pItemTable, 8)) return nullptr;
    auto t = *reinterpret_cast<uintptr_t*>(Addr::pItemTable);
    if (!t || item >= Game::kItems || !Sig::Readable(t + item * 0x44, 0x44)) return nullptr;
    return reinterpret_cast<const uint8_t*>(t + item * 0x44);
}

std::string Fmt(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    return buf;
}

const char* CharName(int id) {
    static const char* n[] = {"", "Yu", "Yosuke", "Chie", "Yukiko", "Rise", "Kanji", "Naoto", "Teddie"};
    return (id >= 1 && id <= 8) ? n[id] : "?";
}

// The same bookkeeping as the equip menu, then the game's own swap.
bool Swap(int charId, int slot, uint16_t from, uint16_t to) {
    bool bag = slot != Outfit;
    if (bag) {
        Natives::SetItemCount(to, uint8_t(Count(to) - 1));
        if (from) Natives::SetItemCount(from, uint8_t(Count(from) + 1));
    }
    Flow::Call(Flow::Fn::SET_EQUIPMENT, {charId, slot, to});
    if (Units()[charId - 1].equip[slot] == to) return true;
    // The game refused: put the bag back as it was.
    if (bag) {
        Natives::SetItemCount(to, uint8_t(Count(to) + 1));
        if (from) Natives::SetItemCount(from, uint8_t(Count(from) - 1));
    }
    LOGW("equipment: the game did not equip item %u on %s", to, CharName(charId));
    return false;
}

}  // namespace

bool Available() {
    return HostCheck::GroupOk("equipment") && HostCheck::GroupOk("flow") && HostCheck::GroupOk("inventory");
}

const char* SlotName(int slot) {
    switch (slot) {
        case Weapon: return "Weapon";
        case Armour: return "Armour";
        case Accessory: return "Accessory";
        case Outfit: return "Outfit";
    }
    return "?";
}

bool ItemFitsSlot(uint16_t item, int slot) {
    switch (slot) {
        case Weapon: return (item >= 0x001 && item < 0x0FE) || (item >= 0x900 && item < 0xA00);
        case Armour: return item > 0x100 && item < 0x200;
        case Accessory: return item > 0x200 && item < 0x300;
        case Outfit: return item >= 0x700 && item < 0x800;
    }
    return false;
}

bool CanWield(uint16_t item, int charId) {
    if (!Available() || charId < 1 || charId > 8) return false;
    return reinterpret_cast<int (*)(int16_t, int16_t)>(Addr::CanEquip)(int16_t(item), int16_t(charId)) != 0;
}

Stats ItemStats(uint16_t item) {
    Stats s;
    const uint8_t* e = Entry(item);
    if (!e) return s;
    auto u16 = [e](int off) { return int(*reinterpret_cast<const uint16_t*>(e + off)); };
    s.attack = u16(0x08);
    s.hit = u16(0x0A);
    s.defence = u16(0x10);
    s.evasion = u16(0x12);
    return s;
}

const char* WhyNot(int charId, int slot, uint16_t item) {
    if (!Available()) return "Unavailable on this game build.";
    const auto& s = Game::Current();
    if (!s.valid) return "No save is loaded.";
    if (s.inBattle) return "Not during a battle.";
    if (s.campMenu) return "Close the game's menu first.";
    if (charId < 1 || charId > 8 || slot < 0 || slot > 3) return "?";
    if (!ItemFitsSlot(item, slot)) return "That item doesn't go in this slot.";
    if (!CanWield(item, charId)) return "This member can't use it.";
    uint16_t cur = s.U(charId).equip[slot];
    if (cur == item) return "Already equipped.";
    if (slot != Outfit && Count(item) == 0) return "Not in the bag (get one on the Inventory page first).";
    if (slot == Outfit && Count(item) == 0) return "Not in the wardrobe.";
    if (slot != Outfit && cur && Count(cur) >= 99) return "The bag already holds 99 of the current piece.";
    return nullptr;
}

void Equip(int charId, int slot, uint16_t item) {
    GameThread::Post([charId, slot, item] {
        if (WhyNot(charId, slot, item)) return;
        uint16_t from = Units()[charId - 1].equip[slot];
        if (!Swap(charId, slot, from, item)) return;
        std::string a = Names::ItemOr(from), b = Names::ItemOr(item);
        Journal::Record(Fmt("%s's %s: %s → %s", CharName(charId), SlotName(slot), a.c_str(), b.c_str()),
                        [charId, slot, from, item] {
                            if (Units()[charId - 1].equip[slot] != item) return;
                            if (slot != Outfit && from && Count(from) == 0) return;
                            Swap(charId, slot, item, from);
                        });
    });
}

}  // namespace Equipment
