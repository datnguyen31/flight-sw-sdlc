#!/usr/bin/env python3
"""MISRA C:2012 gate for the HK-TM flight code (docs/coding-standard.md S1, S3).

Runs cppcheck's MISRA addon over src/app and src/fdir and fails on any
finding against a Mandatory or Required rule. Advisory findings are listed
and counted but do not fail the gate: the standard adopts Advisory rules
"where practical". A Required rule that cannot be met is carried as a
logged deviation in docs/coding-standard.md S1 and suppressed here by
location, never by rule alone.

The addon is run without rule texts (MISRA's text is licensed), so it
cannot report categories itself; ADVISORY below is the MISRA C:2012
Advisory set (with Amendments 1 and 2). Any rule not in it is treated as
Mandatory or Required, so a newer cppcheck that finds more fails safe.

Scope is the flight image. hk_critical_host.c, the host unit-test build's
no-op critical section, is excluded: it and hk_critical_rtems.c define the
same functions and are never linked together (CMake picks one per build),
so analysing both reports a false 8.6 (one external definition).

Rule 17.3 (no implicit declaration) cannot be decided by cppcheck for
files that include <rtems.h> when the RTEMS headers are absent, as in the
host CI job: every RTEMS call then looks undeclared. Those findings are
listed as not decidable here, not passed silently. They are decided by
the target build, where -Wall -Werror (HK_STRICT_WARNINGS) makes an
implicit declaration a build failure.
"""

import pathlib
import subprocess
import sys

ADVISORY = {
    "dir-4.2", "dir-4.4", "dir-4.5", "dir-4.6", "dir-4.8", "dir-4.9", "dir-4.13",
    "1.2", "2.3", "2.4", "2.5", "2.6", "2.7", "4.2", "5.9", "8.7", "8.9",
    "8.11", "8.13", "10.5", "11.4", "11.5", "12.1", "12.3", "12.4", "13.3",
    "13.4", "15.1", "15.4", "15.5", "17.5", "17.8", "18.4", "18.5", "19.2",
    "20.1", "20.5", "20.10", "21.12",
}

# Logged deviations (docs/coding-standard.md S1): (rule, file). Empty at v1.0.
DEVIATIONS = set()

CPPCHECK = [
    "cppcheck", "--addon=misra", "--enable=style", "--quiet",
    "--template={file}:{line}:{id}", "-i", "src/app/src/hk_critical_host.c",
    "-I", "src/app/include", "-I", "src/app/tasks", "-I", "src/app/rtems",
    "-I", "src/fdir/include", "--library=ci/rtems.cfg",
    "src/app", "src/fdir",
]


def includes_rtems(path):
    return "#include <rtems.h>" in pathlib.Path(path).read_text(encoding="utf-8")


def main():
    result = subprocess.run(CPPCHECK, capture_output=True, text=True, check=False)
    advisory = {}
    blocking = []
    undecided = []
    # cppcheck exits 0 when the addon is missing or fails to run, and then
    # reports nothing: that must fail the gate, not pass it.
    if result.returncode != 0 or "addon" in result.stdout.lower():
        print(result.stdout + result.stderr)
        print("MISRA addon did not run")
        return 1
    for line in result.stderr.splitlines():
        path, _, rest = line.partition(":")
        lineno, _, finding = rest.partition(":")
        if finding == "internalError":  # addon did not run: never a pass
            blocking.append(f"{path}: cppcheck internalError, MISRA addon did not run")
            continue
        if not finding.startswith("misra-c2012-"):
            continue
        rule = finding[len("misra-c2012-"):]
        if rule in ADVISORY:
            advisory[rule] = advisory.get(rule, 0) + 1
        elif rule == "17.3" and includes_rtems(path):
            undecided.append(f"{path}:{lineno}")
        elif (rule, path) not in DEVIATIONS:
            blocking.append(f"{path}:{lineno}: MISRA C:2012 {rule} (Mandatory/Required)")

    summary = ", ".join(f"{r} x{n}" for r, n in sorted(advisory.items()))
    print(f"MISRA advisory findings: {sum(advisory.values())} ({summary or 'none'})")
    if undecided:
        print(f"MISRA 17.3 not decidable without RTEMS headers (target build decides): "
              f"{', '.join(undecided)}")
    for item in blocking:
        print(item)
    print(f"MISRA Mandatory/Required findings: {len(blocking)}")
    return 1 if blocking else 0


if __name__ == "__main__":
    sys.exit(main())
