#!/usr/bin/env bash
# Fetch third-party sources into P4G/third_party/ at pinned commits (git-ignored).
#   Dear ImGui  — MIT        https://github.com/ocornut/imgui
#   MinHook     — BSD-2      https://github.com/TsudaKageyu/minhook
# Already-present checkouts are verified and left alone (nothing is ever deleted).
set -eu
cd "$(dirname "$0")/.."
mkdir -p third_party

fetch() {   # name url tag commit
    local name=$1 url=$2 tag=$3 commit=$4 dir="third_party/$1"
    if [ ! -d "$dir/.git" ]; then
        echo "--- cloning $name $tag"
        git -c advice.detachedHead=false clone --quiet --depth 1 --branch "$tag" "$url" "$dir"
    fi
    local have
    have=$(git -C "$dir" rev-parse HEAD)
    if [ -n "$commit" ] && [ "$have" != "$commit" ]; then
        echo "!!! $name is at $have, expected $commit (tag $tag)." >&2
        echo "    Move third_party/$name aside and run this script again." >&2
        exit 1
    fi
    echo "ok  $name $tag ($have)"
}

fetch imgui   https://github.com/ocornut/imgui.git        v1.91.9b  f5befd2d29e66809cd1110a152e375a7f1981f06
fetch minhook https://github.com/TsudaKageyu/minhook.git  v1.3.4    c3fcafdc10146beb5919319d0683e44e3c30d537
