#!/usr/bin/env bash
#
# mayhem/test.sh — RUNS the known-answer test binaries that mayhem/build.sh produced (mayhem/build/kat_*).
# Each one links the real vkQuake parser sources (JSON, console text, PNG/TGA/JPG, PCX/LMP, WAD, WAV)
# and asserts parsed values for known inputs; it prints `KAT <name> passed=N failed=M`. A binary that
# prints no such line, or fewer passes than expected, fails — so a program neutered to exit(0) cannot pass.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# <binary suffix>:<minimum number of passed assertions>
KATS="json:23 text:30 image:13 pcx:11 wad:9 wav:9"

passed=0
failed=0
for spec in $KATS; do
  name="${spec%%:*}"; min="${spec##*:}"
  bin="$SRC/mayhem/build/kat_$name"
  if [ ! -x "$bin" ]; then
    echo "missing known-answer binary $bin (build.sh should have produced it)" >&2
    failed=$((failed + min))
    continue
  fi
  out="$("$bin" 2>&1)"; rc=$?
  line="$(printf '%s\n' "$out" | grep -E "^KAT $name passed=[0-9]+ failed=[0-9]+$" | tail -n 1)"
  if [ -z "$line" ]; then
    echo "kat_$name printed no result line (rc=$rc)" >&2
    printf '%s\n' "$out" >&2
    failed=$((failed + min))
    continue
  fi
  p="${line#*passed=}"; p="${p%% *}"
  f="${line##*failed=}"
  echo "$line"
  passed=$((passed + p))
  failed=$((failed + f))
  if [ "$p" -lt "$min" ]; then
    echo "kat_$name passed only $p assertions, expected at least $min" >&2
    failed=$((failed + min - p))
  elif [ "$rc" -ne 0 ] && [ "$f" -eq 0 ]; then
    failed=$((failed + 1))
  fi
done

emit_ctrf "vkquake-kat" "$passed" "$failed" 0
