#include "protection_test_assert.h"
#include <string.h>
#include "protection_fake_platform.h"

ProtectionFake fake;
void Fake_SetMoe(uint32_t value)
{
    fake.current.moe=value;
    if (value) fake.current.fail012|=FAKE_MOE;
    else fake.current.fail012&=~FAKE_MOE;
}
void Fake_Qualified(void)
{
    memset(&fake,0,sizeof(fake));
    fake.current.mcpwm_readable=1u;
    fake.current.raw_valid=1u;
    fake.current.config_valid=1u;
    fake.current.hardware_qualified=1u;
    fake.current.safe_config_matches=1u;
    fake.current.ch_def=0x15u;
    fake.current.ch_mask=0x3fu;
    fake.boot=fake.current;
}
uint32_t LksProtection_EnterCritical(void)
{
    uint32_t key=fake.primask;
    CHECK(fake.depth<16u);
    fake.keys[fake.depth++]=key;
    fake.primask=1u;
    ++fake.enters;
    if (fake.depth>fake.max_depth) fake.max_depth=fake.depth;
    return key;
}
void LksProtection_ExitCritical(uint32_t key)
{
    CHECK(fake.depth>0u && fake.primask==1u);
    CHECK(fake.keys[fake.depth-1u]==key);
    --fake.depth;
    fake.primask=key;
    ++fake.exits;
}
/* Each platform operation nests inside the Manager critical section, just
 * as real Read/SetMoe do. This tests save/restore semantics, not CPU silicon. */
static uint32_t platform_enter(void)
{
    CHECK(fake.primask==1u && fake.depth>=1u);
    return LksProtection_EnterCritical();
}
ProtectionResult LksProtection_Init(ProtectionSnapshot *boot)
{
    uint32_t key=platform_enter();
    CHECK(boot!=0);
    ++fake.init_calls;
    *boot=fake.boot;
    if (fake.sdk_clears_eif) fake.current.eif=0u;
    LksProtection_ExitCritical(key);
    return fake.init_result;
}
void LksProtection_Read(ProtectionSnapshot *out)
{
    uint32_t key=platform_enter();
    CHECK(out!=0);
    ++fake.read_calls;
    *out=fake.current;
    LksProtection_ExitCritical(key);
}
ProtectionResult LksProtection_SetMoe(uint32_t enable)
{
    ProtectionResult result;
    uint32_t key=platform_enter();
    CHECK(enable<=1u);
    if (enable) {
        ++fake.enable_calls;
        result=fake.enable_result;
        if ((result==PROTECTION_OK && !fake.enable_no_write) ||
            fake.enable_partially_writes) Fake_SetMoe(1u);
        /* An asynchronous hardware event is deliberately injected even with
         * PRIMASK=1: disabling CPU interrupts does not suppress FAIL. */
        if (fake.race_eif) fake.current.eif|=FAKE_FAIL1;
        if (fake.race_raw) fake.current.raw=1u;
        if (fake.race_drops_moe) Fake_SetMoe(0u);
        if (fake.enable_loses_config) fake.current.config_valid=0u;
        if (fake.enable_loses_readability) fake.current.mcpwm_readable=0u;
    } else {
        ++fake.disable_calls;
        result=fake.disable_result;
        if (result==PROTECTION_OK) Fake_SetMoe(0u);
    }
    LksProtection_ExitCritical(key);
    return result;
}
ProtectionResult LksProtection_ClearFail(void)
{
    uint32_t key=platform_enter();
    ++fake.clear_calls;
    if (fake.clear_result==PROTECTION_OK) {
        fake.current.eif&=~FAKE_FAIL1; /* W1C model preserves unrelated flags. */
        if (fake.clear_reassert_eif) fake.current.eif|=FAKE_FAIL1;
        if (fake.clear_reassert_raw) fake.current.raw=1u;
        if (fake.clear_sets_moe) Fake_SetMoe(1u);
        if (fake.clear_loses_raw) fake.current.raw_valid=0u;
        if (fake.clear_loses_config) fake.current.config_valid=0u;
        if (fake.clear_loses_readability) fake.current.mcpwm_readable=0u;
    }
    LksProtection_ExitCritical(key);
    return fake.clear_result;
}
void Fake_AssertBalanced(void)
{
    CHECK(fake.depth==0u && fake.enters==fake.exits);
    CHECK(fake.primask==fake.expected_primask);
    CHECK(fake.max_depth>=2u);
}
