#!/usr/bin/env bash
# Package a release of the P4G trainer.
#
#   bash tools/package.sh            # builds release, then writes build/package/*.zip
#
# Output (in P4G/build/package/, which is git-ignored):
#   P4GTrainer-v<version>.zip          Autoload/ (dinput8.dll, P4GTrainer.dll)
#                                      Manual/   (P4GTrainer.exe, P4GTrainer.dll)
#                                      INSTALL.txt, README.md, LICENSE
#   P4GTrainer-v<version>-symbols.zip  the matching .pdb files (for reading crash reports)
#
# Refuses to package uncommitted changes, and checks the binaries for local paths or user names
# before zipping. Never deletes anything.
set -euo pipefail
cd "$(dirname "$0")/.."

if ! git diff --quiet HEAD -- . || [ -n "$(git status --porcelain -- .)" ]; then
    echo "package: commit your changes first (the release build must match a commit)"
    exit 1
fi

bash tools/build.sh release
VERSION="$(tr -d ' \r\n' < VERSION)"
OUT="build/release"
PKG="build/package"
NAME="P4GTrainer-v$VERSION"
mkdir -p "$PKG"

python -I - "$OUT" "$PKG" "$NAME" "$VERSION" "$(git rev-parse --short HEAD)" <<'PYEOF'
import os, re, sys, zipfile

out, pkg, name, version, commit = sys.argv[1:6]
bins = ["P4GTrainer.dll", "P4GTrainer.exe", "dinput8.dll"]

# Nothing machine-specific may ship: drive-letter paths and user-profile paths, ASCII or UTF-16.
bad = re.compile(rb"[A-Za-z]:\\[^\x00]{2,}|\\Users\\[^\\\x00]+")
for b in bins:
    data = open(os.path.join(out, b), "rb").read()
    hits = [m.group() for m in bad.finditer(data)]
    hits += [m.group() for m in bad.finditer(data.decode("utf-16le", "ignore").encode("latin-1", "ignore"))]
    if hits:
        sys.exit(f"package: {b} contains local paths: {hits[:3]}")

install = f"""P4G Trainer v{version}  (build {commit})
For Persona 4 Golden on Steam (64-bit, game version 1.0.5).

Pick ONE way to install.

A. AUTOLOAD - the trainer starts with the game every time
   1. In Steam: right-click Persona 4 Golden > Manage > Browse local files.
   2. Copy both files from the "Autoload" folder (dinput8.dll and P4GTrainer.dll)
      into that folder, next to P4G.exe.
   3. Start the game as usual. A few seconds after the window appears, press
      Insert (or L3 + R3 on a controller) to open the trainer.
   To start the game without it: untick "Load automatically with the game" on the
   trainer's Settings page. To uninstall: remove the two files again.

B. MANUAL - load it only when you want it
   1. Put both files from the "Manual" folder (P4GTrainer.exe and P4GTrainer.dll)
      in any folder, together.
   2. Start the game, then run P4GTrainer.exe (at the title screen or in game).
      "P4GTrainer.exe --wait" waits for the game to start; "--eject" unloads it.

If the game gets an update the trainer doesn't recognise, it stays switched off and
the game runs normally. Your saves are copied to %LOCALAPPDATA%\\P4GTrainer\\backups
the first time the trainer loads in a session; it never writes save files itself.

Problems? Include %LOCALAPPDATA%\\P4GTrainer\\logs\\P4GTrainer.log in your report.
Full documentation: README.md
"""

zpath = os.path.join(pkg, name + ".zip")
with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
    z.write(os.path.join(out, "dinput8.dll"), f"{name}/Autoload/dinput8.dll")
    z.write(os.path.join(out, "P4GTrainer.dll"), f"{name}/Autoload/P4GTrainer.dll")
    z.write(os.path.join(out, "P4GTrainer.exe"), f"{name}/Manual/P4GTrainer.exe")
    z.write(os.path.join(out, "P4GTrainer.dll"), f"{name}/Manual/P4GTrainer.dll")
    z.writestr(f"{name}/INSTALL.txt", install.replace("\n", "\r\n"))
    z.write(os.path.join("..", "README.md"), f"{name}/README.md")
    z.write(os.path.join("..", "LICENSE"), f"{name}/LICENSE")

spath = os.path.join(pkg, name + "-symbols.zip")
with zipfile.ZipFile(spath, "w", zipfile.ZIP_DEFLATED) as z:
    for p in ["P4GTrainer.pdb", "P4GTrainer_exe.pdb", "dinput8.pdb"]:
        z.write(os.path.join(out, p), f"{name}-symbols/{p}")

for p in (zpath, spath):
    print(f"package: {p}  ({os.path.getsize(p) // 1024} KB)")
PYEOF
