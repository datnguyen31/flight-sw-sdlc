# HK-TM Measured Timing (HKS-SR-140, HKS-SR-150, HKS-FD-090)

Session 5 bounded the collection cycle by analysis, and Session 8 revised
the bound with FDIR (`verification/timing-analysis.md`, revision 2). This
record measures the cycle on the running system, to corroborate that
analysis. It does not replace it.

## 1. Method

**Instrumentation.** In the flight collection task
(`src/app/tasks/hk_collection_task.c`, DD-17), each release reads the RTEMS
uptime clock (`rtems_clock_get_uptime_nanoseconds()`) on wake-up and again
after `hk_collection_step()`. The window covers the whole per-release work
of the analysis: timestamp read, sampling, build, FDIR monitor and any
events, and on a report tick the encode and sink handoff. The task keeps:

- the best and worst cycle time of a release that sent no report
  (`collect_min`, `collect_max`), and the cycle of the worst;
- the worst cycle time of a release that sent a report (`report_max`) and
  its cycle;
- the worst deviation of a wake-up from its ideal release time
  (`jitter_max`), with ideal releases every 1000 ms from release 2, the
  first one the period timer makes (DD-17).

It prints the record every 60 releases as a `[timing]` line.

**Run.** The system test image (`verification/system-test-spec.md`),
requested for 600 s of virtual time: the ten scenarios in the first 85 s,
then a soak with reporting enabled. The worst cases are therefore taken
over nominal releases, report ticks, and the scenario releases that emit
FDIR events (cycles 54, 64 and 74).

```sh
RENODE=... RENODE_QUANTUM=0.00001 test/system/run_system_tests.sh <hk_system_test.exe> 600
```

**The long runs ended early, on the host.** Renode's own memory grew
steadily during the run, at either time quantum. The host's OOM killer
stopped it at 12.4 GB RSS (15 GB machine): at release 538 with a 10 µs
quantum and at release 527 with the default 100 µs. The guest image's
memory is fixed, so this is a limit of the emulator on this host, not of
the service. The last complete `[timing]` record in both runs is at
**480 releases** (8 minutes of virtual time), and the figures below are
from it. Nothing changed in the record after release 74, the last scenario
release, through to release 480.

| Item | Value |
|---|---|
| Image | `hk_system_test.exe` built from the `vv-v0.1` sources (byte-identical to the one in `system-test-results.md`) |
| Platform | Renode 1.17.0, LEON3 platform, CPU model 100 MIPS, global time quantum 10 µs (`RENODE_QUANTUM`) |
| Clock | RTEMS 1 ms tick; GPTIMER at 1 MHz (`tools/renode/hk_leon3.resc`) |
| Releases | 480 in the last complete record (run stopped by host OOM at release 538), 0 period overruns |

**Resolution.** Renode advances virtual time in quanta, so clock reads are
quantized to the global quantum. At the default 100 µs every figure is a
multiple of 100 µs. The 90 s run in `system-test-results.md` printed
`collect_min 200000 ns, collect_max 400000 ns, report_max 1100000 ns`.
The timing run uses 10 µs, at about the same wall-clock rate.
Figures below are ±10 µs. A second long run at 100 µs gave the same
record within that run's quantization (§2).

## 2. Results

| Quantity | Analysis (S5/S8, rev. 2) | Measured (Renode) | Where | Requirement |
|---|---|---|---|---|
| Collection cycle, nominal (best) | – | **270 µs** | soak | – |
| Collection cycle, worst case | 986 µs (no new condition); 1156 µs with two events | **570 µs** | cycle 74: critical limit event + safe-mode event (ST-08) | SR-150 (< 50 ms) |
| Report cycle, no event | 1206 µs | **1150 µs** | cycle 12 (ST-01) | within the 1000 ms period |
| Report cycle, worst case | 1286 µs with one event | **1300 µs** | cycle 64: report tick + warning event (ST-07) | within the 1000 ms period |
| Period release jitter | about one tick (1 ms), ≪ 400 ms | **0 µs** (at 10 µs resolution), 0 overruns | all 479 timer releases | SR-140 (< 10 %, 400 ms) |

Last complete `[timing]` records of the two long runs:

```
10 µs quantum:  [timing] cycles 480 overruns 0 collect_min 270000 ns collect_max 570000 ns (cycle 74) report_max 1300000 ns (cycle 64) jitter_max 0 ns (cycle 0)
100 µs quantum: [timing] cycles 480 overruns 0 collect_min 200000 ns collect_max 500000 ns (cycle 74) report_max 1300000 ns (cycle 64) jitter_max 0 ns (cycle 0)
```

The two agree on where every worst case falls, and on the values within
the 100 µs run's quantization. The table uses the 10 µs run.

## 3. Comparison with the analysis

**SR-150 is corroborated.** The worst collection cycle measured, 0.57 ms
including two FDIR events, is about 1.1 % of the 50 ms budget. The
heaviest release of any kind, 1.30 ms, is 2.6 % of it.

**The analysis has the right total and the wrong split.** The difference
between release types gives the cost of each part:

| Part | Analysis | Measured (by difference) |
|---|---|---|
| Sample + build + monitor, 32 parameters | 986 µs | about 270 µs (nominal release) |
| One HK packet: encode, PEC, sink | 220 µs (150 + 50 + 20) | about 880 µs (1150 − 270) |
| One event packet: encode, PEC, sink | 80 µs | about 150 µs ((570 − 270) / 2) |

The computational parts come in at about a quarter of their bound. The
output path costs four times its bound for an HK packet and twice for an
event. On the release that has both, a report tick with a warning event,
the measurement is 14 µs (about 1 %) above the analysis figure for that
combination (SYS-4 in `system-test-results.md`).

**Cause.** The analysis bounds the sink handoff at 50 µs, which assumes a
handoff to a downlink queue. The target's sink is the console stand-in
(`hk_tm_sink_console.c`, DD-03): it formats every byte as two hex
characters with `printk` to a polled UART, inside the collection task. Its
cost is proportional to packet length: about 6 µs per byte for both packet
kinds (880 µs / 148 bytes, 150 µs / 24 bytes), and that includes the
bitwise CRC. The measurement does not separate encode from sink. That the
two packet kinds cost the same per byte is consistent with the console
output dominating, but it is an inference, not a measurement.

**Consequence.** No requirement is at risk: the worst release is 1.3 ms
against a 50 ms budget and a 1000 ms period. The analysis needs revision 3
before Session 10's verification report. It should state the sink handoff
assumption per sink, keep the 50 µs figure for a queued downlink, and use a
measured per-byte cost for the console stand-in. Until then, the analysis
remains the evidence for SR-150 for the computational part, and this
measurement corroborates the total. `timing-analysis.md` carries a note
pointing here.

## 4. What emulation can and cannot tell us

The measurement shows the service completes every release with a large
margin, catches any gross regression (the worst release is printed every
minute), and confirms the relative costs of the parts, which is how SYS-4
was found. It is not a hardware measurement:

- Renode's virtual time is instruction count at the CPU model's 100 MIPS.
  It models no caches, pipeline stalls, memory wait states or bus
  contention, and the UART transmits instantly. Absolute microseconds on a
  real LEON3 will differ.
- Jitter is exactly 0 because emulated time is deterministic: the tick
  interrupt arrives at exact virtual times and nothing else competes. It
  says nothing about interrupt latency or contention on hardware.

**Status.** HKS-SR-150: verified by analysis, **corroborated by
measurement under emulation**. HKS-SR-140: **partial**, indicative 0 µs
jitter and 0 overruns in emulation. Both are marked for final measurement
on flight-representative hardware, with the instrumentation already in the
flight build (DD-17). HKS-FD-090 timing half: corroborated; the FDIR events
are inside every measured release.
