# A4 host tests

Run from repository root with a host C99 compiler (GCC example):

```sh
gcc -std=c99 -Wall -Wextra -Werror -Ifirmware/motor/commutation tests/unit/test_sixstep_table.c firmware/motor/commutation/sixstep_table.c -o test_sixstep_table
./test_sixstep_table
gcc -std=c99 -Wall -Wextra -Werror -Itests/unit/fakes -Ifirmware/tests/bench -Ifirmware/platform/lks32mc071 -Ifirmware/motor/commutation tests/unit/test_sixstep_bench.c firmware/tests/bench/test_sixstep_logic.c firmware/motor/commutation/sixstep_table.c -o test_sixstep_bench
./test_sixstep_bench
```

The bench test executes the real polling controller with a fake timebase, buttons and output backend. It does not validate the MCPWM registers or physical output timing. Never add the fake SDK header directory to the firmware project.
