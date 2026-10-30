#!/usr/bin/env bash
#
# mayhem/build.sh — builds the vkQuake fuzz harnesses and the known-answer test binaries.
#
# vkQuake normally needs meson, SDL3, Vulkan and glslang. The parsers fuzzed here (JSON, console text,
# image/WAD/WAV loaders) only need the engine's own sources plus a handful of engine-service stubs, and
# the in-tree SDL3/Vulkan headers are enough to compile them. So the engine files are compiled directly
# with clang, once with the sanitizers (fuzz binaries) and once with normal flags (known-answer tests).
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}"
: "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
export SANITIZER_FLAGS DEBUG_FLAGS CC LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

Q="$SRC/Quake"
M="$SRC/mayhem"
B="$M/build"
mkdir -p "$B/san" "$B/plain"

COMMON_FLAGS="-DUSE_SDL3 -DUSE_CRT_MALLOC -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
  -I$SRC/Windows/SDL3/include -I$SRC/Windows/misc/include -I$Q
  -Wno-trigraphs -w -ffunction-sections -fdata-sections"

ENGINE_SRCS="common cmd json mem strlcpy strlcat image wad snd_mem"
SUPPORT_SRCS="stubs mockfs"

# harness list: <name> <class> where class A links the real common.c/cmd.c/mem.c and class B the file mocks
HARNESSES="fuzz_json:A fuzz_text:A fuzz_image:B fuzz_pcx:B fuzz_wad:B fuzz_wav:B"

objs_for() {
  local dir="$1" name="$2" cls="$3"
  case "$name" in
    fuzz_json) echo "$dir/json.o $dir/common.o $dir/mem.o $dir/strlcpy.o $dir/strlcat.o $dir/stubs.o" ;;
    fuzz_text) echo "$dir/common.o $dir/cmd.o $dir/mem.o $dir/strlcpy.o $dir/strlcat.o $dir/stubs.o" ;;
    fuzz_image|fuzz_pcx) echo "$dir/image.o $dir/mockfs.o $dir/stubs.o" ;;
    fuzz_wad) echo "$dir/wad.o $dir/mockfs.o $dir/stubs.o" ;;
    fuzz_wav) echo "$dir/snd_mem.o $dir/mockfs.o $dir/stubs.o" ;;
  esac
}

# Compile every engine/support source in parallel into <outdir> with the given flags.
build_objects_checked() {
  local out="$1"; shift
  local pids=() f
  for f in $ENGINE_SRCS; do
    "$CC" "$@" $COMMON_FLAGS -c "$Q/$f.c" -o "$out/$f.o" & pids+=($!)
  done
  for f in $SUPPORT_SRCS; do
    "$CC" "$@" $COMMON_FLAGS -c "$M/$f.c" -o "$out/$f.o" & pids+=($!)
  done
  local rc=0 p
  for p in "${pids[@]}"; do wait "$p" || rc=1; done
  return $rc
}

# Linking uses lld: with GNU ld, the coverage-counter sections keep every function alive through
# --gc-sections, which would drag the whole SDL/Vulkan-facing part of common.c into the link.
# 1) Sanitized engine objects + fuzz binaries. The engine objects carry libFuzzer's coverage
#    instrumentation (fuzzer-no-link) so the fuzzer sees edges inside the parsers, not just the harness.
COV_FLAGS="-fsanitize=fuzzer-no-link"
build_objects_checked "$B/san" $SANITIZER_FLAGS $COV_FLAGS $DEBUG_FLAGS -O1
"$CC" $SANITIZER_FLAGS $DEBUG_FLAGS -O1 -c "$M/lsan_off.c" -o "$B/san/lsan_off.o"

for hc in $HARNESSES; do
  name="${hc%%:*}"; cls="${hc##*:}"
  hobj="$B/san/$name.o"
  "$CC" $SANITIZER_FLAGS $COV_FLAGS $DEBUG_FLAGS -O1 $COMMON_FLAGS -I"$M/harnesses" -c "$M/harnesses/$name.c" -o "$hobj"
  objs="$(objs_for "$B/san" "$name" "$cls")"
  if [ "$name" = fuzz_wav ]; then
    # file-input target: Sys_Error is fatal in the engine, so this one is not an in-process libFuzzer harness
    "$CC" $SANITIZER_FLAGS $DEBUG_FLAGS "$hobj" $objs "$B/san/lsan_off.o" -fuse-ld=lld -Wl,--gc-sections -lm -o "$SRC/$name"
  else
    "$CC" $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$hobj" $objs "$B/san/lsan_off.o" -fuse-ld=lld -Wl,--gc-sections -lm -o "$SRC/$name"
    "$CC" $SANITIZER_FLAGS $DEBUG_FLAGS "$STANDALONE_FUZZ_MAIN" "$hobj" $objs "$B/san/lsan_off.o" -fuse-ld=lld -Wl,--gc-sections -lm -o "$SRC/$name-standalone"
  fi
done

# 2) Known-answer test binaries: the same engine code with the project's normal flags, no sanitizers.
build_objects_checked "$B/plain" -O2 $COVERAGE_FLAGS
for hc in $HARNESSES; do
  name="${hc%%:*}"; cls="${hc##*:}"
  "$CC" -O2 $COVERAGE_FLAGS -DMAYHEM_KAT $COMMON_FLAGS -I"$M/harnesses" -c "$M/harnesses/$name.c" -o "$B/plain/$name.kat.o"
  objs="$(objs_for "$B/plain" "$name" "$cls")"
  "$CC" -O2 $COVERAGE_FLAGS "$B/plain/$name.kat.o" $objs -fuse-ld=lld -Wl,--gc-sections -lm -o "$B/kat_${name#fuzz_}"
done
