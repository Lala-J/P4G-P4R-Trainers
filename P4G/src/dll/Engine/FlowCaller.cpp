#include "Engine/FlowCaller.h"

#include <cstring>

#include "Engine/Addresses.h"
#include "Engine/GameThread.h"
#include "Engine/HostCheck.h"
#include "Engine/SigScan.h"
#include "Util/Log.h"

namespace Flow {

namespace {

struct Entry {
    uint64_t fn;
    uint32_t argc;
    uint32_t pad;
};

const Entry* Lookup(uint16_t id) {
    uint16_t group = id >> 12, index = id & 0xFFF;
    uintptr_t base = 0;
    uint32_t count = 0;
    switch (group) {
        case 0: base = Addr::FlowTableCommon; count = 192; break;
        case 1: base = Addr::FlowTableField; count = 80; break;
        case 3: base = Addr::FlowTableEvent; count = 78; break;
        default: return nullptr;
    }
    if (index >= count) return nullptr;
    auto* e = reinterpret_cast<const Entry*>(base + index * sizeof(Entry));
    if (!Sig::Readable(reinterpret_cast<uintptr_t>(e), sizeof(Entry))) return nullptr;
    if (e->fn < Addr::kSharedStart || e->fn >= Addr::kSharedStart + Addr::kSharedSize) return nullptr;
    return e;
}

// Large enough for the 48 argument slots plus the result slot the game's layout implies.
alignas(16) uint8_t g_ctx[0x400];

}  // namespace

int ArgCount(uint16_t id) {
    const Entry* e = Lookup(id);
    return e ? int(e->argc) : -1;
}

Result Call(uint16_t id, std::initializer_list<int64_t> args) {
    Result r;
    if (!HostCheck::GroupOk("flow") || !GameThread::IsGameThread()) {
        LOGW("flow: 0x%04X not called (%s)", id,
             GameThread::IsGameThread() ? "flow address checks failed" : "not on the game thread");
        return r;
    }
    const Entry* e = Lookup(id);
    if (!e) {
        LOGW("flow: unknown function 0x%04X", id);
        return r;
    }
    int argc = int(args.size());
    if (argc != int(e->argc)) {
        LOGW("flow: 0x%04X takes %u argument(s), %d given", id, e->argc, argc);
        return r;
    }

    std::memset(g_ctx, 0, sizeof g_ctx);
    int sp = argc > 0 ? argc - 1 : 0;
    *reinterpret_cast<uint32_t*>(g_ctx + 0x20) = uint32_t(sp);
    int i = 0;
    for (int64_t v : args) {
        int slot = sp - i++;
        g_ctx[0x23 + slot] = 0;  // integer literal
        *reinterpret_cast<int64_t*>(g_ctx + 0x50 + slot * 8) = v;
    }

    auto** cur = reinterpret_cast<uint8_t**>(Addr::pFlowContext);
    uint8_t* saved = *cur;
    *cur = g_ctx;
    int ret = reinterpret_cast<int (*)()>(e->fn)();
    *cur = saved;

    r.ok = true;
    r.done = ret != 0;
    r.value = *reinterpret_cast<int64_t*>(g_ctx + 0x1D0);
    return r;
}

}  // namespace Flow
