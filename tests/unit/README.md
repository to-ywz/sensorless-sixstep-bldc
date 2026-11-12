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

Run `python tests/unit/run_lks560_tests.py --cc <gcc-path>` to run all three suites, including the actual A0-A4 physical-output sources against the host register model. It checks HIN=0/LIN=1 idle/STOP, sector-to-driver truth mapping, physical N inversion in A1-A3, and CH_DEF bypassing that inversion. No fake headers belong in firmware include paths. Hardware VGS, deadtime and reset-high-impedance are outside this model.

## A5 tests

The same `run_lks560_tests.py` also runs the real A5 polling controller against fake buttons/time/output. It covers default OFF, all phases, finite low hold and precharge/gap/high-packet ordering, STOP in every state, ignored repeated RUN, configuration changes, missing bootstrap selection, illegal parameters/states, backend rejection, late polling and time wrap. Backend tests also check one-phase-only internal requests and physical polarity; neither suite measures actual VBS, VGS or analog timing.
