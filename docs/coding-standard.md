# Coding Standard

This closes HKS-SR-170. It is short on purpose: a standard that is read and
followed beats an exhaustive one that is not. Verification method is
Inspection, enforced automatically in the CI pipeline built in Session 6.

## 1. Basis and deviations

The standard is the MISRA C:2012 Mandatory and Required rule set, applied
to C11. Advisory rules are adopted where practical but are not mandatory.

Any deviation from a Mandatory or Required rule must be documented in this
file with the rule number, the location it applies to, the rationale, and a
reviewer sign-off, consistent with ECSS-Q-ST-80C product assurance
practice. A deviation without a recorded rationale and sign-off is a
finding at review, not an allowance.

| Rule         | Location | Rationale | Reviewer |
| ------------ | -------- | --------- | -------- |
| _(none yet)_ |          |           |          |

## 2. House rules

Ten rules of thumb, adapted from Gerard Holzmann's JPL "Power of Ten" rules
for safety-critical C (*IEEE Computer*, 2006). These supplement MISRA where
the project wants a specific, checkable convention; numbering follows the
original.

1. **Simple control flow.** No `goto`, no `setjmp`/`longjmp`, no direct or
   indirect recursion. The call graph is acyclic and can be drawn as a
   tree.
2. **Fixed loop bounds.** Every loop has a statically provable upper bound
   on iterations (e.g. iterate over the fixed parameter table with a
   compile-time count, never an open-ended condition). A bound a tool
   cannot prove statically is a violation, not a judgment call.
3. **No dynamic memory after initialization.** No `malloc` or `free` in
   flight code once startup completes (HKS-SR-160, DD-02). All buffers,
   including the report buffer, are sized at build time.
4. **Short, single-purpose functions.** A function's body fits on one
   printed page (a guideline of ~60 lines, one statement or declaration
   per line) and does one thing; cyclomatic complexity above 10 is a
   trigger to decompose it, not a hard failure on its own.
5. **Assert defensively.** Average at least two assertions per function,
   checking preconditions, postconditions, and invariants that should
   never be false in a correct execution. Assertions are side-effect-free
   boolean checks; a failing one triggers an explicit, defined recovery
   action. Flight code does not use the standard `assert()` macro as-is
   (its default is `abort()`) but a project `HK_ASSERT` wrapper that logs
   and hands off to a defined safe state.
6. **Smallest scope.** Declare every data object at the smallest scope
   that can hold it — block-local before function-static before
   file-static before global. The reporting-state flag shared between the
   collection and command tasks (`design/detailed-design.md` §1.6) is the
   exception that proves the rule: it is shared because two tasks
   genuinely need it, not by default.
7. **Check every return value.** The caller checks the return value of
   every non-`void` function (e.g. `hk_param_source_read()`'s status,
   `hk_tm_sink_send()`'s success flag), and every function validates the
   parameters it receives. An intentionally-ignored return value is cast
   to `(void)` with a comment saying why.
8. **Restrained preprocessor use.** The preprocessor is for header
   inclusion and simple, single-expansion macro definitions. No token
   pasting, no variadic macros, no recursive macro expansion; every macro
   expands to one complete syntactic unit. Conditional compilation is
   limited to include guards and build-target selection (RTEMS vs. test
   stubs), never to silently changing behaviour.
9. **Restrained pointer use.** At most one level of dereference; no
   pointer dereference hidden inside a macro or a `typedef`; no function
   pointers. Swappable implementations (DD-03 — parameter sources, the
   telemetry sink) are selected by linking a different `.c` file per
   build target, not through function-pointer indirection.
10. **Compile and analyze clean, from day one.** All code builds from the
    first commit with every compiler warning enabled at its most pedantic
    setting, and is checked daily by static analysis. See Section 3 for
    the exact flags and tools; a warning is a defect, not noise to
    suppress.

### Project conventions beyond the JPL ten

- Fixed-width integer types from `stdint.h`; no reliance on plain `char`
  signedness for arithmetic.
- One module is one `.h` and one `.c`; headers declare, they do not define;
  every header carries an include guard.
- Each module and public function names, in a comment, the requirement
  IDs it implements, so a reader can trace code to requirement without
  leaving the file. (See the existing interface headers under
  `src/app/include/` for the pattern this follows.)

## 3. Enforcement

The standard is machine-checked, not just written down. The build and CI
apply:

```
# compiler, treated as the first line of enforcement
-std=c11 -Wall -Wextra -Werror -Wconversion -Wshadow -Wpedantic

# static analysis in CI (Session 6)
cppcheck --enable=all --addon=misra ...
clang-tidy ...
```

Flight code must build and analyze clean, or carry a logged deviation per
Section 1. These gates are wired into the pipeline in Session 6, which is
also where the unit-test harness is properly enabled.
