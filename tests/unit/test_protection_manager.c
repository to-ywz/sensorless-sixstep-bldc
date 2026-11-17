#include "protection_test_assert.h"
#include <stdio.h>
#include <string.h>
#include "protection_manager.h"
#include "protection_fake_platform.h"

static ProtectionFaultInfo info(void)
{
    ProtectionFaultInfo f;
    memset(&f,0,sizeof(f));
    Protection_GetFaultInfo(&f);
    return f;
}
static void disarmed(void)
{
    ProtectionFaultInfo f=info();
    CHECK(Protection_GetState()==DISARMED);
    CHECK(!fake.current.moe && !f.latched);
}
static void latched(ProtectionResult reason)
{
    ProtectionFaultInfo f=info();
    CHECK(Protection_GetState()==FAULT_LATCHED);
    CHECK(f.latched==1u && f.reason==reason);
}
static void init_ok(void)
{
    CHECK(Protection_GetState()==BOOT_SAFE);
    CHECK(Protection_Init()==PROTECTION_OK);
    disarmed();
}
static void running(void)
{
    init_ok();
    CHECK(Protection_RequestRun()==PROTECTION_OK);
    CHECK(Protection_GetState()==RUNNING && fake.current.moe==1u);
}
static void eif_fault(void)
{
    running();
    fake.current.eif=FAKE_FAIL1;
    CHECK(Protection_Process()==PROTECTION_FAULT);
    latched(PROTECTION_FAULT);
    CHECK(fake.current.moe==0u);
}
static void source_removed(void)
{
    fake.current.raw=0u;
    fake.current.eif=0u;
}
static void run_refused(void)
{
    uint32_t calls=fake.enable_calls;
    CHECK(Protection_RequestRun()!=PROTECTION_OK);
    CHECK(fake.enable_calls==calls);
    CHECK(Protection_GetState()==FAULT_LATCHED);
}
static void clear_refused(void)
{
    ProtectionFaultInfo before=info(), after;
    ProtectionState before_state=Protection_GetState();
    uint32_t clears=fake.clear_calls, enables=fake.enable_calls;
    CHECK(Protection_RequestClear()==PROTECTION_INVALID);
    after=info();
    CHECK(fake.clear_calls==clears && fake.enable_calls==enables);
    CHECK(Protection_GetState()==before_state);
    CHECK(memcmp(&before,&after,sizeof(before))==0);
}
static void sw01(void)
{
    ProtectionFaultInfo f;
    fake.boot.eif=FAKE_FAIL1|0x100u;
    fake.sdk_clears_eif=1u;
    fake.current.eif=fake.boot.eif;
    CHECK(Protection_Init()==PROTECTION_FAULT);
    f=info();
    latched(PROTECTION_FAULT);
    CHECK(f.boot.eif==(FAKE_FAIL1|0x100u));
    CHECK(f.first_fault.eif==0u && fake.current.eif==0u);
    run_refused();
}
static void sw02_blocked(void)
{
    fake.init_result=PROTECTION_BLOCKED;
    fake.current.raw_valid=0u;
    fake.current.config_valid=0u;
    fake.current.hardware_qualified=0u;
    fake.current.blockers=PROTECTION_BLOCK_CLOCK|PROTECTION_BLOCK_WINDOW|
        PROTECTION_BLOCK_ANALOG|PROTECTION_BLOCK_RESTART;
    fake.boot=fake.current;
    CHECK(Protection_Init()==PROTECTION_BLOCKED);
    latched(PROTECTION_BLOCKED);
    CHECK(Protection_Process()==PROTECTION_BLOCKED);
    CHECK(Protection_RequestRun()==PROTECTION_BLOCKED);
    CHECK(fake.enable_calls==0u && fake.current.moe==0u);
    clear_refused();
}
static void sw02_raw(void)
{
    fake.current.raw_valid=0u;
    fake.current.raw=0u; /* invalid placeholder zero is NOT a healthy source */
    CHECK(Protection_Init()==PROTECTION_BLOCKED);
    run_refused();
    CHECK(!fake.enable_calls && !fake.current.moe);
}
static void sw02_config(void)
{
    fake.current.config_valid=0u;
    CHECK(Protection_Init()==PROTECTION_BLOCKED);
    run_refused();
}
static void sw02_read(void)
{
    fake.current.mcpwm_readable=0u;
    CHECK(Protection_Init()==PROTECTION_BLOCKED);
    run_refused();
}
static void sw02_blockers(void)
{
    fake.current.blockers=PROTECTION_BLOCK_RESTART;
    CHECK(Protection_Init()==PROTECTION_BLOCKED);
    run_refused();
}
static void sw03(void)
{
    running();
    CHECK(fake.enable_calls==1u);
    CHECK(Protection_RequestRun()==PROTECTION_INVALID);
    CHECK(fake.enable_calls==1u);
    CHECK(Protection_Process()==PROTECTION_OK);
    CHECK(Protection_RequestStop()==PROTECTION_OK);
    disarmed();
    CHECK(fake.enable_calls==1u);
    CHECK(Protection_RequestStop()==PROTECTION_OK);
    disarmed();
}
static void sw04(void)
{
    ProtectionFaultInfo f;
    uint32_t offs;
    running();offs=fake.disable_calls;
    fake.current.raw=1u;
    CHECK(Protection_Process()==PROTECTION_FAULT);
    latched(PROTECTION_FAULT);f=info();
    CHECK(fake.disable_calls>offs && !fake.current.moe);
    CHECK(f.first_fault.raw_valid && f.first_fault.raw);
    CHECK(f.first_fault.moe==1u);
    run_refused();
}
static void sw05(void)
{
    ProtectionFaultInfo f;
    eif_fault();f=info();
    CHECK(f.first_fault.eif & FAKE_FAIL1);
    CHECK(!f.first_fault.raw && f.first_fault.moe);
    run_refused();
}
static void sw06_raw(void)
{
    init_ok();fake.current.raw=1u;
    CHECK(Protection_RequestRun()==PROTECTION_FAULT);
    latched(PROTECTION_FAULT);CHECK(!fake.enable_calls && !fake.current.moe);
}
static void sw06_eif(void)
{
    init_ok();fake.current.eif=FAKE_FAIL1;
    CHECK(Protection_RequestRun()==PROTECTION_FAULT);
    latched(PROTECTION_FAULT);CHECK(!fake.enable_calls && !fake.current.moe);
}
static void sw06_latched(void)
{
    eif_fault();source_removed();run_refused();latched(PROTECTION_FAULT);
}
static void sw07(void)
{
    ProtectionFaultInfo f;
    init_ok();fake.race_eif=fake.race_drops_moe=1u;
    CHECK(Protection_RequestRun()==PROTECTION_FAULT);
    latched(PROTECTION_FAULT);f=info();
    CHECK(fake.enable_calls==1u && fake.disable_calls>0u);
    CHECK((f.first_fault.eif & FAKE_FAIL1) && !f.first_fault.moe);
    CHECK(!fake.current.moe);
}
static void sw07_raw(void)
{
    init_ok();fake.race_raw=1u;
    CHECK(Protection_RequestRun()==PROTECTION_FAULT);
    latched(PROTECTION_FAULT);CHECK(!fake.current.moe);
}
static void sw07_eif_moe(void)
{
    init_ok();fake.race_eif=1u; /* FAIL evidence even if fake MOE stayed high */
    CHECK(Protection_RequestRun()==PROTECTION_FAULT);
    latched(PROTECTION_FAULT);CHECK(!fake.current.moe);
}
static void sw08(void)
{
    uint32_t calls,i;
    eif_fault();source_removed();calls=fake.enable_calls;
    for(i=0u;i<32u;++i) {
        CHECK(Protection_Process()==PROTECTION_FAULT);
        latched(PROTECTION_FAULT);
    }
    CHECK(Protection_RequestStop()==PROTECTION_OK);
    latched(PROTECTION_FAULT);run_refused();
    CHECK(fake.enable_calls==calls && !fake.current.moe);
}
static void sw09_raw_invalid(void)
{ eif_fault();fake.current.raw_valid=0u;clear_refused(); }
static void sw09_raw_fault(void)
{ eif_fault();fake.current.raw=1u;clear_refused(); }
static void sw09_moe(void)
{ eif_fault();Fake_SetMoe(1u);clear_refused(); }
static void sw09_not_latched(void)
{ init_ok();clear_refused(); }
static void sw09_read(void)
{ eif_fault();fake.current.mcpwm_readable=0u;clear_refused(); }
static void sw09_running(void)
{ running();clear_refused(); }
static void sw10(void)
{
    ProtectionFaultInfo before,after;
    uint32_t calls;
    eif_fault();before=info();calls=fake.enable_calls;
    fake.current.eif|=0x100u;
    CHECK(Protection_RequestClear()==PROTECTION_OK);
    disarmed();after=info();
    CHECK(fake.current.eif==0x100u && fake.clear_calls==1u);
    CHECK(fake.enable_calls==calls);
    CHECK(memcmp(&before.first_fault,&after.first_fault,sizeof(before.first_fault))==0);
    CHECK(Protection_Process()==PROTECTION_OK);disarmed();
    CHECK(Protection_RequestRun()==PROTECTION_OK);
    CHECK(fake.enable_calls==calls+1u && fake.current.moe);
}
static void clear_failure(ProtectionResult result)
{
    eif_fault();fake.clear_result=result;
    CHECK(Protection_RequestClear()==result);
    latched(result);CHECK(fake.clear_calls==1u && !fake.current.moe);
    run_refused();
}
static void sw10_blocked(void) { clear_failure(PROTECTION_BLOCKED); }
static void sw10_io(void) { clear_failure(PROTECTION_IO_ERROR); }
static void sw10_eif(void)
{
    eif_fault();fake.clear_reassert_eif=1u;
    CHECK(Protection_RequestClear()==PROTECTION_FAULT);
    latched(PROTECTION_FAULT);run_refused();CHECK(!fake.current.moe);
}
static void sw10_raw(void)
{
    eif_fault();fake.clear_reassert_raw=1u;
    CHECK(Protection_RequestClear()==PROTECTION_FAULT);
    latched(PROTECTION_FAULT);run_refused();
}
static void sw10_moe(void)
{
    eif_fault();fake.clear_sets_moe=1u;
    CHECK(Protection_RequestClear()==PROTECTION_IO_ERROR);
    latched(PROTECTION_IO_ERROR);CHECK(!fake.current.moe);run_refused();
}
static void sw10_raw_invalid(void)
{
    eif_fault();fake.clear_loses_raw=1u;
    CHECK(Protection_RequestClear()==PROTECTION_BLOCKED);
    latched(PROTECTION_BLOCKED);run_refused();
}
static void sw10_config(void)
{
    eif_fault();fake.clear_loses_config=1u;
    CHECK(Protection_RequestClear()==PROTECTION_BLOCKED);
    latched(PROTECTION_BLOCKED);run_refused();
}
static void sw10_read(void)
{
    eif_fault();fake.clear_loses_readability=1u;
    CHECK(Protection_RequestClear()==PROTECTION_BLOCKED);
    latched(PROTECTION_BLOCKED);run_refused();
}
static void extra_reinit(void)
{
    ProtectionFaultInfo before,after;
    eif_fault();before=info();source_removed();
    CHECK(Protection_Init()==PROTECTION_INVALID);
    after=info();CHECK(fake.init_calls==1u);
    CHECK(memcmp(&before,&after,sizeof(before))==0);
    latched(PROTECTION_FAULT);run_refused();
}
static void extra_enable_fail(void)
{
    init_ok();fake.enable_result=PROTECTION_IO_ERROR;
    fake.enable_partially_writes=1u;
    CHECK(Protection_RequestRun()==PROTECTION_IO_ERROR);
    latched(PROTECTION_IO_ERROR);CHECK(!fake.current.moe);run_refused();
}
static void extra_enable_blocked(void)
{
    init_ok();fake.enable_result=PROTECTION_BLOCKED;
    CHECK(Protection_RequestRun()==PROTECTION_BLOCKED);
    latched(PROTECTION_BLOCKED);run_refused();
}
static void extra_disable_fail(void)
{
    running();fake.disable_result=PROTECTION_IO_ERROR;
    CHECK(Protection_RequestStop()==PROTECTION_IO_ERROR);
    latched(PROTECTION_IO_ERROR);
    CHECK(fake.current.moe==1u); /* failure is visible, NOT falsely shut down */
    run_refused();CHECK(fake.current.moe==1u);
    CHECK(Protection_Process()==PROTECTION_IO_ERROR);
    clear_refused();
}
static void extra_fault_disable_fail(void)
{
    running();fake.current.raw=1u;fake.disable_result=PROTECTION_IO_ERROR;
    CHECK(Protection_Process()==PROTECTION_IO_ERROR);
    latched(PROTECTION_IO_ERROR);
    CHECK(fake.current.moe==1u && info().first_fault.raw==1u);
}
static void extra_unexpected_moe(void)
{
    init_ok();Fake_SetMoe(1u);
    CHECK(Protection_Process()==PROTECTION_IO_ERROR);
    latched(PROTECTION_IO_ERROR);CHECK(!fake.current.moe);
}
static void extra_first_fault(void)
{
    ProtectionFaultInfo before,after;
    eif_fault();before=info();
    fake.current.raw=1u;fake.current.eif=FAKE_FAIL1|0x200u;
    fake.current.flt=0xabcdefu;
    CHECK(Protection_Process()==PROTECTION_FAULT);
    fake.disable_result=PROTECTION_IO_ERROR;
    CHECK(Protection_RequestStop()==PROTECTION_IO_ERROR);
    after=info();CHECK(after.reason==PROTECTION_IO_ERROR);
    CHECK(memcmp(&before.first_fault,&after.first_fault,sizeof(before.first_fault))==0);
    CHECK(memcmp(&before.boot,&after.boot,sizeof(before.boot))==0);
}
static void extra_null(void)
{ Protection_GetFaultInfo(NULL);init_ok();Protection_GetFaultInfo(NULL); }
static void extra_critical(void)
{
    ProtectionFaultInfo f;
    uint32_t outer,inner;
    fake.primask=fake.expected_primask=1u;
    running();
    outer=LksProtection_EnterCritical();inner=LksProtection_EnterCritical();
    CHECK(outer==1u && inner==1u);
    Protection_GetFaultInfo(&f);
    CHECK(Protection_RequestStop()==PROTECTION_OK);
    CHECK(fake.primask==1u);
    LksProtection_ExitCritical(inner);LksProtection_ExitCritical(outer);
    disarmed();
}
static void extra_no_enable_write(void)
{
    init_ok();fake.enable_no_write=1u;
    CHECK(Protection_RequestRun()==PROTECTION_IO_ERROR);
    latched(PROTECTION_IO_ERROR);CHECK(!fake.current.moe);
}
static void extra_run_config_lost(void)
{
    init_ok();fake.enable_loses_config=1u;
    CHECK(Protection_RequestRun()==PROTECTION_BLOCKED);
    latched(PROTECTION_BLOCKED);CHECK(!fake.current.moe);
}
static void extra_run_read_lost(void)
{
    init_ok();fake.enable_loses_readability=1u;
    CHECK(Protection_RequestRun()==PROTECTION_BLOCKED);
    latched(PROTECTION_BLOCKED);CHECK(!fake.current.moe);
}
static void extra_init_io(void)
{
    fake.init_result=PROTECTION_IO_ERROR;
    CHECK(Protection_Init()==PROTECTION_IO_ERROR);
    latched(PROTECTION_IO_ERROR);run_refused();
}
static void extra_fault_priority(void)
{
    init_ok();fake.current.raw=1u;fake.current.config_valid=0u;
    CHECK(Protection_RequestRun()==PROTECTION_FAULT);
    latched(PROTECTION_FAULT);CHECK(!fake.enable_calls);
}
typedef struct { const char *name; void (*test)(void); } Scenario;
static const Scenario scenarios[]={
    {"SW-01-boot-history",sw01},
    {"SW-02-current-blocked",sw02_blocked},
    {"SW-02-invalid-raw",sw02_raw},
    {"SW-02-invalid-config",sw02_config},
    {"SW-02-unreadable",sw02_read},
    {"SW-02-blockers",sw02_blockers},
    {"SW-03-run-stop",sw03},
    {"SW-04-raw-fault",sw04},
    {"SW-05-eif-fault",sw05},
    {"SW-06-run-raw",sw06_raw},
    {"SW-06-run-eif",sw06_eif},
    {"SW-06-run-latched",sw06_latched},
    {"SW-07-async-fail",sw07},
    {"SW-07-raw-during-enable",sw07_raw},
    {"SW-07-eif-with-moe-high",sw07_eif_moe},
    {"SW-08-no-auto-restart",sw08},
    {"SW-09-clear-invalid-raw",sw09_raw_invalid},
    {"SW-09-clear-active-raw",sw09_raw_fault},
    {"SW-09-clear-moe-high",sw09_moe},
    {"SW-09-clear-not-latched",sw09_not_latched},
    {"SW-09-clear-unreadable",sw09_read},
    {"SW-09-clear-running",sw09_running},
    {"SW-10-clear-explicit-rerun",sw10},
    {"SW-10-clear-blocked",sw10_blocked},
    {"SW-10-clear-io-error",sw10_io},
    {"SW-10-clear-eif-reassert",sw10_eif},
    {"SW-10-clear-raw-reassert",sw10_raw},
    {"SW-10-clear-moe-reassert",sw10_moe},
    {"SW-10-clear-raw-invalidated",sw10_raw_invalid},
    {"SW-10-clear-config-invalidated",sw10_config},
    {"SW-10-clear-read-lost",sw10_read},
    {"EX-reinit-keeps-latch",extra_reinit},
    {"EX-enable-partial-failure",extra_enable_fail},
    {"EX-enable-blocked",extra_enable_blocked},
    {"EX-disable-failure-visible",extra_disable_fail},
    {"EX-fault-disable-failure",extra_fault_disable_fail},
    {"EX-disarmed-unexpected-moe",extra_unexpected_moe},
    {"EX-first-fault-preserved",extra_first_fault},
    {"EX-null-fault-info",extra_null},
    {"EX-primask-already-disabled-nested",extra_critical},
    {"EX-enable-no-write",extra_no_enable_write},
    {"EX-run-config-lost",extra_run_config_lost},
    {"EX-run-read-lost",extra_run_read_lost},
    {"EX-init-io-error",extra_init_io},
    {"EX-fault-before-blocked",extra_fault_priority}
};
int main(int argc,char **argv)
{
    size_t i;
    if (argc!=2) {fputs("One scenario per fresh process; use --list\n",stderr);return 2;}
    if (!strcmp(argv[1],"--list")) {
        for(i=0;i<sizeof(scenarios)/sizeof(scenarios[0]);++i) puts(scenarios[i].name);
        return 0;
    }
    for(i=0;i<sizeof(scenarios)/sizeof(scenarios[0]);++i) {
        if (strcmp(argv[1],scenarios[i].name)) continue;
        Fake_Qualified();
        CHECK(Protection_GetState()==BOOT_SAFE);
        scenarios[i].test();
        Fake_AssertBalanced();
        printf("PASS: %s\n",scenarios[i].name);
        return 0;
    }
    fprintf(stderr,"Unknown scenario: %s\n",argv[1]);return 2;
}
