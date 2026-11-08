#!/usr/bin/env bash
# Build the P4G trainer (x64, MSVC): trainer DLL, injector, self-tests; stage dist/.
#   usage: tools/build.sh [release|debug]
# Incremental: an object is rebuilt when its source, or any project header, is newer.
# Never deletes anything: stale dist/ files are moved to the repo's .trash/ folder.
set -u
cd "$(dirname "$0")/.."
P4G_ROOT="$(pwd)"
REPO_ROOT="$(cd .. && pwd)"
source tools/env.sh || exit 1
export MSYS2_ARG_CONV_EXCL='*'   # stop MSYS mangling /flags and -Fo: paths

CFG="${1:-release}"
OUT="build/$CFG"
OBJ="$OUT/obj"
mkdir -p "$OBJ" "$OUT/test"

[ -d third_party/imgui ] && [ -d third_party/minhook ] || bash tools/fetch_deps.sh || exit 1

BUILD_ID="$(git rev-parse --short HEAD 2>/dev/null || echo nogit)"
git diff --quiet HEAD -- . 2>/dev/null || BUILD_ID="$BUILD_ID+dirty"
BUILD_ID="$BUILD_ID-$CFG"
GEN="$OUT/gen"
GEN_H="$GEN/BuildId.h"
mkdir -p "$GEN"
NEW_GEN="#define P4G_BUILD \"$BUILD_ID\""
[ -f "$GEN_H" ] && [ "$(cat "$GEN_H")" = "$NEW_GEN" ] || echo "$NEW_GEN" > "$GEN_H"

if [ "$CFG" = "debug" ]; then
    OPT="-Od -Zi -D_DEBUG -DP4G_DEBUG"
else
    OPT="-O2 -Zi -DNDEBUG"
fi
DEFS="-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -DUNICODE -D_UNICODE"
INCS="-Isrc/dll -Ithird_party/imgui -Ithird_party/imgui/backends -Ithird_party/minhook/include"
CXX_OWN="-nologo -std:c++20 -EHsc -W3 -GS- -MT -utf-8 $OPT $DEFS $INCS -I$GEN"
CXX_3P="-nologo -std:c++20 -EHsc -W0 -GS- -MT -utf-8 $OPT $DEFS $INCS -DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD"
C_3P="-nologo -W0 -GS- -MT $OPT -Ithird_party/minhook/include"

NEWEST_H="$(find src -name '*.h' -o -name '*.inc' | xargs ls -t 2>/dev/null | head -1)"
JOBS="${NUMBER_OF_PROCESSORS:-4}"
fail=0
FAILFILE="$OBJ/.failed"
: > "$FAILFILE"

# compile <source> <flags...>  -> prints the object path
obj_of() { echo "$OBJ/$(echo "$1" | sed 's#[/.]#_#g').obj"; }
compile() {
    local src=$1; shift
    local o; o=$(obj_of "$src")
    if [ -f "$o" ] && [ "$o" -nt "$src" ] && { [ -z "$NEWEST_H" ] || [ "$o" -nt "$NEWEST_H" ] || [[ "$src" == third_party/* ]]; } \
       && ! { [[ "$src" == */Core/Trainer.cpp ]] && [ "$GEN_H" -nt "$o" ]; }; then
        return 0
    fi
    if ! "$CL_EXE" -c "$@" -Fo:"$o" -Fd:"$OBJ/vc.pdb" -FS "$src" > "$o.txt" 2>&1; then
        grep -v '^$' "$o.txt" | grep -v "^$(basename "$src")$" | head -40
        echo "$src" >> "$FAILFILE"
        return 1
    fi
    grep -iE "warning|error" "$o.txt" | head -20
    return 0
}

run_parallel() {   # flags... -- sources...
    local flags=()
    while [ "$1" != "--" ]; do flags+=("$1"); shift; done; shift
    local n=0
    for s in "$@"; do
        compile "$s" "${flags[@]}" &
        n=$((n + 1))
        if [ $n -ge "$JOBS" ]; then wait -n; n=$((n - 1)); fi
    done
    wait
}

IMGUI_SRC="third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp third_party/imgui/imgui_tables.cpp
           third_party/imgui/imgui_widgets.cpp third_party/imgui/backends/imgui_impl_dx11.cpp
           third_party/imgui/backends/imgui_impl_win32.cpp"
MINHOOK_SRC="third_party/minhook/src/buffer.c third_party/minhook/src/hook.c
             third_party/minhook/src/trampoline.c third_party/minhook/src/hde/hde64.c"
DLL_SRC=$(find src/dll -name '*.cpp' | sort)
INJ_SRC=$(find src/injector -name '*.cpp' | sort)
PROXY_SRC=$(find src/proxy -name '*.cpp' | sort)

echo "=== compile ($CFG, $BUILD_ID) ==="
run_parallel $CXX_3P -- $IMGUI_SRC
run_parallel $C_3P -- $MINHOOK_SRC
run_parallel $CXX_OWN -- $DLL_SRC $INJ_SRC $PROXY_SRC src/tests/selftest.cpp
if [ -s "$FAILFILE" ]; then
    echo; echo "compile errors in:"; sed 's/^/  /' "$FAILFILE"
    fail=1
fi

objs() { for s in "$@"; do obj_of "$s"; done; }

if [ $fail -eq 0 ]; then
    echo "=== link P4GTrainer.dll ==="
    "$LINK_EXE" -nologo -DLL -DEBUG -OPT:REF -OPT:ICF -MACHINE:X64 \
        -OUT:"$OUT/P4GTrainer.dll" -PDB:"$OUT/P4GTrainer.pdb" \
        $(objs $DLL_SRC $IMGUI_SRC $MINHOOK_SRC) \
        user32.lib kernel32.lib gdi32.lib shell32.lib ole32.lib advapi32.lib \
        d3d11.lib dxgi.lib d3dcompiler.lib dinput8.lib dxguid.lib imm32.lib dwmapi.lib \
        | grep -v "^ *Creating library" || true
    [ -f "$OUT/P4GTrainer.dll" ] && [ "$OUT/P4GTrainer.dll" -nt "$(obj_of src/dll/dllmain.cpp)" ] || fail=1

    echo "=== link P4GTrainer.exe (injector) ==="
    "$LINK_EXE" -nologo -DEBUG -MACHINE:X64 -SUBSYSTEM:CONSOLE \
        -OUT:"$OUT/P4GTrainer.exe" -PDB:"$OUT/P4GTrainer_exe.pdb" \
        $(objs $INJ_SRC) kernel32.lib user32.lib shell32.lib ole32.lib advapi32.lib || fail=1

    echo "=== link dinput8.dll (game-folder autoloader) ==="
    "$LINK_EXE" -nologo -DLL -DEBUG -OPT:REF -MACHINE:X64         -OUT:"$OUT/dinput8.dll" -PDB:"$OUT/dinput8.pdb"         $(objs $PROXY_SRC) kernel32.lib user32.lib         | grep -v "^ *Creating library" || true
    [ -f "$OUT/dinput8.dll" ] && [ "$OUT/dinput8.dll" -nt "$(obj_of $PROXY_SRC)" ] || fail=1

    echo "=== self-test ==="
    "$LINK_EXE" -nologo -DEBUG -MACHINE:X64 -SUBSYSTEM:CONSOLE -OUT:"$OUT/test/selftest.exe" \
        -PDB:"$OUT/test/selftest.pdb" \
        $(objs src/tests/selftest.cpp src/dll/Engine/SigScan.cpp src/dll/Hooks/Hook.cpp src/dll/Util/Log.cpp $MINHOOK_SRC) \
        kernel32.lib user32.lib > /dev/null || fail=1
    [ $fail -eq 0 ] && { "$OUT/test/selftest.exe" || fail=1; }
fi

echo "=== public-repo check ==="
python -I "$(cygpath -w "$REPO_ROOT/tools/check_public.py")" || echo "(check_public reported findings — fix before committing)"

if [ $fail -ne 0 ]; then
    echo; echo "BUILD FAILED"
    exit 1
fi

# Stage dist/: overwrite the payload; move anything stale into .trash/ (never delete).
DIST="dist"
mkdir -p "$DIST"
KEEP="P4GTrainer.dll P4GTrainer.exe P4GTrainer.pdb dinput8.dll README.md LICENSE"
for f in "$DIST"/*; do
    [ -e "$f" ] || continue
    b=$(basename "$f")
    case " $KEEP " in *" $b "*) ;; *)
        mkdir -p "$REPO_ROOT/.trash/dist-stale"
        mv -f "$f" "$REPO_ROOT/.trash/dist-stale/$b.$(date +%Y%m%d%H%M%S)" ;;
    esac
done
cp -f "$OUT/P4GTrainer.dll" "$OUT/P4GTrainer.exe" "$OUT/P4GTrainer.pdb" "$OUT/dinput8.dll" "$DIST/"
cp -f "$REPO_ROOT/README.md" "$REPO_ROOT/LICENSE" "$DIST/"

echo
echo "=== dist ==="
ls -la "$DIST"
echo "BUILD OK ($BUILD_ID)"
