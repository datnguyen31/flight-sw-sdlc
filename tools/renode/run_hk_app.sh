#!/usr/bin/env bash
# Run the HK-TM image headless on Renode's LEON3 platform for a fixed span
# of virtual time, then decode and check the telemetry it produced.
#
# Usage: tools/renode/run_hk_app.sh [path/to/hk_app.exe] [virtual-seconds]
# Renode is taken from $RENODE, else `renode` on PATH.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "${here}/../.." && pwd)"
bin="$(realpath "${1:-${repo}/build-target/src/app/rtems/hk_app.exe}")"
seconds="${2:-27}"
renode="${RENODE:-renode}"
out_dir="$(dirname "${bin}")"
uart_log="${out_dir}/hk_uart.log"

rm -f "${uart_log}"
echo "Renode: running ${bin} for ${seconds} s of virtual time"
"${renode}" --console --disable-gui --plain -e "
\$bin=@${bin}
\$uart_log=@${uart_log}
include @${here}/hk_leon3.resc
emulation RunFor \"${seconds}\"
quit" > "${out_dir}/renode.log" 2>&1

echo "---- target console (${uart_log}) ----"
cat "${uart_log}"
echo "---- ground decode ----"
python3 "${repo}/tools/hk_tm_decode.py" "${uart_log}"
