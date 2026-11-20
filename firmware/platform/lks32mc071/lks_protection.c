/**
 * @file lks_protection.c
 * @brief 只实现已有依据的安全初始化、快照与关断；未决项保持阻塞。
 * 依据：SDK Source/lks32mc07x_mcpwm.c；手册表14-33、14-36及
 * FAIL012/CH_DEF/CH_MASK/SDCFG/PRT位表；时钟差异见2026-11-17专项报告。
 * 不修改SDK，不调用A5输出后端，不以寄存器读回替代硬件验收。
 */
#include "lks32mc07x_lib.h"
#include "lks560_pwm_defaults.h"
#include "lks_protection.h"
#include <string.h>
#include "board_init.h"

#ifdef M2_P1_DYNAMIC
static uint32_t p1_prepared;
static uint32_t p1_output;
#endif
/* 仅表示平台已建立过MCPWM访问条件，不表示保护初始化成功。
 * 每次Read仍需重新检查实际模块门控，不能只信任这个软件标志。 */
static uint32_t initialized;
/* SDK mcpwm.c / manual FAIL012, EIF, PRT tables. No SDK files modified. */
#define PRT_UNLOCK 0xDEADu
#define PRT_LOCK 0xCAFEu
#define FAIL1_FLAG (1u << 5)
#define UNRESOLVED (PROTECTION_BLOCK_CLOCK | PROTECTION_BLOCK_WINDOW | PROTECTION_BLOCK_ANALOG | PROTECTION_BLOCK_RESTART)
/* 保存调用前PRIMASK，支持Manager和平台层嵌套；退出不能无条件开中断。
 * 临界区防止可屏蔽中断与前台软件竞争，不会冻结异步硬件FAIL事件。 */
uint32_t LksProtection_EnterCritical(void)
{
    uint32_t key = __get_PRIMASK();
    __disable_irq();
    return key;
}
void LksProtection_ExitCritical(uint32_t key) { __set_PRIMASK(key); }
/* This checks the intentionally DISABLED safe configuration only. It does
 * not qualify an armed FAIL path or prove a silicon clock connection. */
static uint32_t safe_config_matches(const ProtectionSnapshot *s)
{
    /* CH_DEF/CH_MASK低6位覆盖三相P/N；SDCFG[6:4]禁自动恢复/更新。
     * FAIL012掩码0xC70覆盖SEL[11:10]、MOE[6]、EN[5:4]：仅SEL=1。
     * TCLK掩码0x47覆盖CNT0_EN[6]、CLK_EN[2]、DIV[1:0]：仅CLK_EN=1。
     * 这是关闭态子集检查，未声称核对所有外设寄存器。 */
    return s->mcpwm_readable &&
        (s->ch_def & 0x3Fu) == 0x15u &&
        (s->ch_mask & 0x3Fu) == 0x3Fu &&
        s->flt == 0u && s->auen == 0u &&
        (s->sdcfg & 0x70u) == 0u &&
        (s->fail012 & 0xC70u) == 0x400u &&
        (s->tclk & 0x47u) == 4u;
}
void LksProtection_Read(ProtectionSnapshot *s)
{
    uint32_t key;
    if (s == 0) return;
    key=LksProtection_EnterCritical();
    /* 禁用CPU中断仅防软件交叉访问，多寄存器快照不是硬件原子采样。
     * FAIL仍可在读取过程中发生；不能据此证明精确先后时序。 */
    memset(s,0,sizeof(*s));
    s->sys_clk_fen=SYS_CLK_FEN;
    s->blockers=UNRESOLVED;
    /* Never access a gated peripheral. Early reads expose SYS state only. */
    if (initialized && (s->sys_clk_fen & SYS_Module_MCPWM0)) {
        s->mcpwm_readable=1u;
        s->fail012=MCPWM0_FAIL012; s->eif=MCPWM0_EIF;
        s->ch_def=MCPWM0_CH_DEF; s->ch_mask=MCPWM0_CH_MASK;
        s->tclk=MCPWM0_TCLK; s->flt=MCPWM0_FLT;
        s->sdcfg=MCPWM0_SDCFG; s->auen=MCPWM0_AUEN;
        s->moe=(s->fail012 & MCPWM0_FAIL012_MOE_BIT) != 0u;
        s->safe_config_matches=safe_config_matches(s);
    }
    /* No qualified CMP/DAC setup yet: raw=0 is a placeholder, NOT a
     * measurement. Even enabling CMP digital clock alone cannot qualify it. */
    s->raw_valid=0u;
    s->hardware_qualified=0u;
    s->config_valid=0u;
#ifdef M2_P1_DYNAMIC
    if (p1_prepared && s->mcpwm_readable) {
        uint32_t raw=LksProtection_P0ReadRaw();
        s->raw_valid=(raw<=1u);
        s->raw=raw;
        /* Eligibility is local to this experiment. Hardware qualification stays zero. */
        s->config_valid=(s->fail012 & 0xC38u)==0x420u &&
            (s->tclk & 0x07u)==4u && s->flt==0u && s->auen==0u &&
            !(s->sdcfg & 0x70u) && (s->ch_def & 0x3fu)==0x15u &&
            (s->ch_mask & 0x3fu)==0x3fu;
        if (s->config_valid && s->raw_valid) s->blockers=0u;
    }
#endif
    LksProtection_ExitCritical(key);
}
#ifdef M2_P1_DYNAMIC
uint32_t LksProtection_SelectBenchOutput(uint32_t output)
{
    if (!p1_prepared || output>=6u) return 0u;
    p1_output=output;
    return 1u;
}
#endif

ProtectionResult LksProtection_SetMoe(uint32_t enable)
{
    ProtectionResult result;
    uint32_t key=LksProtection_EnterCritical();
    if (enable != 0u) {
#ifdef M2_P1_DYNAMIC
        ProtectionSnapshot s;
        LksProtection_Read(&s);
        if (enable==1u && s.config_valid && s.raw_valid && !s.raw &&
            !(s.eif & MCPWM0_EIF_FAIL1_IF_BIT) && !s.moe &&
            !(s.tclk & MCPWM0_TCLK_BASE_CNT0_EN_BIT)) {
            GPIO_PinAFConfig(GPIO1, GPIO_PinSource_4+p1_output, AF3_MCPWM);
            MCPWM0_PRT=PRT_UNLOCK;
            MCPWM0_CNT0=(uint16_t)-2400;
            MCPWM0_UPDATE=0x083fu; /* CNT0 and CH0/1/2 compare pairs. */
            MCPWM0_TCLK |= MCPWM0_TCLK_BASE_CNT0_EN_BIT;
            MCPWM0_FAIL012 |= MCPWM0_FAIL012_MOE_BIT;
            __DSB();
            MCPWM0_PRT=PRT_LOCK;
            LksProtection_ExitCritical(key);
            return PROTECTION_OK; /* Manager immediately checks readback and EIF. */
        }
#endif
        LksProtection_ExitCritical(key);
        return PROTECTION_BLOCKED;
    }
    /* Safe disable may be called before Init; establish access first. */
    SYS_ModuleClockCmd(SYS_Module_MCPWM0, ENABLE);
    if (!(SYS_CLK_FEN & SYS_Module_MCPWM0)) {
        LksProtection_ExitCritical(key);
        return PROTECTION_IO_ERROR;
    }
    /* SDK要求受保护写操作先DEAD解锁，再CAFE重锁。
     * 顺序：先关闭输出权限，再停CNT0，避免以停计数器代替安全默认态。
     * 不清EIF：普通STOP必须保留故障证据。 */
    MCPWM0_PRT=PRT_UNLOCK;
    MCPWM0_FAIL012 &= ~MCPWM0_FAIL012_MOE_BIT;
    MCPWM0_TCLK &= ~MCPWM0_TCLK_BASE_CNT0_EN_BIT;
    __DSB();
    MCPWM0_PRT=PRT_LOCK;
    result=((MCPWM0_FAIL012 & MCPWM0_FAIL012_MOE_BIT) ||
            (MCPWM0_TCLK & MCPWM0_TCLK_BASE_CNT0_EN_BIT)) ?
            PROTECTION_IO_ERROR : PROTECTION_OK;
#ifdef M2_P1_DYNAMIC
    board_pwm_inputs_off();
#endif
    LksProtection_ExitCritical(key);
    return result;
}
ProtectionResult LksProtection_Init(ProtectionSnapshot *boot)
{
    MCPWM_InitTypeDef cfg;
    ProtectionSnapshot current;
    if (boot == 0) return PROTECTION_INVALID;
    memset(boot,0,sizeof(*boot));
    SYS_ModuleClockCmd(SYS_Module_MCPWM0, ENABLE);
    if (!(SYS_CLK_FEN & SYS_Module_MCPWM0)) return PROTECTION_IO_ERROR;
    /* 这里只建立寄存器可访问性；SDK随后会清EIF，因此必须先拍快照。 */
    initialized=1u;
    /* Capture evidence before SDK MCPWM_Init clears EIF. */
    LksProtection_Read(boot);
    if (LksProtection_SetMoe(0u) != PROTECTION_OK) return PROTECTION_IO_ERROR;
    /* StructInit清零使CNT0/FAIL使能、AEC、AUEN及两级分频保持目标初值。
     * MCLK_EN写TCLK bit2，不是SYS_CLK_FEN模块门控。
     * FAIL1只准备CMP1来源选择；极性未知前不允许使能输入。 */
    MCPWM_StructInit(&cfg);
    cfg.MCLK_EN=ENABLE;
    cfg.FAIL1_Signal_Sel=1u;
#ifdef M2_P1_DYNAMIC
    cfg.TH0=2400u;
    cfg.TH00=cfg.TH10=cfg.TH20=(uint16_t)-600;
    cfg.TH01=cfg.TH11=cfg.TH21=600u;
    cfg.MCPWM_WorkModeCH0=cfg.MCPWM_WorkModeCH1=
        cfg.MCPWM_WorkModeCH2=MCPWM0_CENTRAL_PWM_MODE;
    /* N uses a constant physical low request while MOE is active. Only
     * the selected pad is muxed; FAIL forces that N pad back high. */
    cfg.CH0_NS=cfg.CH1_NS=cfg.CH2_NS=ENABLE;
    cfg.CH0N_SCTRLN=cfg.CH1N_SCTRLN=cfg.CH2N_SCTRLN=MCPWM0_LOW_LEVEL;
#endif
    lks560_pwm_set_off_defaults(&cfg);
    MCPWM_Init(&cfg);
    /* FAIL1_EN stays zero: clock/window/polarity not qualified.
       CMP/DAC initialization intentionally blocked; do not guess voltage codes. */
    LksProtection_Read(&current);
    if (!current.safe_config_matches) return PROTECTION_IO_ERROR;
#ifdef M2_P1_DYNAMIC
    if (!(boot->eif & MCPWM0_EIF_FAIL1_IF_BIT) &&
        LksProtection_P0Init() && LksProtection_P1ProbeArm()) {
        p1_prepared=1u;
        return PROTECTION_OK;
    }
#endif
    return PROTECTION_BLOCKED;
}
ProtectionResult LksProtection_ClearFail(void)
{
#ifdef M2_P1_DYNAMIC
    ProtectionSnapshot s;
    ProtectionResult result=PROTECTION_BLOCKED;
    uint32_t key=LksProtection_EnterCritical();
    LksProtection_Read(&s);
    if (s.config_valid && s.raw_valid && !s.raw && !s.moe &&
        !(s.tclk & MCPWM0_TCLK_BASE_CNT0_EN_BIT)) {
        MCPWM0_PRT=PRT_UNLOCK;
        MCPWM0_EIF=MCPWM0_EIF_FAIL1_IF_BIT;
        __DSB();
        MCPWM0_PRT=PRT_LOCK;
        LksProtection_Read(&s);
        if (s.config_valid && s.raw_valid && !s.moe) {
            result=(s.raw || (s.eif & FAIL1_FLAG)) ? PROTECTION_FAULT : PROTECTION_OK;
        }
    }
    LksProtection_ExitCritical(key);
    return result;
#else
    /* Polarity and analog source are not qualified in this build. */
#endif
    return PROTECTION_BLOCKED;
}

/* P0 only: observe the comparator without granting protection qualification. */
uint32_t LksProtection_P0Init(void)
{
    GPIO_InitTypeDef gpio;
    const uint32_t mux_mask=SYS_AFE_REG3_CMP1_SELP_MASK |
                            SYS_AFE_REG3_CMP1_SELN_MASK;
    const uint32_t mux_value=SYS_AFE_REG3_CMP1_SELP(CMP1_SELP_IP0) |
                             SYS_AFE_REG3_CMP1_SELN(CMP_SELN_REF);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin=GPIO_Pin_1;
    gpio.GPIO_Mode=GPIO_Mode_ANA;
    SYS_ModuleClockCmd(SYS_Module_GPIO, ENABLE);
    GPIO_PinAFConfig(GPIO2, GPIO_PinSource_1, AF0_GPIO);
    GPIO_Init(GPIO2, &gpio);
    SYS_ModuleClockCmd(SYS_Module_CMP, ENABLE);
    if (!(SYS_CLK_FEN & SYS_Module_CMP)) return 0u;

    /* AFE manual section 5: BGPPD=0 enables the nominal 1.2 V reference.
     * Wait conservatively before enabling CMP1; this is not a timing test. */
    SYS_WR_PROTECT=0x7A83u;
    SYS_AFE_REG5 &= ~SYS_AnalogModule_BGP;
    SYS_WR_PROTECT=0u;
    for (volatile uint32_t wait=0; wait<1024u; ++wait) __NOP();

    SYS_WR_PROTECT=0x7A83u;
    SYS_AFE_REG3=(SYS_AFE_REG3 & ~mux_mask) | mux_value;
    SYS_AFE_REG5 |= SYS_AnalogModule_CMP1;
    SYS_WR_PROTECT=0u;
    return (SYS_AFE_REG3 & mux_mask)==mux_value &&
           (SYS_AFE_REG5 & (SYS_AnalogModule_CMP1 | SYS_AnalogModule_BGP))==
           SYS_AnalogModule_CMP1;
}

uint32_t LksProtection_P0ReadRaw(void)
{
    if (!(SYS_CLK_FEN & SYS_Module_CMP)) return 2u;
    return (CMP->DATA & CMP_DATA_CMP1_RAW_DATA_BIT)!=0u;
}


/* Routing precheck only: keep GPIO defaults, CNT0 stopped and MOE clear. */
uint32_t LksProtection_P1ProbeArm(void)
{
    ProtectionSnapshot s;
    uint32_t key=LksProtection_EnterCritical();
    uint32_t armed=0u;
    const uint32_t cmp1_windows=CMP_BLCWIN_CMP1_CHN0P_WIN_EN_BIT |
        CMP_BLCWIN_CMP1_CHN1P_WIN_EN_BIT | CMP_BLCWIN_CMP1_CHN2P_WIN_EN_BIT |
        CMP_BLCWIN_CMP1_CHN3P_WIN_EN_BIT;
    LksProtection_Read(&s);
    if (s.safe_config_matches && LksProtection_P0ReadRaw()==0u &&
        !(CMP->BLCWIN & cmp1_windows)) {
        MCPWM0_PRT=PRT_UNLOCK;
        MCPWM0_EIF=MCPWM0_EIF_FAIL1_IF_BIT;
        if (!(MCPWM0_EIF & MCPWM0_EIF_FAIL1_IF_BIT)) {
            MCPWM0_FAIL012=(s.fail012 & ~MCPWM0_FAIL012_FAIL1_POL_BIT) |
                          MCPWM0_FAIL012_FAIL1_EN_BIT;
            __DSB();
            armed=(MCPWM0_FAIL012 & 0xC78u)==0x420u;
            if (!armed) MCPWM0_FAIL012=s.fail012;
        }
        MCPWM0_PRT=PRT_LOCK;
    }
    LksProtection_ExitCritical(key);
    return armed;
}
