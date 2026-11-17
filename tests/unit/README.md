# A4 host tests

Run from repository root with a host C99 compiler (GCC example):

```sh
gcc -std=c99 -Wall -Wextra -Werror -Ifirmware/motor/commutation tests/unit/test_sixstep_table.c firmware/motor/commutation/sixstep_table.c -o test_sixstep_table
./test_sixstep_table
gcc -std=c99 -Wall -Wextra -Werror -Itests/unit/fakes -Ifirmware/tests/bench -Ifirmware/platform/lks32mc071 -Ifirmware/motor/commutation tests/unit/test_sixstep_bench.c firmware/tests/bench/test_sixstep_logic.c firmware/motor/commutation/sixstep_table.c -o test_sixstep_bench
./test_sixstep_bench
```

The bench test executes the real polling controller with a fake timebase, buttons and output backend. It does not validate the MCPWM registers or physical output timing. Never add the fake SDK header directory to the firmware project.

## LKS560 polarity regression

Run `python tests/unit/run_lks560_tests.py --cc <gcc-path>` to run all host suites, including the actual A0-A4 physical-output sources against the host register model. It checks HIN=0/LIN=1 idle/STOP, sector-to-driver truth mapping, physical N inversion in A1-A3, and CH_DEF bypassing that inversion. No fake headers belong in firmware include paths. Hardware VGS, deadtime and reset-high-impedance are outside this model.

## A5 tests

The same `run_lks560_tests.py` also runs the real A5 polling controller against fake buttons/time/output. It covers default OFF, all phases, finite low hold and precharge/gap/high-packet ordering, STOP in every state, ignored repeated RUN, configuration changes, missing bootstrap selection, illegal parameters/states, backend rejection, late polling and time wrap. Backend tests also check one-phase-only internal requests and physical polarity; neither suite measures actual VBS, VGS or analog timing.

## V02-01 Protection Manager Mock regression

Baseline reviewed: `493ab126082c3c3fc38500d58c57c49877ecc395` (`main`).
The task's D-drive checkout was absent on this workstation; the existing
`F:/Demo/sensorless-sixstep-bldc` checkout matched that exact commit. No branch
switch, pull, reset, commit or push is part of this work package.

```text
run_lks560_tests.py (C99, -Wall -Wextra -Werror; TemporaryDirectory)
  +-- check_protection_gates.py: read-only source guard, NO MCU execution
  +-- protection.exe --list
  +-- protection.exe <one scenario> [fresh process for EACH of 45 scenarios]
        +-- REAL firmware/application/protection/protection_manager.c
        +-- REAL protection_manager.h / lks_protection.h interfaces
        +-- protection_fake_platform.c (all six platform entry points)
              controlled snapshots/results/asynchronous FAIL injection
              saved/restored PRIMASK + checked nesting stack
  +-- original table / A4 bench / A5 gate / LKS560 output regressions
```

No real `lks_protection.c`, SDK or MCU register model is linked into the Manager
executable. `Fake_Qualified()` supplies a hypothetical qualified platform ONLY
inside host processes; it cannot qualify or unblock the firmware. There is no
production Reset API: every scenario begins with the actual Manager's static
BOOT_SAFE state in a new OS process. Tests fail noninteractively via `CHECK`,
with file/line and a nonzero exit, even if NDEBUG is supplied.

Run from the repository root:

```powershell
python tests/unit/run_lks560_tests.py --cc 'E:\embedded_general_software\mingw_w64\mingw64\bin\gcc.exe'
```

A GCC-compatible compiler on PATH also works with the default `--cc gcc`.
Compiled executables are temporary and deleted by the runner. The runner does
not generate Python import bytecode in the repository. Each Manager scenario
has a 10-second process timeout. Unknown/missing scenarios return nonzero.

| Requirement | Scenarios / assertions |
|---|---|
| SW-01 | Boot EIF survives simulated SDK clearing; first_fault is a later snapshot; RUN refused |
| SW-02 | Actual blocked configuration mirror plus separate invalid RAW/config/readability/blocker cases; no enable request |
| SW-03 | Explicit RUN/STOP; repeat RUN rejected without another MOE enable; no latch |
| SW-04 | Valid RAW fault closes MOE and saves pre-shutdown evidence |
| SW-05 | FAIL1 EIF fault with healthy RAW locks out RUN |
| SW-06 | RAW / EIF / software-latched fault independently reject RUN without SetMoe(1) |
| SW-07 | FAIL injected during SetMoe(1), even under PRIMASK=1; post-check latches; RAW-only and EIF-with-MOE-high variants |
| SW-08 | Source withdrawal, 32 Process calls, STOP and RUN do not clear or restart |
| SW-09 | Invalid RAW, active RAW, MOE high, DISARMED/RUNNING, unreadable peripheral refuse CLEAR without ClearFail |
| SW-10 | Explicit CLEAR rereads source/EIF/MOE/config; never enables; separate RUN required; failed clear/reassertion/qualification loss keep latch |
| Additional | Reinit preserves latch; failed/partial/no-write enable; visible failed shutdown; unexpected MOE; first snapshot preservation despite reason update; NULL info; nested/already-disabled PRIMASK |

### Manual review and interpretation

- `inspect()` is NOT a read-only query: faults, disqualification or inconsistent
  MOE invoke `latch()`. Init, RUN, STOP and Process use that behavior; CLEAR reads
  preconditions first so refused CLEAR does not mutate existing latch information.
- Boot evidence is captured before SDK initialization clears EIF; first_fault
  records the first locking observation of the current fault episode. Repeated
  fault observations while latched cannot overwrite it; reason may change to
  IO_ERROR. Explicit CLEAR clears latched, so a subsequent new episode can record
  a new first_fault. Permanent multi-episode history is not this interface's
  guarantee. `reason` can remain historical when `latched=0`.
- RUN's post-enable inspect catches the specified asynchronous injected FAIL,
  and requests shutdown. It does NOT prove there is no transient re-enable
  window on real silicon. PRIMASK serializes CPU requests, not hardware FAIL.
- CLEAR invokes the platform only from FAULT_LATCHED with readable peripheral,
  valid withdrawn RAW and MOE=0; post-clear inspect verifies RAW, EIF, MOE and
  configuration. Successful CLEAR returns DISARMED without enabling MOE.
- Failed MOE shutdown remains visible as IO_ERROR. Software latch cannot force
  a failed physical gate off; independent hardware shutdown is still required.
- Real platform/bench reviewed read-only. Their source gates and safe defaults
  are checked by `check_protection_gates.py`, not executed or hardware-qualified.
  Current config_valid/raw_valid/hardware_qualified remain 0, FAIL1_EN remains
  disabled, RUN/MOE enable/CLEAR remain BLOCKED, and P0-P4 remain NOT RUN.

No production logic modification was necessary for the covered scenarios.
No Keil project, main, A5 backend, SDK/HAL, real platform or bench file is changed.
A full result report, captured console output and diff are delivered outside the
repository for review; no Git submission or hardware activity is performed.
