#ifndef PROTECTION_FAKE_PLATFORM_H
#define PROTECTION_FAKE_PLATFORM_H
#include "lks_protection.h"

#define FAKE_FAIL1 (1u << 5)
#define FAKE_MOE (1u << 6)
typedef struct {
    ProtectionSnapshot boot, current;
    ProtectionResult init_result, enable_result, disable_result, clear_result;
    uint32_t sdk_clears_eif, race_eif, race_raw, race_drops_moe;
    uint32_t clear_reassert_eif, clear_reassert_raw, clear_sets_moe;
    uint32_t clear_loses_raw, clear_loses_config, clear_loses_readability;
    uint32_t enable_no_write, enable_loses_config, enable_loses_readability;
    uint32_t enable_partially_writes;
    uint32_t init_calls, read_calls, enable_calls, disable_calls, clear_calls;
    uint32_t primask, expected_primask, depth, max_depth, enters, exits;
    uint32_t keys[16];
} ProtectionFake;
extern ProtectionFake fake;
/* Called once per fresh test PROCESS, never resets production Manager state. */
void Fake_Qualified(void);
void Fake_SetMoe(uint32_t value);
void Fake_AssertBalanced(void);
#endif
