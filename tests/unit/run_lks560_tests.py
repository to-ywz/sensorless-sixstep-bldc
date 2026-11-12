"""Compile and run host regressions without writing build files into the repo."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--cc', default='gcc', help='Host C99 GCC-compatible compiler')
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
platform = 'firmware/platform/lks32mc071'
board = 'firmware/board/lks_evb_mvpowpre'
bench = 'firmware/tests/bench'
model = 'firmware/motor/commutation'
cases = [
    ('table', [model], ['tests/unit/test_sixstep_table.c', model+'/sixstep_table.c']),
    ('bench', ['tests/unit/fakes', bench, platform, model],
     ['tests/unit/test_sixstep_bench.c', bench+'/test_sixstep_logic.c', model+'/sixstep_table.c']),
    ('lks560', ['tests/unit/lks560_fakes', board, platform, model, bench],
     ['tests/unit/test_lks560_outputs.c', board+'/board_init.c',
      platform+'/bench_sixstep_output.c', platform+'/lks560_pwm_defaults.c',
      model+'/sixstep_table.c'] +
     [bench+'/test_mcpwm_'+name+'.c' for name in ['single', 'complementary', 'three_pair']]),
]
with tempfile.TemporaryDirectory(prefix='lks560-tests-') as temp:
    for name, includes, sources in cases:
        exe = str(Path(temp)/(name+'.exe'))
        subprocess.run([args.cc, '-std=c99', '-Wall', '-Wextra', '-Werror']+
                       ['-I'+str(root/p) for p in includes]+
                       [str(root/p) for p in sources]+['-o', exe], check=True)
        subprocess.run([exe], check=True)
