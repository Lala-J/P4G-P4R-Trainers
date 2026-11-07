#!/usr/bin/env python3
"""Fail if anything private, machine-specific or copyrighted is about to be published.

This repository is public. The checks below catch the usual accidents before they reach
GitHub: absolute local paths, the current Windows user name, Steam account folders, email
addresses, API keys and tokens, and game assets or other binaries.

    python tools/check_public.py            # scan every tracked + untracked (non-ignored) file
    python tools/check_public.py --staged   # scan what is staged (used by .githooks/pre-commit)

Exit code 0 = clean, 1 = findings (printed as path:line: reason).
A deliberate exception goes in .public-allow (one 'path-glob' or 'path-glob:regex' per line).
"""
import fnmatch
import os
import re
import subprocess
import sys

ROOT = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True,
                      text=True, check=True).stdout.strip()

# Binary or asset types that never belong here (game files, textures, build output).
BLOCKED_EXT = {".exe", ".dll", ".pdb", ".lib", ".obj", ".cpk", ".bin", ".tmx", ".dds",
               ".gxt", ".pak", ".pac", ".bf", ".bmd", ".acb", ".awb", ".usm", ".ct",
               ".7z", ".zip", ".rar"}
MAX_BYTES = 512 * 1024

ALLOWED_EMAILS = {"noreply@anthropic.com", "noreply@github.com"}

def _rx(*parts):
    # Patterns are assembled from pieces so this file never matches itself.
    return re.compile("".join(parts))

RULES = [
    (_rx(r"(?<![A-Za-z0-9])[A-Za-z]", r":(\\\\|\\|/)(?!/)"), "absolute drive path"),
    (_rx(r"(?i)[\\/]", r"users[\\/][^\\/\s\"'`<>]+"), "user profile path"),
    (_rx(r"(?i)userdata[\\/]", r"\d{3,}"), "Steam account folder"),
    (_rx(r"\b7656119", r"\d{10}\b"), "SteamID64"),
    (_rx(r"[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.", r"[A-Za-z]{2,}"), "email address"),
    (_rx(r"\b(sk-[A-Za-z0-9_-]{20,}|gh[pousr]", r"_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{20,})"), "API token"),
    (_rx(r"\bAKIA", r"[0-9A-Z]{16}\b"), "AWS key"),
    (_rx(r"\bAIza", r"[0-9A-Za-z_-]{35}\b"), "Google API key"),
    (_rx(r"\bxox[baprs]", r"-[A-Za-z0-9-]{10,}"), "Slack token"),
    (_rx(r"-----BEGIN [A-Z ]*PRIV", r"ATE KEY-----"), "private key"),
]

def local_identity_words():
    """Names that identify this machine's user. Read at run time, never stored in the repo."""
    words = set()
    for var in ("USERNAME", "USER"):
        v = os.environ.get(var, "")
        if len(v) >= 4:
            words.add(v.lower())
    prof = os.environ.get("USERPROFILE", "")
    if prof:
        base = os.path.basename(prof.rstrip("\\/"))
        if len(base) >= 4:
            words.add(base.lower())
    words.discard("root")
    return words

def load_allow():
    allow = []
    path = os.path.join(ROOT, ".public-allow")
    if os.path.exists(path):
        for line in open(path, encoding="utf-8"):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            glob, _, rx = line.partition(":")
            allow.append((glob, re.compile(rx) if rx else None))
    return allow

def allowed(allow, path, text):
    for glob, rx in allow:
        if fnmatch.fnmatch(path, glob) and (rx is None or rx.search(text)):
            return True
    return False

def git_lines(*args):
    out = subprocess.run(["git", *args], cwd=ROOT, capture_output=True, check=True).stdout
    return [p for p in out.decode("utf-8", "replace").split("\0") if p]

def read_file(path, staged):
    if staged:
        r = subprocess.run(["git", "show", ":" + path], cwd=ROOT, capture_output=True)
        return r.stdout if r.returncode == 0 else None
    full = os.path.join(ROOT, path)
    if not os.path.isfile(full):
        return None
    with open(full, "rb") as f:
        return f.read()

def main():
    staged = "--staged" in sys.argv
    if staged:
        files = git_lines("diff", "--cached", "--name-only", "--diff-filter=ACMR", "-z")
    else:
        files = git_lines("ls-files", "-co", "--exclude-standard", "-z")
    allow = load_allow()
    idwords = local_identity_words()
    findings = []

    for path in sorted(set(files)):
        if path.startswith(".trash/"):
            findings.append((path, 0, ".trash/ must never be committed"))
            continue
        data = read_file(path, staged)
        if data is None:
            continue
        ext = os.path.splitext(path)[1].lower()
        if ext in BLOCKED_EXT and not allowed(allow, path, ""):
            findings.append((path, 0, f"blocked file type {ext} (binary/game asset)"))
            continue
        if len(data) > MAX_BYTES and not allowed(allow, path, ""):
            findings.append((path, 0, f"large file ({len(data)} bytes)"))
            continue
        if b"\0" in data[:8192]:
            if not allowed(allow, path, ""):
                findings.append((path, 0, "binary content"))
            continue
        text = data.decode("utf-8", "replace")
        for n, line in enumerate(text.splitlines(), 1):
            low = line.lower()
            for word in idwords:
                if word in low and not allowed(allow, path, line):
                    findings.append((path, n, "contains the local Windows user name"))
            for rx, why in RULES:
                for m in rx.finditer(line):
                    hit = m.group(0)
                    if why == "email address" and (hit.lower() in ALLOWED_EMAILS
                                                   or hit.lower().endswith("@users.noreply.github.com")):
                        continue
                    if allowed(allow, path, line):
                        continue
                    findings.append((path, n, f"{why}: {hit[:60]}"))

    for path, n, why in findings:
        print(f"{path}:{n}: {why}")
    if findings:
        print(f"\ncheck_public: {len(findings)} finding(s). Fix them, or add a justified entry "
              f"to .public-allow.")
        return 1
    print(f"check_public: {len(files)} file(s) clean.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
