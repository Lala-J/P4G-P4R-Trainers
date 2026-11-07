#include "Features/Edits.h"

#include <cstdarg>
#include <cstdio>
#include <string>

#include "Data/Stats.h"
#include "Engine/Addresses.h"
#include "Engine/GameState.h"
#include "Engine/GameThread.h"
#include "Engine/GameTypes.h"
#include "Engine/HostCheck.h"
#include "Engine/Names.h"
#include "Engine/Natives.h"
#include "Features/Journal.h"

namespace Edits {

namespace {

uint8_t* Block() { return reinterpret_cast<uint8_t*>(Addr::SaveBlock); }
Game::Unit* Units() { return reinterpret_cast<Game::Unit*>(Block() + Game::Off::Units); }
Game::Persona* Stock() { return reinterpret_cast<Game::Persona*>(Block() + Game::Off::Stock); }

bool CoreOk() { return HostCheck::GroupOk("core"); }

std::string Fmt(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    return buf;
}

}  // namespace

void SetYen(uint32_t yen) {
    if (yen > kMaxYen) yen = kMaxYen;
    GameThread::Post([yen] {
        if (!CoreOk()) return;
        auto* p = reinterpret_cast<uint32_t*>(Block() + Game::Off::Yen);
        uint32_t old = *p;
        if (old == yen) return;
        *p = yen;
        Journal::Record(Fmt("Money ¥%u → ¥%u", old, yen), [old] {
            *reinterpret_cast<uint32_t*>(Block() + Game::Off::Yen) = old;
        });
    });
}

void SetSocialStat(int stat, uint16_t points) {
    if (stat < 0 || stat > 4) return;
    GameThread::Post([stat, points] {
        if (!CoreOk()) return;
        uint16_t& v = Units()[0].social[stat];
        uint16_t old = v;
        if (old == points) return;
        v = points;
        Journal::Record(Fmt("%s %u → %u", Data::kStats[stat].name, old, points), [stat, old] {
            Units()[0].social[stat] = old;
        });
    });
}

void SetItemCount(uint16_t id, uint8_t count) {
    if (id >= Game::kItems) return;
    if (count > 99) count = 99;
    GameThread::Post([id, count] {
        if (!CoreOk()) return;
        uint8_t old = Block()[Game::Off::Items + id];
        if (old == count) return;
        Natives::SetItemCount(id, count);
        std::string name = Names::ItemOr(id);
        Journal::Record(Fmt("%s ×%u → ×%u", name.c_str(), old, count), [id, old] {
            Natives::SetItemCount(id, old);
        });
    });
}

void SetPersonaStat(int slot, int stat, uint8_t base) {
    if (slot < 0 || slot >= Game::kStock || stat < 0 || stat > 4) return;
    GameThread::Post([slot, stat, base] {
        if (!CoreOk()) return;
        Game::Persona& p = Stock()[slot];
        if (!p.valid) return;
        uint8_t old = p.stats[stat];
        if (old == base) return;
        p.stats[stat] = base;
        static const char* n[] = {"St", "Ma", "En", "Ag", "Lu"};
        Journal::Record(Fmt("%s %s %u → %u", Names::PersonaOr(p.id).c_str(), n[stat], old, base),
                        [slot, stat, old, id = p.id] {
                            Game::Persona& q = Stock()[slot];
                            if (q.valid && q.id == id) q.stats[stat] = old;
                        });
    });
}

void SetPersonaSkill(int slot, int index, uint16_t skill) {
    if (slot < 0 || slot >= Game::kStock || index < 0 || index > 7) return;
    GameThread::Post([slot, index, skill] {
        if (!CoreOk()) return;
        Game::Persona& p = Stock()[slot];
        if (!p.valid) return;
        uint16_t old = p.skills[index];
        if (old == skill) return;
        p.skills[index] = skill;
        Journal::Record(Fmt("%s skill %d: %s → %s", Names::PersonaOr(p.id).c_str(), index + 1,
                            old ? Names::SkillOr(old).c_str() : "(empty)",
                            skill ? Names::SkillOr(skill).c_str() : "(empty)"),
                        [slot, index, old, id = p.id] {
                            Game::Persona& q = Stock()[slot];
                            if (q.valid && q.id == id) q.skills[index] = old;
                        });
    });
}

void SetUnitHpSp(int charId, int hp, int sp) {
    if (charId < 1 || charId > Game::kUnits) return;
    GameThread::Post([charId, hp, sp] {
        if (!CoreOk()) return;
        Game::Unit* u = &Units()[charId - 1];
        uint16_t oldHp = u->hp, oldSp = u->sp;
        // Through AddHP / AddSP, so the game clamps to 0..max exactly as it always does.
        if (hp >= 0 && hp != u->hp) Natives::AddHP(u, hp - int(u->hp));
        if (sp >= 0 && sp != u->sp) Natives::AddSP(u, sp - int(u->sp));
        if (u->hp == oldHp && u->sp == oldSp) return;
        static const char* n[] = {"", "Yu", "Yosuke", "Chie", "Yukiko", "Rise", "Kanji", "Naoto", "Teddie"};
        Journal::Record(Fmt("%s HP %u → %u, SP %u → %u", n[charId], oldHp, u->hp, oldSp, u->sp),
                        [charId, oldHp, oldSp] {
                            Game::Unit* v = &Units()[charId - 1];
                            Natives::AddHP(v, int(oldHp) - int(v->hp));
                            Natives::AddSP(v, int(oldSp) - int(v->sp));
                        });
    });
}

}  // namespace Edits
