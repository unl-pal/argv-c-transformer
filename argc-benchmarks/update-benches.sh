#!/bin/bash
set -e

preprocess() {
    local src="$1"
    local dst="$2"
    local arch="$3"   # -m32 or -m64

    local tmp
    tmp=$(mktemp --suffix=.i)

    gcc -E -P -std=gnu11 "$arch" "$src" -o "$tmp"

    # Prepend SPDX header lines from the source file, then the preprocessed body
    {
        grep '^// SPDX-' "$src"
        echo
        cat "$tmp"
    } > "$dst"

    rm "$tmp"
    echo "  $(basename "$src") -> $(basename "$dst") ($arch)"
}

echo "Preprocessing .i files..."
for src in *.c; do
    base="${src%.c}"
    dst="${base}.i"

    # Only regenerate files that have a corresponding .i (not all .c need one)
    [[ -f "$dst" ]] || continue

    # match any ILP32 variants
    if [[ "$base" == *localtime_unsafe* ]]; then
        preprocess "$src" "$dst" "-m32"
    else
        preprocess "$src" "$dst" "-m64"
    fi
done

BENCHMARKS="/home/nat/Repos/bench-defs/sv-benchmarks/c/argv-c/"
mkdir -p "$BENCHMARKS"

echo "Copying to $BENCHMARKS..."
cp -- *.yml *.c *.i README.md Makefile "$BENCHMARKS"
echo "Done."
