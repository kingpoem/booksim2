#!/usr/bin/env bash
# Minimal smoke: generate .cfg from v1 JSON specs and run booksim (from repo root).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT/src"
make -s
TMP="$ROOT/.mesh_chiplet_smoke_out"
mkdir -p "$TMP"
python3 "$ROOT/tools/chiplet_spec_to_config.py" "$ROOT/tools/mesh_chiplet_spec_sync_d2d.json" -o "$TMP/sync.cfg"
python3 "$ROOT/tools/chiplet_spec_to_config.py" "$ROOT/tools/mesh_chiplet_spec.sample.json" -o "$TMP/cdc.cfg"
run_bs() {
  local cfg="$1"
  ./booksim "$cfg" >/dev/null
  local rc=$?
  # booksim often exits 255 on normal completion; treat 0 and 255 as success.
  if [ "$rc" -ne 0 ] && [ "$rc" -ne 255 ]; then
    echo "booksim failed (exit $rc): $cfg" >&2
    exit "$rc"
  fi
}
run_bs "$TMP/sync.cfg"
run_bs "$TMP/cdc.cfg"
echo "mesh_chiplet JSON smoke: OK (sync + hetero CDC)"
