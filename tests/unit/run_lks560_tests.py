"""Compile and run host regressions without writing build files into the repo."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile
import sys
sys.dont_write_bytecode = True
from check_protection_gates import check as check_protection_gates

parser = argparse.ArgumentParser()
parser.add_argument('--cc', default='gcc', help='Host C99 GCC-compatible compiler')
parser.add_argument('--current-only', action='store_true', help='Run only M3 current sampling Mock')
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
platform = 'firmware/platform/lks32mc071'
board = 'firmware/board/lks_evb_mvpowpre'
bench = 'firmware/tests/bench'
model = 'firmware/motor/commutation'
protection = 'firmware/application/protection'
if not args.current_only:
    check_protection_gates(root)
cases = [
    ('current', ['tests/unit/current_fakes', platform],
     ['tests/unit/test_current_sample.c', 'firmware/motor/current/current_sample.c',
      platform+'/lks_current_sample.c', model+'/sixstep_table.c']),
    ('current-bench', [],
     ['tests/unit/test_bench_current.c', 'firmware/tests/bench_current/bench_current.c',
      'firmware/motor/current/current_sample.c', model+'/sixstep_table.c']),
    ('protection', ['tests/unit', platform, protection],
     ['tests/unit/test_protection_manager.c', 'tests/unit/protection_fake_platform.c',
      protection+'/protection_manager.c']),
    ('table', [model], ['tests/unit/test_sixstep_table.c', model+'/sixstep_table.c']),
    ('bench', ['tests/unit/fakes', bench, platform, model],
     ['tests/unit/test_sixstep_bench.c', bench+'/test_sixstep_logic.c', model+'/sixstep_table.c']),
    ('gate', ['tests/unit/fakes', bench, platform],
     ['tests/unit/test_gate_driver.c', bench+'/test_gate_driver.c']),
    ('lks560', ['tests/unit/lks560_fakes', board, platform, model, bench],
     ['tests/unit/test_lks560_outputs.c', board+'/board_init.c',
      platform+'/bench_sixstep_output.c', platform+'/lks560_pwm_defaults.c',
      model+'/sixstep_table.c'] +
     [bench+'/test_mcpwm_'+name+'.c' for name in ['single', 'complementary', 'three_pair']]),
]
with tempfile.TemporaryDirectory(prefix='lks560-tests-') as temp:
    for name, includes, sources in cases:
        if args.current_only and not name.startswith('current'):
            continue
        exe = str(Path(temp)/(name+'.exe'))
        subprocess.run([args.cc, '-std=c99', '-Wall', '-Wextra', '-Werror']+
                       ['-I'+str(root/p) for p in includes]+
                       [str(root/p) for p in sources]+['-o', exe], check=True)
        if name == 'protection':
            scenarios = subprocess.check_output([exe, '--list'], text=True, timeout=10).splitlines()
            if not scenarios or len(set(scenarios)) != len(scenarios):
                raise RuntimeError('Empty or duplicate Protection Manager scenarios')
            for scenario in scenarios:
                # Fresh OS process for each scenario: no Manager reset seam.
                subprocess.run([exe, scenario], check=True, timeout=10)
            print(f'Protection Manager: {len(scenarios)} isolated scenarios PASS', flush=True)
        else:
            subprocess.run([exe], check=True)
