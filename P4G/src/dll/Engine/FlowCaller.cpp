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
alignas(16) uint8_t g_yctx[0x400];  // the context of the one yielding call in progress
struct Yield {
    bool active = false;
    uint64_t fn = 0;
    uint16_t id = 0;
    int framesLeft = 0;
    void (*onDone)(bool) = nullptr;
} g_y;
struct Seq {
    bool active = false;
    Step steps[16];
    int count = 0, pos = 0;
    const char* what = "";
} g_seq;

void Fill(uint8_t* ctx, std::initializer_list<int64_t> args) {
    std::memset(ctx, 0, 0x400);
    int argc = int(args.size());
    int sp = argc > 0 ? argc - 1 : 0;
    *reinterpret_cast<uint32_t*>(ctx + 0x20) = uint32_t(sp);
    int i = 0;
    for (int64_t v : args) {
        int slot = sp - i++;
        ctx[0x23 + slot] = 0;
        *reinterpret_cast<int64_t*>(ctx + 0x50 + slot * 8) = v;
    }
}

int Invoke(uint64_t fn, uint8_t* ctx) {
    auto** cur = reinterpret_cast<uint8_t**>(Addr::pFlowContext);
    uint8_t* saved = *cur;
    *cur = ctx;
    int ret = reinterpret_cast<int (*)()>(fn)();
    *cur = saved;
    return ret;
}

void YieldTick() {
    if (!g_y.active) return;
    // Like the script VM between re-calls of a waiting function: advance the context's frame
    // timers (+0x224 "time in this call", +0x220 "time in this script"), keep the arguments.
    constexpr float kFrame = 1.0f / 60.0f;
    *reinterpret_cast<float*>(g_yctx + 0x21C) = *reinterpret_cast<float*>(g_yctx + 0x224);
    *reinterpret_cast<float*>(g_yctx + 0x224) += kFrame;
    *reinterpret_cast<float*>(g_yctx + 0x218) = *reinterpret_cast<float*>(g_yctx + 0x220);
    *reinterpret_cast<float*>(g_yctx + 0x220) += kFrame;
    int ret = Invoke(g_y.fn, g_yctx);
    bool done = ret != 0;
    if (done || --g_y.framesLeft <= 0) {
        g_y.active = false;
        LOGI("flow: 0x%04X %s", g_y.id, done ? "finished" : "gave up waiting");
        if (g_y.onDone) g_y.onDone(done);
        return;
    }
    GameThread::Post(YieldTick);
}

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

    Fill(g_ctx, args);
    int ret = Invoke(e->fn, g_ctx);

    r.ok = true;
    r.done = ret != 0;
    r.value = *reinterpret_cast<int64_t*>(g_ctx + 0x1D0);
    return r;
}

void CallYielding(uint16_t id, std::initializer_list<int64_t> args, int maxFrames, void (*onDone)(bool)) {
    if (g_y.active) {
        LOGW("flow: 0x%04X not started; 0x%04X is still running", id, g_y.id);
        return;
    }
    if (!HostCheck::GroupOk("flow") || !GameThread::IsGameThread()) return;
    const Entry* e = Lookup(id);
    if (!e || int(args.size()) != int(e->argc)) return;
    Fill(g_yctx, args);
    int ret = Invoke(e->fn, g_yctx);
    LOGI("flow: 0x%04X started (%s)", id, ret ? "finished at once" : "running");
    if (ret) {
        if (onDone) onDone(true);
        return;
    }
    g_y = {true, e->fn, id, maxFrames, onDone};
    GameThread::Post(YieldTick);
}

bool YieldingBusy() { return g_y.active || g_seq.active; }

namespace {
void SeqNext(bool);
void SeqStart() {
    while (g_seq.pos < g_seq.count) {
        const Step& st = g_seq.steps[g_seq.pos++];
        const Entry* e = Lookup(st.id);
        if (!e || int(e->argc) != st.argc) {
            LOGE("flow: sequence '%s' step 0x%04X has a bad argument count; stopped", g_seq.what, st.id);
            g_seq.active = false;
            return;
        }
        Fill(g_yctx, {});
        int sp = st.argc > 0 ? st.argc - 1 : 0;
        *reinterpret_cast<uint32_t*>(g_yctx + 0x20) = uint32_t(sp);
        for (int i = 0; i < st.argc; ++i)
            *reinterpret_cast<int64_t*>(g_yctx + 0x50 + (sp - i) * 8) = st.a[i];
        if (Invoke(e->fn, g_yctx)) continue;  // finished this frame
        g_y = {true, e->fn, st.id, 60 * 60 * 30, SeqNext};
        GameThread::Post(YieldTick);
        return;
    }
    LOGI("flow: sequence '%s' finished", g_seq.what);
    g_seq.active = false;
}
void SeqNext(bool) { GameThread::Post(SeqStart); }
}  // namespace

bool RunSequence(const Step* steps, int count, const char* what) {
    if (YieldingBusy() || count > 16 || !GameThread::IsGameThread() || !HostCheck::GroupOk("flow")) return false;
    for (int i = 0; i < count; ++i) g_seq.steps[i] = steps[i];
    g_seq.count = count;
    g_seq.pos = 0;
    g_seq.what = what;
    g_seq.active = true;
    LOGI("flow: sequence '%s' (%d steps)", what, count);
    SeqStart();
    return true;
}

}  // namespace Flow
