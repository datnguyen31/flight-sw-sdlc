# Capstone Defense Response (asynchronous)

| Item | Value |
|---|---|
| Product | Housekeeping Telemetry Service (HK-TM) with the FDIR increment, RTEMS 7, sparc/leon3 on Renode |
| Baseline | branch `session10-closure` / PR #8 (open). Requirements: 31, of which 29 Verified and 2 Partial (HKS-SR-140, HKS-SR-170) |
| Author | Dat Nguyen (developer) |
| Reviewer | Mentor, with the mentor-side ISVV |
| Scope | Written answers only. No functional change. Nothing here changes a verification status. |

Two things I will say once, so they are not repeated in every answer.
First, no requirement is independently checked yet. ISVV items I-1 to I-4
(`verification/isvv-record.md`) belong to the mentor, and their result
columns are empty. Where I say "verified" below, I mean verified by me, the
developer, by the method assigned in the baseline. Second, SYS-3 (OI-4) is a
decision for the reviewer. I give a recommendation in answer 5, not a
disposition.

---

## 1. Requirement-to-evidence traceability

I traced **HKS-FD-030**, the debounce requirement, because it crosses every
level of the life cycle and because it is the requirement where the project
caught its own weak evidence.

| Step | What it says | Where |
|---|---|---|
| Mission need | HKS-SYS-040: the spacecraft responds autonomously to out-of-limit health. A response to a noisy sensor is itself a hazard, so "autonomous" needs a filter. | `requirements/hk-tm-baseline.md`, `fdir-increment.md` |
| Software requirement | HKS-FD-030: confirm a limit condition only after N consecutive out-of-limit samples, N configurable, default 3. Rationale: one noisy sample must not trip a fault response. Method: Test. | `requirements/fdir-increment.md` |
| Design | Each parameter has a confirmed level (NOMINAL, WARNING, CRITICAL), moved by four saturating run counters. Acts only on a rise of level. DD-13. | `design/fdir-design.md` §2, `design-decisions.md` DD-13 |
| Implementation | `hk_monitor_check()` and `HK_DEBOUNCE_N` | `src/fdir/src/hk_monitor.c`, `src/fdir/include/hk_monitor.h` |
| Verification method | Test, at two levels: host unit tests and target system tests | traceability matrix row FD-030 |
| Evidence | Unit: `test_debounce_confirms_on_Nth_sample`, `test_interrupted_run_does_not_confirm`, `test_mixed_soft_and_hard_samples`, `test_recovery_debounced_then_rearmed`. System: ST-07 (event on exactly the 3rd sample, cycle 64), ST-08 (critical on the 3rd sample, cycle 74), ST-10 (1 and N − 1 samples past a hard limit: no event, no safe mode). Check of the checker: code mutant `HK_DEBOUNCE_N = 2` fails ST-07, ST-08 and the extended ST-10. | `test/unit/test_hk_monitor.c`, `verification/system-test-results.md` §1, §3 |
| Final status | **Verified (Test)**. I-2 (independent boundary tests) and I-4 (statement review) pending with the mentor. | traceability matrix |

**Why I consider the evidence sufficient, and where it stops.**

A debounce can be wrong in two directions: too early (N − 1 trips it) or too
late (never, or N + 1). Evidence for only one direction passes a wrong N.
This is exactly what I found as SYS-2. So the evidence is built around both
directions: the Nth-sample test and ST-07/ST-08 show it fires on the third
sample, and the N − 1 tests and ST-10 show it does not fire on the second.
A mutant run on the target shows the test set can actually fail. The unit
tests prove the logic. The system tests prove the same behaviour on the
flight image, read only from emitted telemetry, so the verdict does not rely
on my own instrumentation.

What this evidence does not give me:

- Everything was written by me. A misreading of the requirement would be
  shared by the code and by the tests. That is the reason I-2 exists, and
  until it is executed I would not call the item more than "Verified by the
  developer".
- The tests are written against `HK_DEBOUNCE_N`, but only the default
  N = 3 (and the N = 2 mutant) was run on the target.
- The limit values are illustrative (DD-14), so the evidence is about the
  logic, not about whether 3 samples is the right filter for a real sensor.
  That is a mission and system decision.

For a software unit at this scale I judge the evidence sufficient for the
claim "the debounce logic implements FD-030". I do not judge it sufficient
for "the FDIR configuration is right for a flight mission".

---

## 2. Architecture and the RTEMS task model

**Decision (DD-01, DD-10).** One rate-monotonic collection task runs at the
collection period. A counter divides it down so that every fourth release
also emits a report. The command task is the only other task.

**Why not independent sampling and reporting tasks.** HKS-SR-060 says each
report must contain a complete, consistent sample set. With two tasks, that
property is not automatic. The reporter could read the buffer while the
sampler is half-way through it, or read a set from two different cycles.
Getting it right means a shared buffer, a lock or a double buffer, and a
rule for which sample set is "the" set, plus a way to tie the report
timestamp (HKS-SR-070) to the collection time. With one task and a divider,
the report is built from the cycle that just finished, in the same function
call, so alignment is true by construction, not by synchronization.

**Advantages I got from it.**

- *Alignment by construction.* ST-01 shows the reports at cycles 12, 16 and
  20, with intervals of exactly 4.000000 s.
- *One timing source.* There is one period, one deadline, and one
  worst-case path to analyze. The timing analysis (rev. 3) is a sum along
  one path: 0.99 ms against the 50 ms budget. It is not a schedulability
  problem between tasks.
- *Testability.* Because the per-release pipeline is a plain C module
  (`hk_collection.c`, DD-10), the divider, the enable gate and the sink
  hand-off are tested on the host. The RTEMS task is reduced to the loop
  and the clock.
- *One caller of the sink.* Every report and every FDIR event is emitted
  from the collection task, so the sink needs no locking (fdir-design §5).
  The only shared state is the reporting flag and the safe-mode indicator,
  both behind one priority-inheritance mutex (DD-11).
- *FDIR runs every cycle,* whether reporting is enabled or not, because it
  is attached before the enable gate.

**The price.** Collection and reporting are coupled. A slow report step
delays the next collection, and report rate is an integer multiple of the
collection rate (SR-060 says this). A long sink call blocks sampling, which
is what SYS-4 showed in practice: the console sink cost about 880 µs per
packet.

**When I would reconsider.**

- The real downlink sink blocks for a long time or at unpredictable times
  (a flow-controlled bus, a shared link). Then the worst case on the
  collection path is dominated by something I do not control, and I would
  move reporting to its own lower-priority task fed by a queue, accepting
  the synchronization cost. The timing analysis already notes that a polled
  UART would cost about 26 ms per packet. That is half the cycle budget.
- Parameters that need different sampling rates, for example one at 10 Hz
  and the rest at 1 Hz. One period would then be forced down to the fastest
  rate, or the task would need sub-dividers, and at that point separate
  tasks begin to pay for themselves.
- A mission need for configurable report definitions or several reports at
  different rates (deferred by DD-05). One divider no longer describes that.
- A measured WCET on hardware that leaves too little margin, or a mission
  requirement for a response time (for example in FDIR) shorter than the
  collection period.

The architecture, interfaces and data model are RTOS-agnostic (DD-06). The
change would be in the task layer only.

---

## 3. CCSDS/PUS encoder confidence

The risk with an encoder is **common-mode error**: if the same person reads
the standard once and writes both the encoder and its test, a misreading is
built into both and the test passes. So my evidence is built from sources
that do not share that one reading.

| Evidence | What it shows | What it does not show |
|---|---|---|
| Field-by-field mapping in `design/data-model.md`, inspected against CCSDS 133.0-B and ECSS-E-ST-70-41C (S4, PR #2), then the encoder inspected against the mapping (S7, PR #5). This is the **assigned** method for SR-080 (Inspection). | The format was derived from the standards before the code existed. | Written by me. Reviewed by a mentor in the PRs, but not an independent derivation. |
| `test_encoder_matches_known_vector`, byte-exact, 148 bytes. | Output does not drift from a fixed expected packet. | The vector is mine. If I misread a field, both agree. |
| `test_crc16_ccitt_check_value` | The CRC routine gives the published check value for the standard test string, which comes from outside the project. | It checks the CRC routine, not where the CRC is placed or what it covers. |
| `tools/hk_tm_decode.py`, a decoder written separately, run on **every** packet from the target. It checks packet ID, version, sequence flags and counts, data length, PUS type and subtype, PEC, and values against its own copy of the limits. | Packets from the real flight image, not a unit-test fixture, are parsed by code that shares no source with the encoder. | Still my reading of the standard. It is a second implementation, not a second reader. |
| ST-05 and the global check: seq, packet ID 0x08CA, data length 141, PUS [3,25], CUC time, PEC valid, 16 HK and 4 event packets all contiguous. | The behaviour as a stream, over the target run. | Same. |
| Negative runs: a tampered log with sequence-control bytes zeroed fails the checker. | The checker detects corruption. | |

**How the independent verifier should work differently.** The developer
confirms that the code does what the developer meant. The verifier must
establish that what was meant is what the standard says. So the verifier
should:

1. Work from the standards and `design/data-model.md` and **derive a packet
   by hand**: the primary header bits, the secondary header, the 32
   parameter fields, and the CRC computed independently. That is I-1.
2. Then compare it to a packet the target emitted (for example the
   ST-05 packet in `test/system/st_uart.log`).
3. **Not** use my unit vector or my decoder as the oracle. Reusing them
   makes the check dependent on my reading again. I wrote that exclusion
   into the ISVV record on purpose.
4. Preferably also run the packet through a ground tool they trust that I
   did not write, such as a ground-segment or open-source PUS parser. That
   adds a third independent reading.
5. Try boundaries I might have treated as typical: the maximum sequence
   count (14-bit wrap), a stale marker value (0xFFFFFFFF) in the data field,
   and the length-field "minus one" convention. These are classic
   off-by-one places in CCSDS.

Until I-1 is done, my honest statement is: the encoder is consistent with
the standard as I read it, cross-checked by two implementations of my own,
and independent confirmation is pending.

---

## 4. FDIR debounce design

**The issue with the original approach.** The Session 8 brief sketches one
violation counter per parameter that fires an event when the counter *equals*
N. Implemented as written, it has two problems:

1. **It does not act once.** The counter stops at N. The test "counter == N"
   is therefore true on every later cycle of a sustained fault. The same
   event would be sent again every cycle, and `hk_safemode_enter()` called
   every cycle. That contradicts the brief's own note that the condition is
   "acted on exactly once".
2. **It cannot escalate.** After a soft-limit run has confirmed a warning,
   the counter is already saturated at N. If the value then crosses the hard
   limit, the counter has nothing left to count, and no critical event or
   safe mode ever follows. A warning that turns critical is the most
   important case for FDIR, and a single counter silently loses it.

A single counter also mixes two different questions ("is something out of
limits?" and "which severity is it?") and has no memory of what was already
reported. The fix is to keep **state** (a confirmed level) and to separate
the evidence for each transition.

**Final design (DD-13, fdir-design §2).** Per parameter: a confirmed level
(NOMINAL, WARNING, CRITICAL) and four run counters, each saturating at N so
that none can wrap: `n_warn` (soft or worse), `n_crit` (hard), `n_ok`
(within limits), `n_noncrit` (inside hard limits). Rules are evaluated in a
fixed order and the first match wins. An action happens only when the level
**rises**.

| Case | Behaviour |
|---|---|
| **Confirmation** | N consecutive fresh samples at soft or worse confirm WARNING. N at hard or worse confirm CRITICAL. One limit event is emitted, with the severity of the new level. CRITICAL also enters safe mode. |
| **Escalation warning to critical** | `n_crit` is a separate run, so it keeps counting while the level is already WARNING. N hard samples confirm CRITICAL, with an event and safe mode, even if the warning came first. `test_warning_escalates_to_critical`. |
| **Recovery** | Debounced with the same N. N samples within limits give NOMINAL. From CRITICAL, N samples inside the hard limits give WARNING first. A fall emits no event and re-arms the monitor, so a relapse is reported again. A parameter hovering at a limit does not flap. `test_recovery_debounced_then_rearmed`, `test_critical_steps_down_and_relapse_is_reported`. |
| **Sustained fault** | Reported once, on the transition. The level stays confirmed, so nothing re-fires. `test_sustained_condition_reported_once`. Safe mode entry is also idempotent. |
| **Transient fault** | Any sample that breaks the run resets that counter, so the confirmation never arrives. `test_interrupted_run_does_not_confirm`, ST-10. |
| **Mixed soft and hard** | `n_warn` counts soft and hard together, so N out-of-limit samples of mixed kind still confirm at least a warning. That is the plain reading of FD-030. `test_mixed_soft_and_hard_samples`. |
| **Stale samples** | Neither count toward nor break a limit run. A silent sensor is not evidence that the quantity is out of limits. They have their own counter and event (FD-070). |

**How I showed it does not fire too early.** "Does not fire early" cannot
be proven by only watching it fire. The evidence is:

- **Boundary at N − 1 in both layers.** A unit test with N − 1 samples
  followed by a good sample (interrupted run), and ST-10 on the target with
  1 and N − 1 samples past a **hard** limit: 0 events, no safe mode.
  A hard limit is the strictest case, because it ends in safe mode.
- **Exact firing point.** ST-07 and ST-08 check that the event is on the
  *third* sample (cycles 64 and 74), not earlier and not later.
- **Mutation.** I changed `HK_DEBOUNCE_N` from 3 to 2 and built and ran it
  on the target. The brief's ST-10, one sample, **still passed**. That was
  SYS-2: the spec could not tell N = 2 from N = 3 on a single transient.
  After I extended ST-10 with the N − 1 run, the same mutant failed (two
  events from transients and a spurious safe mode). A separate mutation
  that restored "fire every cycle once saturated" failed five unit tests.
- Independent check still to come: I-2, in which the mentor writes
  boundary tests at N − 1, N and N + 1 without using my tests.

---

## 5. Safe mode

**Why recovery is ground-commanded (HKS-FD-080).** The onboard function sees
a limit violation, but it does not know why it happened or whether the cause
has gone. A value that returns inside limits could mean the fault cleared,
or that the fault is intermittent, or that the spacecraft has just shed the
load that was causing it, which is exactly what safe mode is supposed to do.
If recovery were automatic, the spacecraft could leave safe mode, resume the
activity that caused the fault, trip again, and repeat. That oscillation is
worse than staying safe and waiting for operators, who have context the
software lacks (the whole telemetry history, ground information, the
mission plan). Staying in the safe state costs time, but is the less
harmful failure. The design supports this: no code path in FDIR clears the
indicator (`hk_safemode.c`), and ST-08 checks safe mode **holds** after the
value has returned to nominal.

A corollary is documented in the design. If ground clears safe mode while
the condition persists, safe mode is not re-entered at once, because the
parameter is still CRITICAL with no new rise. The operator has taken
responsibility for that. A relapse after recovery enters safe mode again.

**The gap (SYS-3, OI-4).** Entry emits a TM[5,4] event, but exit emits
nothing, and the indicator is not in the housekeeping report. After
CLEAR_SAFE_MODE, ground only sees the command accepted and, on the console
line, `[SAFE MODE]` disappear. Acceptance of a command is not proof of its
effect: a command can be accepted and lose a race, or the indicator could
fail to clear. For a safety-relevant state that is an operational weakness.

**What I would do, and in what order. Requirement first.**

1. **Requirement.** Write the need: *"On clearing safe mode, the FDIR
   function shall emit a safe-mode-exited event"* (recommended form: an
   informative PUS TM[5,1], parallel to the entry event). Trace it to
   HKS-SYS-040 and to HKS-FD-080, with method Test.
2. Then the design: one new event id, and an owner for emitting it.
3. Then the implementation and the tests, including ST-09 observing the
   event in telemetry instead of the console line.

My reasons for this order:

- HKS-FD-080 is met as written. Changing the code first would be an
  untraced change, and nobody could say what it must do or how to show it
  did it. A requirement gives the verification a target and keeps the
  traceability matrix honest.
- The choice between the two options is a requirements and system decision.
  An exit event leaves the frozen housekeeping structure alone. A status
  parameter changes the fixed structure of HKS-SR-050 and the ground
  decoder, and therefore the ground segment's interface. Whoever owns the
  ground interface should decide, and the reviewer already holds this
  decision (OI-4).
- The need itself should be confirmed with an operator. The validation was
  done by me, and no operator took part. They may want the state in every
  report, not only at the transition.
- The implementation is not trivial, which is another reason to design
  before coding. The command task today emits nothing, because everything
  that sends telemetry runs in the collection task, so the sink has one
  caller. A naive implementation would call the sink from the command task
  and break that property. The safer design is for `CLEAR_SAFE_MODE` to set
  a "cleared" flag under the existing critical section, and for the
  collection task to emit the event on its next release. A dropped event is
  not retried (fdir-design §3), which is a reason that a status parameter in
  the report could still be worth adding later, since it re-announces the
  state every report.

Since this adds a requirement, it also reopens the I-3 and I-4 questions
for the mentor: is the new statement testable from the ground?

---

## 6. Renode versus QEMU

The decision is DD-18, and the reasons are engineering ones, not
preference. I will also state what I did not do: **QEMU was not run on this
image**, so my comparison rests on documented capabilities, not on a
head-to-head test.

**Capabilities that mattered to the verification strategy.**

1. **Reproducible time.** Renode advances virtual time by executed
   instructions at a configured rate (100 MIPS). The same image gives the
   same CUC times, sequence counts and timing record on every run. That
   made the system-test results re-checkable: reruns on 2026-10-29 and
   2026-11-03 reproduced the 2026-10-16 run exactly. By default QEMU ties
   guest time to the host clock. Its `-icount` mode approaches
   determinism, but I did not evaluate it.
2. **Scriptable platform state and hooks.** The `.resc` script sets
   registers and peripheral state before boot. Hooks on guest addresses
   gave evidence without changing the image.
3. **Platform as text.** The machine is a `.repl` description, so adding
   a peripheral (for example the second UART that a queued downlink sink
   would need) is a description edit.
4. **Headless capture.** A file-backed UART and fixed virtual run time let
   the scripts run unattended and feed the host-side checkers. QEMU can do
   this too, so it did not decide the question.

**What it let me automate or observe.**

- The whole system-test run (ten scenarios, one boot, 90 s virtual time),
  and the host-side `st_capture.py` verdicts, with no manual steps.
- A hook on `malloc` as run-time evidence for HKS-SR-160 (no allocation
  after startup).
- Hooks on `bsp_fatal_extension` and `_Memory_Allocate`, which located the
  boot fault that stops before any console output (initial `%sp` left at 0).
- Cycle timing measured by the flight task's own instrumentation (DD-17).

**Disadvantages and limitations.**

- Renode starts the CPU raw, so my script does two jobs a boot loader would
  do (initial `%sp`, and the GPTIMER prescaler: without it ticks run about
  5 times slow). These are real knowledge that I had to find, and they are
  mismatches with real hardware, so any move to hardware must not inherit
  them silently.
- Memory grows during a long run, and Renode was killed after about 530 s
  on my machine, so runs are kept near 300 s or less.
- Clock reads are quantized to the time quantum (100 µs by default), so my
  timing resolution is limited (10 µs when tuned).
- It is a smaller community and I depend on a single tool version
  (1.17.0), whereas QEMU is more widely used and has more experience on
  LEON targets. I have no qualification evidence for either tool.
- Neither emulator models ERC32, the usual ESA-heritage chip. I used LEON3,
  which shares the SPARC V8 CPU and RTEMS API (DD-09).

**Why Renode timing is not flight timing evidence.** Virtual time here is
*instruction count divided by an assumed rate*. That alone makes it
deterministic, so it cannot show jitter: "0 µs jitter over 480 releases"
(SR-140) is a property of the emulator, not of the software. Renode models
no caches, wait states, bus contention, or interrupt latency, and its UART
transmits instantly, which is why the console sink cost (SYS-4) is not the
cost of a real downlink. The 100 MIPS figure is an assumption, not a
measurement of any chip. So I use Renode timing as *indicative* and as a
regression check (an overrun or a change would show). I closed SR-150 by
**analysis** with 98 % margin, and left SR-140 Partial (OI-1) until the
DD-17 record is read on flight-representative LEON3 hardware.

---

## 7. Verification versus validation

**Verification** asks whether I built the product right: does it meet the
written requirements? **Validation** asks whether I built the right
product: do the requirements, taken together, deliver what the mission
needed?

**Concrete example: SYS-3.** Every requirement under HKS-SYS-040 passes
verification. An operator sends CLEAR_SAFE_MODE and receives "accepted",
but cannot see in telemetry that safe mode ended (OI-4). HKS-FD-080 asks for
an indicator, an entry event, and ground-only recovery. It says nothing
about telling ground when recovery worked. Verification checks the product
against the text, and the text had a hole, so verification could not find
it. It was found by asking what an operator needs to know after the command.
ST-09 even showed the symptom: its only observable was a console status
line.

**Why the distinction matters for spacecraft software.** After launch I
cannot attach a debugger. Ground sees only telemetry, and the operator
decides based on it. A requirement set can be perfectly implemented and
still leave the operator blind at the moment it matters, such as after a
safe-mode exit, during a recovery. Passing every test then gives
false confidence. Also, a requirement error is cheap to fix at
specification and expensive after the design and the tests have been built
on it. This is why I treat validation as an activity in its own right and
say plainly that no real operator took part, which is what I would add
first on a real project.

---

## 8. V&V findings

I chose **SYS-2** and **SYS-4**. SYS-3 is covered in answers 5 and 7.

### SYS-2: the brief's ST-10 cannot detect an off-by-one in N

- **How discovered.** Every scenario passed first time, which made me
  suspicious. I checked the checker with a mutant: N changed from 3 to 2.
  ST-07 and ST-08 failed, as expected, but the single-sample ST-10 still
  passed.
- **Type.** A weakness in the **test specification**, not in the code.
- **Why the process did not detect it earlier.** As far as I can tell, the spec was checked for
  coverage ("is there a test for FD-030?"), not for *discriminating power*
  ("can this test tell a correct N from a wrong N?"). A test that passes
  for the right and the wrong implementation looks fine in a traceability
  matrix, because the matrix records that a test exists, not that it can
  fail. ST-07/ST-08 only tested the firing side.
- **Action.** ST-10 was extended with an N − 1 run, and the same mutant
  now fails it. The mutation result is recorded in the system-test results.
- **What I learned.** Test existence is not evidence. For every
  boundary-type requirement I now ask what mutant the test would let
  through. I consider a passing test only as strong as the failing run I
  have seen for it, which is why I kept five tampered-log negative runs too.

### SYS-4: the console sink costs far more than the timing analysis assumed

- **How discovered.** By measuring on the target (DD-17 instrumentation),
  which I compared with the analysis by difference. The output path was
  about 880 µs per HK packet against 220 µs assumed.
- **Type.** An **analysis discrepancy**, not a defect: the total agreed
  (1.30 ms measured against 1.29 ms analyzed, within the ±10 µs
  resolution), but the split did not.
- **Why it was not detected earlier.** The analysis was first written in Session 5,
  before the console sink was measured. It assumed a 50 µs queued handoff, which is
  correct for the flight design but not for the polled-UART stand-in that
  is what actually ran. Analysis alone has no way to find a wrong
  assumption. Only a measurement against it can.
- **Action.** Closed in Session 10 with timing-analysis rev. 3, which
  bounds the sink handoff **per sink**. The flight-configuration bound is
  unchanged and the console stand-in gets its own bound. Rev. 3 also shows
  that this sink would cost about 26 ms per packet on a real UART.
- **What I learned.** "The total agrees" can hide a wrong decomposition,
  and the wrong decomposition then misleads the next decision (here: the
  real downlink). I now compare measurement with analysis term by term,
  and treat an agreeing total with disagreeing parts as a finding.

---

## 9. MISRA and coding-standard closure

**Why a clean cppcheck MISRA result does not prove compliance.**

- **Partial rule coverage.** The cppcheck MISRA addon implements part of
  the MISRA C:2012 rules. A rule it does not implement produces no finding,
  and "no finding" cannot be told apart from "not checked". My gate proves
  that no *checked* Mandatory or Required rule is violated, which is
  weaker than "the code conforms".
- **Undecidable and semantic rules.** Some rules need whole-program or
  semantic analysis (pointer aliasing, value ranges, object lifetimes).
  A heuristic checker approximates them, and can miss real violations or
  raise false ones.
- **The tool is not stable.** The project showed this itself. cppcheck 2.7
  gave 101 Advisory findings and 2.13 gave 54, and 2.13 also raised 8.6 and
  17.3 findings that 2.7 did not. The 17.3 ones appeared only because the
  CI job has no RTEMS headers. A result that moves with the tool version is
  not a proof.
- **Scope of the run.** It covers `src/app` and `src/fdir` as configured
  on the host. The host-only critical-section file is excluded, and the
  target-only code is seen without its headers. Also, the rule *texts* were
  not available to cppcheck 2.7 in my run.
- **Not everything in MISRA is a rule a tool checks.** MISRA C:2012 also
  has Directives, some of which are process (for example, documenting
  implementation-defined behaviour, and the compiler and its use). Compliance also
  needs a **deviation procedure** and records, and a **guideline
  enforcement plan**. The deviation log being empty is a good fact, but it is
  only meaningful if the checking was complete.
- **Mandatory and Required is not everything.** The gate fails on those two
  categories. Advisory rules are adopted "where practical", and I did not
  analyze the 54 to 101 findings one by one.

**Evidence I would require before moving HKS-SR-170 to Verified.**

1. A **rule-by-rule compliance matrix** (in the style of MISRA Compliance:
   2020): for each rule and directive, state whether it is checked by the
   tool, by a second tool, by manual inspection, or not applicable.
   Without it nobody knows which rules were never examined.
2. A **qualified or well-evidenced MISRA checker**, or a second
   independent tool, with the tool's rule coverage stated and its version
   pinned in CI. Run it on the **whole flight image** including the
   target-only code with the real RTEMS headers, so the 17.3 class of
   finding is decided by the tool and not by my argument.
3. **Manual inspection records** for the rules the tools do not
   implement, signed by someone other than the author.
4. **clang-tidy** in CI, as the coding standard (§3) already requires.
   Today the standard asks for a gate that I do not run, which is a gap
   between my own document and my own evidence.
5. A **deviation record** for anything the above finds, each with
   rationale and risk, and an explanation for each remaining Advisory rule
   that I do not follow.
6. A decision on the **third-party code** (the RTEMS headers and the
   configuration macros, `confdefs.h`) and how it is treated, because my
   conformance claim is about my code only.
7. Review by the mentor or the product-assurance role, because
   coding-standard conformance is a PA-owned item in a real project.

Until then, the claim I can support is exactly what the report says:
"the MISRA gate is clean and in CI". It is not "MISRA C:2012 compliant".

---

## 10. Flight readiness

If this became real flight software, the order below is by how much each
item can invalidate everything after it, and how early a change is cheap.
Several items run in parallel, but this is where I would put attention first.

**Priority 1: the flight configuration and hardware that every other
result depends on.**

1. **Flight-representative hardware, mission BSP and hardware
   interfaces.** Everything I measured was on an emulator with a standard
   BSP and a simulated parameter provider. Real drivers, interrupt
   routing, memory map, boot software, watchdog and the real parameter
   sources will change the code, so this comes first. Any evidence gathered
   before this is partly thrown away. Most of my "stub" interfaces
   (DD-03) were built for this moment.
2. **Real telemetry downlink.** The console sink is a stand-in, and the
   timing analysis says a polled UART costs about 26 ms per packet. The
   sink, its flow control and its failure modes decide whether DD-01 holds
   (answer 2). I would do it together with (1).
3. **Timing and WCET evidence.** OI-1: read the DD-17 record on hardware
   with the real sink, then derive WCET properly (measurement with
   hardware cache and interrupt effects, plus analysis), with margin and
   jitter figures. SR-140 stays Partial until then. This depends on 1 and 2.

**Priority 2: the behaviour that protects the spacecraft.**

4. **FDIR and safing at system level.** My safe mode sets an indicator and
   marks a hook (scope boundary). Real safing actions, the system-level
   FDIR hierarchy, the SYS-3 decision, limits from the real mission
   database, the right debounce values per sensor, and watchdog behaviour
   are all system decisions, and they need a failure analysis (FMECA or
   fault tree) with the system engineers. Wrong FDIR can lose a mission
   by itself, so it comes before the formal qualification steps.

**Priority 3: independent assurance, once the code is stable.**

5. **ISVV** (OI-3). It must run on the baseline that will fly, not on
   the emulated one. I would extend the sample beyond I-1 to I-4, because
   an ISVV that finds nothing deserves some suspicion. It would not make
   sense to do this at full cost before 1 to 4 change the code.
6. **Static analysis and coding-standard qualification.** OI-2 and answer 9:
   qualified checker, rule matrix, clang-tidy, and deviation records. This
   is cheap to start early and it should run continuously, but I rank its
   *closure* here because it should be done on the final code.
7. **System and environmental testing.** Hardware-in-the-loop with the
   real avionics, then the campaign that shows the software behaves under
   the environment: power transients, reset and boot-time behaviour, long
   duration runs (my emulator was limited to about 300 s), and radiation
   effects on memory (EDAC, scrubbing, and what the software does on an
   uncorrectable error). Memory and time-related faults appear here and
   not in my tests.

**Throughout, not at the end: configuration management and product
assurance.** I list them last but I would not start them last.
Configuration management (baselines, tool versions pinned,
reproducible builds, controlled change after the baseline) decides whether
any of the evidence above still applies to the thing that flies. PA
(non-conformance process, review records, requirements for the software
criticality category, ECSS-E-ST-40C and Q-ST-80C compliance) decides
whether the evidence is accepted. The **release** is the last step:
a release review that checks every requirement against its evidence on the
final build, every open item and deviation dispositioned, the version
description, a build from clean, and the signatures of the independent
roles. My present report has two Partial items and ISVV pending, so it
would not pass this gate, and it says so.

What I would **not** do: spend effort on polish of the emulator results
before the hardware exists. They are a good regression harness, but they
are not flight evidence.

---

## Final reflection

When I started I thought of the work mostly as getting the code to run on
RTEMS. What changed is that I now think the code is the smallest part of
the evidence for it.

**Requirements.** I used to treat a requirement as an instruction. Now I
treat it as a claim that needs a rationale and a way to fail. The rationale
column earned its place when the debounce design needed it, and SYS-3
showed me that a requirement can be satisfied and still not be enough.

**Architecture.** I now keep the reason for a decision with its
alternatives (DD-01, DD-13, DD-18), and I write down when I would reverse
it. Choosing one task was not "simpler", it was a trade of independence for
alignment that I can now defend, with the conditions that would change it.

**Traceability.** A matrix that lists a test is not evidence. SYS-2
taught me that the test must be able to fail. Traceability is useful when
each link says what it shows and what it does not.

**Fault management.** I used to think of FDIR as detecting faults. Most of
the design effort was in deciding what *not* to do: not fire on one sample,
not fire every cycle, not flap, not recover alone. Fault management is
mostly restraint, and the sketch I was given was wrong in two ways that a
review of the code would have missed but a test of the transitions did not.

**V&V.** I learned to separate "does it meet the text" from "is the text
enough", and to distrust a first run with no failures. The negative runs and
the mutant taught me more than the ten passes.

**QA and ISVV.** I wrote the ISVV plan and could not fill in its results,
and I now understand why that rule matters: my reading of the standard, my
tests and my decoder all share one author. Independence is a property of the
method, not of who sits at the desk.

**Configuration control and release readiness.** I used to think of
release as the moment the tests pass. I now see it as a statement
about what is claimed and what is not. The report says 29 Verified, 2
Partial, none independently checked, and an emulated target. Writing the
open items down, with a closure action and an answer to "does it block
v1.0?", was harder than writing code, and I think it is the part that
would matter most on a real mission.

What I would still want to learn is how this works when the baseline
is not mine, when the requirements come from a mission I did not write and
the ground segment is a different team. That is where I expect validation
to be hardest.
