#!/bin/bash
set -e

# Must match the glibc used by the SV-Comp preprocessing-consistency CI image.
PREPROCESS_IMAGE="registry.gitlab.com/sosy-lab/benchmarking/sv-benchmarks/ci/preprocessing-consistency:latest"

echo "Preprocessing .i files inside ${PREPROCESS_IMAGE}..."
docker run --rm --user "$(id -u):$(id -g)" \
    -v "$(pwd):/work" \
    "$PREPROCESS_IMAGE" \
    bash -c '
set -e
cd /work
for src in *.c; do
    base="${src%.c}"
    dst="${base}.i"
    [[ -f "$dst" ]] || continue
    tmp=$(mktemp --suffix=.i)
    gcc -E -P -std=gnu11 -m64 "$src" -o "$tmp"
    { grep "^// SPDX-" "$src"; echo; cat "$tmp"; } > "$dst"
    rm "$tmp"
    echo "  ${src} -> ${dst}"
done
'

BENCHMARKS="/home/nat/Repos/bench-defs/sv-benchmarks/c/argv-c/"
mkdir -p "$BENCHMARKS"

echo "Copying to $BENCHMARKS..."
cp -- *.yml *.c *.i README.md Makefile "$BENCHMARKS"
echo "Done."
