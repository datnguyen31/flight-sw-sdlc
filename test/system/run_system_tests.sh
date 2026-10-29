#!/usr/bin/env bash
# Run the system test image (ST-01..ST-10) headless on Renode's LEON3
# platform, then check every scenario from the captured telemetry
# (verification/system-test-spec.md).
#
# Usage: test/system/run_system_tests.sh [path/to/hk_system_test.exe] [virtual-seconds]
#   virtual-seconds defaults to 90: the scenarios end at 84.5 s. A longer
#   run adds a soak for the timing measurement (verification/timing-measured.md),
#   e.g. 300. Renode's memory grows over a run; past about 500 s it was
#   OOM-killed on a 15 GB host.
# Renode is taken from $RENODE, else `renode` on PATH. $RENODE_QUANTUM sets
# Renode's global time quantum in seconds (default: Renode's 0.0001); the
# timing measurement uses 0.00001 so clock reads resolve to 10 us. Extra arguments for
# st_capture.py can be passed in $ST_CAPTURE_ARGS (e.g. "--markdown out.md").
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "${here}/../.." && pwd)"
bin="$(realpath "${1:-${repo}/build-target/test/system/hk_system_test.exe}")"
seconds="${2:-90}"
renode="${RENODE:-renode}"
out_dir="$(dirname "${bin}")"
uart_log="${out_dir}/st_uart.log"

rm -f "${uart_log}"
echo "Renode: running ${bin} for ${seconds} s of virtual time"
"${renode}" --console --disable-gui --plain -e "
\$bin=@${bin}
\$uart_log=@${uart_log}
include @${repo}/tools/renode/hk_leon3.resc
${RENODE_QUANTUM:+emulation SetGlobalQuantum \"${RENODE_QUANTUM}\"}
emulation RunFor \"${seconds}\"
quit" > "${out_dir}/st_renode.log" 2>&1

echo "Target console captured in ${uart_log}"
# shellcheck disable=SC2086
python3 "${here}/st_capture.py" "${uart_log}" ${ST_CAPTURE_ARGS:-}
