"""Read-only SOURCE guard for today's blocked platform, not an MCU test.

The Manager regression links a Fake, never lks_protection.c. This separate
check ties the blocked Mock scenario to actual source gates. It intentionally
fails if those gates are later changed; hardware qualification needs its own
review, not an automatic change to these expectations.
"""
from pathlib import Path
import re


def without_comments(text):
    return re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)


def function(text, name):
    match = re.search(r"\b" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{", text)
    if not match:
        raise AssertionError("Missing function: " + name)
    start = match.end()
    depth = 1
    for i in range(start, len(text)):
        depth += (text[i] == "{") - (text[i] == "}")
        if not depth:
            return text[start:i]
    raise AssertionError("Unclosed function: " + name)


def check(root):
    platform = root / 'firmware/platform/lks32mc071/lks_protection.c'
    source = without_comments(platform.read_text(encoding='utf-8'))
    read = function(source, 'LksProtection_Read')
    for field in ('config_valid', 'raw_valid', 'hardware_qualified'):
        assignments = re.findall(r's->' + field + r'\s*=\s*([^;]+);', read)
        assert assignments == ['0u'], (field, assignments)
    enable = function(source, 'LksProtection_SetMoe')
    assert re.search(
        r'if\s*\(enable\s*!=\s*0u\)\s*\{\s*'
        r'LksProtection_ExitCritical\(key\);\s*return PROTECTION_BLOCKED;\s*\}',
        enable), 'MOE enable must return BLOCKED before register writes'
    assert not re.search(r'MCPWM0_FAIL012\s*\|=', enable)
    clear = function(source, 'LksProtection_ClearFail')
    assert clear.strip() == 'return PROTECTION_BLOCKED;'
    init = function(source, 'LksProtection_Init')
    assert init.rstrip().endswith('return PROTECTION_BLOCKED;')
    assert 'MCPWM_StructInit(&cfg);' in init
    assert 'lks560_pwm_set_off_defaults(&cfg);' in init
    assert not re.search(r'cfg\.FAIL[01]_EN\s*=\s*(?:ENABLE|1)', init)
    assert not re.search(r'MCPWM0_FAIL012\s*\|=', init)
    unresolved = re.search(r'#define UNRESOLVED ([^\n]+)', source).group(1)
    for flag in ('CLOCK', 'WINDOW', 'ANALOG', 'RESTART'):
        assert 'PROTECTION_BLOCK_' + flag in unresolved
    assert 's->blockers=UNRESOLVED;' in read
    defaults = without_comments(
        (root / 'firmware/platform/lks32mc071/lks560_pwm_defaults.c')
        .read_text(encoding='utf-8'))
    assert 'FAIL1_EN' not in defaults
    # StructInit supplies FAIL1_EN=0; this is source inspection only.
    sdk = (root / 'firmware/projects/keil/lks32mc071/'
           'LKS32MC07x_Periph_Driver/Source/lks32mc07x_mcpwm.c')
    sdk_source = without_comments(sdk.read_bytes().decode('gb18030', errors='replace'))
    struct_init = function(sdk_source, 'MCPWM_StructInit')
    assert re.search(r'memset\(MCPWM_InitStruct,\s*0,', struct_init)
    # Initial diagnostic bench cannot silently claim an injection test PASS.
    bench = without_comments(
        (root / 'firmware/tests/bench_cmp_fail/bench_cmp_fail.c')
        .read_text(encoding='utf-8'))
    assert 'BENCH_PASS' not in bench
    for name in ('raw', 'hardware_fail', 'latch', 'clear_run'):
        assert f'bench_cmp_fail_report.{name}=BENCH_BLOCKED;' in bench
    assert 'Protection_RequestRun(' not in bench
    print('PASS: blocked platform SOURCE guard (not hardware): '
          'config/raw/hardware qualification=0, FAIL1_EN default=0, '
          'RUN/MOE/CLEAR blocked; P0-P4 not executed', flush=True)


if __name__ == '__main__':
    check(Path(__file__).resolve().parents[2])
