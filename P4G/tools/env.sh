# Source this to get the x64 MSVC toolchain.
# Nothing is hardcoded: Visual Studio is located with vswhere, the Windows SDK through the
# registry, and the newest installed MSVC / SDK versions are picked.
#   override with:  P4G_VS_DIR=<VS install dir>  P4G_SDK_DIR=<Windows Kits\10 dir>

_p4g_die() { echo "env.sh: $*" >&2; return 1; }

_pf86="$(cygpath -u "${PROGRAMFILES:-}") (x86)"
VSWHERE="$_pf86/Microsoft Visual Studio/Installer/vswhere.exe"

if [ -n "${P4G_VS_DIR:-}" ]; then
    VS="$(cygpath -m "$P4G_VS_DIR")"
else
    [ -x "$VSWHERE" ] || _p4g_die "vswhere.exe not found; install Visual Studio with the C++ workload"
    VS="$("$VSWHERE" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \
           -property installationPath | tr -d '\r')"
    [ -n "$VS" ] || _p4g_die "no Visual Studio with MSVC x64 tools found"
    VS="$(cygpath -m "$VS")"
fi

if [ -n "${P4G_SDK_DIR:-}" ]; then
    SDK_ROOT="$(cygpath -m "$P4G_SDK_DIR")"
else
    SDK_ROOT="$(MSYS2_ARG_CONV_EXCL='*' reg query 'HKLM\SOFTWARE\Microsoft\Windows Kits\Installed Roots' /v KitsRoot10 2>/dev/null \
                | sed -n 's/.*KitsRoot10[[:space:]]*REG_SZ[[:space:]]*//p' | tr -d '\r')"
    [ -n "$SDK_ROOT" ] || _p4g_die "Windows 10/11 SDK not found"
    SDK_ROOT="$(cygpath -m "$SDK_ROOT")"
fi
SDK_ROOT="${SDK_ROOT%/}"

MSVC_VER="$(ls "$VS/VC/Tools/MSVC" | sort -V | tail -1)"
SDK_VER="$(ls "$SDK_ROOT/Include" | grep '^10\.' | sort -V | tail -1)"
MSVC="$VS/VC/Tools/MSVC/$MSVC_VER"

export CL_EXE="$MSVC/bin/Hostx64/x64/cl.exe"
export LINK_EXE="$MSVC/bin/Hostx64/x64/link.exe"
export LIB_EXE="$MSVC/bin/Hostx64/x64/lib.exe"
export RC_EXE="$SDK_ROOT/bin/$SDK_VER/x64/rc.exe"
# cl.exe reads INCLUDE/LIB as Windows-style, semicolon-separated lists
export INCLUDE="$MSVC/include;$SDK_ROOT/Include/$SDK_VER/ucrt;$SDK_ROOT/Include/$SDK_VER/um;$SDK_ROOT/Include/$SDK_VER/shared;$SDK_ROOT/Include/$SDK_VER/winrt"
export LIB="$MSVC/lib/x64;$SDK_ROOT/Lib/$SDK_VER/ucrt/x64;$SDK_ROOT/Lib/$SDK_VER/um/x64"
export PATH="$(cygpath -u "$MSVC/bin/Hostx64/x64"):$PATH"

[ -x "$CL_EXE" ] || _p4g_die "cl.exe missing at the discovered location"
