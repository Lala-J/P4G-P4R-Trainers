// Headless self-test, run by tools/build.sh. Exercises the pieces that don't need the game:
// signature parsing/matching, the hook wrapper, and in-flight tracking.

#include <windows.h>
#include <cstdio>
#include <cstring>

#include "Engine/SigScan.h"
#include "Hooks/Hook.h"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (cond) ++g_pass;                                             \
        else {                                                          \
            ++g_fail;                                                   \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
        }                                                               \
    } while (0)

// ---------------------------------------------------------------- signatures
static void TestSig() {
    auto p = Sig::Parse("48 8B ?? 05 ? FF");
    CHECK(p.valid);
    CHECK(p.bytes.size() == 6);
    CHECK(p.mask[2] == 0 && p.mask[4] == 0 && p.mask[5] == 1);
    CHECK(!Sig::Parse("48 GG").valid);

    static uint8_t buf[64] = {};
    const uint8_t pat[] = {0x40, 0x53, 0x48, 0x83, 0xEC, 0x20};
    std::memcpy(buf + 17, pat, sizeof pat);
    std::memcpy(buf + 40, pat, sizeof pat);
    auto a = reinterpret_cast<uintptr_t>(buf);
    CHECK(Sig::MatchAt(a + 17, "40 53 48 83 EC 20"));
    CHECK(Sig::MatchAt(a + 17, "40 ?? 48 ?? EC"));
    CHECK(!Sig::MatchAt(a + 18, "40 53"));
    auto hits = Sig::Find(a, sizeof buf, Sig::Parse("40 53 48 ?? EC 20"));
    CHECK(hits.size() == 2 && hits[0] == a + 17 && hits[1] == a + 40);
    CHECK(Sig::Find(a, sizeof buf, Sig::Parse("40 53"), 1).size() == 1);
    CHECK(!Sig::Readable(0x10, 8));

    // RIP-relative: lea rax, [rip+0x10] at buf+0 (48 8D 05 10 00 00 00) -> buf+7+0x10
    const uint8_t lea[] = {0x48, 0x8D, 0x05, 0x10, 0x00, 0x00, 0x00};
    std::memcpy(buf, lea, sizeof lea);
    CHECK(Sig::RipTarget(a, 3, 7) == a + 7 + 0x10);
}

// ---------------------------------------------------------------- hooks
static int __declspec(noinline) Target(int x) {
    volatile int y = x * 3;
    return y + 1;
}
using Target_t = int (*)(int);
static Target_t o_Target = nullptr;
static int Detour(int x) {
    Hooks::Guard g;
    return o_Target(x) + 1000;
}

static DWORD WINAPI SlowCaller(LPVOID) {
    Hooks::Guard g;
    Sleep(200);
    return 0;
}

static void TestHooks() {
    CHECK(Hooks::Init());
    CHECK(Target(2) == 7);
    CHECK(Hooks::Create("Target", reinterpret_cast<void*>(&Target), &Detour, &o_Target));
    CHECK(Hooks::Count() == 1);
    CHECK(Hooks::EnableAll());
    CHECK(Target(2) == 1007);
    CHECK(Hooks::DisableAll());
    CHECK(Target(2) == 7);
    CHECK(Hooks::EnableAll());
    CHECK(Target(5) == 1016);

    // In-flight tracking: WaitIdle must not return early while a detour is running.
    HANDLE t = CreateThread(nullptr, 0, SlowCaller, nullptr, 0, nullptr);
    Sleep(30);
    CHECK(!Hooks::WaitIdle(50));
    CHECK(Hooks::WaitIdle(2000));
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);

    Hooks::Shutdown();
    CHECK(Target(2) == 7);
    CHECK(Hooks::Count() == 0);
}

int main() {
    TestSig();
    TestHooks();
    std::printf("selftest: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
