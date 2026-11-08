#include "Features/SocialLinks.h"

#include <cstdarg>
#include <cstdio>

#include "Data/LinkRules.h"
#include "Data/Links.h"
#include "Data/Stats.h"
#include "Engine/Addresses.h"
#include "Engine/Calendar.h"
#include "Engine/FlowCaller.h"
#include "Engine/GameThread.h"
#include "Engine/HostCheck.h"
#include "Features/Journal.h"
#include "Util/Log.h"

namespace Links {

namespace {

std::string Fmt(const char* fmt, ...) {
    char b[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    return b;
}

// The live entry for a link (base row, or its romance row once dating).
Game::Link* LiveEntry(uint16_t baseId, uint16_t* liveId) {
    auto* links = reinterpret_cast<Game::Link*>(Addr::SaveBlock + Game::Off::Links);
    const Data::LinkInfo* info = Data::Link(baseId);
    for (int i = 0; i < Game::kLinks; ++i) {
        if (links[i].id == baseId || (info && info->romanceRow && links[i].id == info->romanceRow)) {
            if (liveId) *liveId = links[i].id;
            return &links[i];
        }
    }
    return nullptr;
}

const Game::Link* SnapEntry(const Game::Snapshot& s, uint16_t baseId, uint16_t* liveId) {
    const Data::LinkInfo* info = Data::Link(baseId);
    for (const auto& l : s.links)
        if (l.id && (l.id == baseId || (info && info->romanceRow && l.id == info->romanceRow))) {
            if (liveId) *liveId = l.id;
            return &l;
        }
    return nullptr;
}

bool RankKnown(uint16_t link, uint8_t rank) {
    for (const auto& r : Data::kLinkRanksKnown)
        if (r.link == link && r.rank == rank) return true;
    return false;
}

const char* Person(uint16_t id) {
    const Data::LinkInfo* i = Data::Link(id);
    return i ? i->person : "?";
}

void Raise(Assessment& a, Theme::Risk r) {
    if (int(r) > int(a.risk)) a.risk = r;
}

}  // namespace

std::string Assessment::Text() const {
    std::string t;
    for (const auto& r : reasons) t += "• " + r + "\n";
    return t;
}

Assessment Assess(const Game::Snapshot& s, uint16_t base, Action act) {
    using Theme::Risk;
    Assessment a;
    const Data::LinkInfo* info = Data::Link(base);
    const Data::LinkRule* rule = Data::Rule(base);
    if (!info || info->kind == Data::LinkKind::Unused) {
        a.allowed = false;
        a.reasons.push_back("Unknown link.");
        return a;
    }
    uint16_t live = base;
    const Game::Link* e = SnapEntry(s, base, &live);
    bool started = e && e->rank > 0;
    int rank = started ? e->rank : 0;
    auto date = Calendar::FromIndex(s.day);

    if (info->kind == Data::LinkKind::Story) {
        Raise(a, Risk::Dangerous);
        a.reasons.push_back(Fmt("%s's link follows the main story; it normally ranks up only at fixed "
                                "story points. Changing it early can make later story scenes play out "
                                "of order.", info->person));
    }

    if (act == Action::Start) {
        if (started) {
            a.allowed = false;
            a.reasons.push_back("This link has already started.");
            return a;
        }
        Raise(a, Risk::Story);
        a.reasons.push_back("Starts the link the way the story does (the arcana card plays). The "
                            "introduction scene itself is skipped.");
        if (info->exclusive) {
            const Game::Link* other = SnapEntry(s, info->exclusive, nullptr);
            if (other && other->rank > 0) {
                Raise(a, Risk::Dangerous);
                a.reasons.push_back(Fmt("You already have %s's link; normally only one of the two can "
                                        "exist. Having both is not something the game expects.",
                                        Person(info->exclusive)));
            } else {
                a.reasons.push_back(Fmt("This locks you out of %s's link for the rest of the game.",
                                        Person(info->exclusive)));
            }
        }
        if (rule && rule->fromMonth) {
            int from = Calendar::ToIndex(rule->fromMonth, rule->fromDay);
            if (s.day < from) {
                Raise(a, Risk::Dangerous);
                a.reasons.push_back(Fmt("This link normally can't start before %d/%d.", rule->fromMonth,
                                        rule->fromDay));
            }
        }
        if (rule) {
            const Game::Unit& yu = s.U(Game::Yu);
            for (int i = 0; i < 5; ++i) {
                if (!rule->stat[i]) continue;
                int have = Data::StatRank(i, yu.social[i]);
                if (have < rule->stat[i]) {
                    Raise(a, Risk::Story);
                    a.reasons.push_back(Fmt("Normally needs %s rank %d (you have rank %d).",
                                            Data::kStats[i].name, rule->stat[i], have));
                }
            }
            if (rule->needLink) {
                const Game::Link* need = SnapEntry(s, rule->needLink, nullptr);
                // Ai accepts either Strength link (Kou or Daisuke).
                if ((!need || need->rank < rule->needRank) && rule->needLink == 16)
                    need = SnapEntry(s, 17, nullptr);
                if (!need || need->rank < rule->needRank) {
                    Raise(a, Risk::Story);
                    a.reasons.push_back(Fmt("Normally needs %s's link at rank %d first.",
                                            Person(rule->needLink), rule->needRank));
                }
            }
        }
        return a;
    }

    // Ready / RankUp need a started, unfinished link.
    if (!started) {
        a.allowed = false;
        a.reasons.push_back("Start this link first.");
        return a;
    }
    if (rank >= 10) {
        a.allowed = false;
        a.reasons.push_back("Already at the maximum rank.");
        return a;
    }
    if (info->exclusive && act != Action::Start) {
        // Fine: both can't be started, so nothing to say here.
    }
    if (rule && rule->untilMonth) {
        int until = Calendar::ToIndex(rule->untilMonth, rule->untilDay);
        if (s.day > until) {
            Raise(a, Risk::Dangerous);
            a.reasons.push_back(Fmt("Past %d/%d this link no longer advances in the story.", rule->untilMonth,
                                    rule->untilDay));
        }
    }
    if (base == 33 && rank == 4 && !s.Flag(52)) {
        Raise(a, Risk::Story);
        a.reasons.push_back("Marie's rank 5 normally needs Rise on the team.");
    }
    uint16_t th = Game::LinkThreshold(live, uint16_t(rank));

    if (act == Action::Ready) {
        Raise(a, Risk::Caution);
        if (e->points >= th) {
            a.allowed = false;
            a.reasons.push_back("Already ready: the next hangout ranks it up.");
            return a;
        }
        a.reasons.push_back(Fmt("Sets the points to %u, so your next hangout with %s plays the real "
                                "rank %d scene with all its choices.", th, info->person, rank + 1));
        if (rule && (rule->romance[0] == rank + 1 || rule->romance[1] == rank + 1))
            a.reasons.push_back("That scene contains a romance choice — see the Romance page for the answers.");
        return a;
    }

    // RankUp
    Raise(a, Risk::Story);
    a.reasons.push_back(Fmt("Raises %s's link to rank %d now, without the scene. The rank's skill "
                            "unlocks are applied; any choices in the scene are skipped.",
                            info->person, rank + 1));
    if (rule && (rule->romance[0] == rank + 1 || rule->romance[1] == rank + 1)) {
        Raise(a, Risk::Dangerous);
        a.reasons.push_back(Fmt("Rank %d's scene is where the romance choice happens. Skipping it leaves "
                                "you as friends. To date %s, use \"Ready at next hangout\" instead.",
                                rank + 1, info->person));
    }
    if (!RankKnown(base, uint8_t(rank + 1)))
        a.reasons.push_back("This rank's scene wasn't identified in the game's scripts, so only the rank "
                            "itself changes.");
    return a;
}

void SetPoints(uint16_t base, uint16_t pts) {
    GameThread::Post([base, pts] {
        if (!HostCheck::GroupOk("links")) return;
        uint16_t live = 0;
        Game::Link* e = LiveEntry(base, &live);
        if (!e) return;
        uint16_t old = e->points;
        if (old == pts) return;
        e->points = pts;
        Journal::Record(Fmt("%s link points %u → %u", Person(base), old, pts), [base, old] {
            if (Game::Link* x = LiveEntry(base, nullptr)) x->points = old;
        });
    });
}

void MakeReady(uint16_t base) {
    GameThread::Post([base] {
        uint16_t live = 0;
        Game::Link* e = LiveEntry(base, &live);
        if (!e) return;
        uint16_t th = Game::LinkThreshold(live, e->rank);
        if (e->points < th) SetPoints(base, th);
    });
}

void RankUpNow(uint16_t base) {
    GameThread::Post([base] {
        if (!HostCheck::GroupOk("links") || !HostCheck::GroupOk("flow")) return;
        uint16_t live = 0;
        Game::Link* e = LiveEntry(base, &live);
        if (!e || e->rank >= 10) return;
        uint16_t oldRank = e->rank, oldPts = e->points;
        e->points = Game::LinkThreshold(live, e->rank);
        Flow::Call(Flow::Fn::INCREASE_SL, {live});
        if (e->rank == oldRank) {  // the game refused (e.g. a story gate is active)
            e->points = oldPts;
            LOGW("links: the game refused to rank up %s (rank %u)", Person(base), oldRank);
            return;
        }
        // Replay what the rank's own scene does unconditionally, through the same functions.
        int applied = 0;
        for (const auto& fx : Data::kLinkEffects) {
            if (fx.link != base || fx.rank != e->rank) continue;
            switch (fx.kind) {
                case Data::LinkFx::LearnSkill: Flow::Call(Flow::Fn::LEARN_SL_SKILL, {fx.a, fx.b}); break;
                case Data::LinkFx::BitOn: Flow::Call(Flow::Fn::BIT_ON, {fx.a}); break;
                case Data::LinkFx::BitOff: Flow::Call(Flow::Fn::BIT_OFF, {fx.a}); break;
                case Data::LinkFx::AcceptQuest: Flow::Call(Flow::Fn::ACCEPT_QUEST, {fx.a}); break;
                case Data::LinkFx::CompleteQuest: Flow::Call(Flow::Fn::COMPLETE_QUEST, {fx.a}); break;
                case Data::LinkFx::AddYen: Flow::Call(Flow::Fn::ADD_YEN, {fx.a}); break;
            }
            ++applied;
        }
        LOGI("links: %s rank %u -> %u, %d scene effect(s) replayed", Person(base), oldRank, e->rank, applied);
        Journal::Record(Fmt("%s link rank %u → %u (%d scene effect%s applied; those aren't undone)",
                            Person(base), oldRank, e->rank, applied, applied == 1 ? "" : "s"),
                        [base, oldRank, oldPts] {
                            if (Game::Link* x = LiveEntry(base, nullptr)) {
                                x->rank = oldRank;
                                x->points = oldPts;
                            }
                        });
        (void)applied;
    });
}

void Start(uint16_t base) {
    GameThread::Post([base] {
        if (!HostCheck::GroupOk("flow")) return;
        Flow::Call(Flow::Fn::START_SL, {base});
        Journal::Record(Fmt("Started %s's link (card shown by the game)", Person(base)), nullptr);
    });
}

}  // namespace Links
