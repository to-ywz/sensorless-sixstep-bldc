#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "lks32mc07x_lib.h"
#include "board_init.h"
#include "bench_sixstep_output.h"
#include "test_mcpwm_single.h"
#include "test_mcpwm_complementary.h"
#include "test_mcpwm_three_pair.h"

uint32_t MCPWM0_PRT,MCPWM0_TCLK,MCPWM0_IO01,MCPWM0_IO23;
uint32_t MCPWM0_TH00,MCPWM0_TH01,MCPWM0_TH10,MCPWM0_TH11,MCPWM0_TH20,MCPWM0_TH21;
uint32_t MCPWM0_CNT0,MCPWM0_UPDATE,MCPWM0_FAIL012;
static MCPWM_InitTypeDef captured;
static uint32_t gpio[3],af[16],def_word,fail_mask,stop_pressed;
static FakeSysTick tick;
FakeSysTick *fake_systick(void)
{
    if(tick.CTRL&1u) tick.VAL=(tick.VAL-48u)&0xffffffu;
    return &tick;
}
void GPIO_PinAFConfig(uint32_t port,uint32_t pin,uint32_t mode)
{
    if(port==1u){
        if(mode==3u) assert(!(MCPWM0_FAIL012&64u));
        af[pin]=mode;
    }
}
void GPIO_StructInit(GPIO_InitTypeDef *p){memset(p,0,sizeof(*p));}
void GPIO_Init(uint32_t port,GPIO_InitTypeDef *p){(void)port;(void)p;}
void GPIO_ResetBits(uint32_t p,uint32_t mask){gpio[p]&=~mask;}
void GPIO_SetBits(uint32_t p,uint32_t mask){gpio[p]|=mask;}
uint32_t GPIO_ReadInputDataBit(uint32_t port,uint32_t pin)
{(void)port;return pin==(1u<<12)?!stop_pressed:1u;}
uint32_t GPIO_ReadOutputDataBit(uint32_t port,uint32_t pin){return !!(gpio[port]&pin);}
void SYS_ModuleClockCmd(uint32_t a,uint32_t b){(void)a;(void)b;}
void SYS_MclkChoice(uint32_t a){(void)a;}
void MCPWM_StructInit(MCPWM_InitTypeDef *p){memset(p,0,sizeof(*p));}
static uint32_t io_word(uint32_t ns,uint32_t ps,uint32_t n,uint32_t p,
                        uint32_t ni,uint32_t pi)
{return ni|(pi<<1)|(ns<<2)|(ps<<3)|(n<<4)|(p<<5);}
void MCPWM_Init(MCPWM_InitTypeDef *p)
{
    captured=*p;MCPWM0_FAIL012=0;
    MCPWM0_IO01=io_word(p->CH0_NS,p->CH0_PS,p->CH0N_SCTRLN,p->CH0P_SCTRLP,
        p->CH0N_Polarity_INV,p->CH0P_Polarity_INV)|
        (io_word(p->CH1_NS,p->CH1_PS,p->CH1N_SCTRLN,p->CH1P_SCTRLP,
        p->CH1N_Polarity_INV,p->CH1P_Polarity_INV)<<8);
    MCPWM0_IO23=io_word(p->CH2_NS,p->CH2_PS,p->CH2N_SCTRLN,p->CH2P_SCTRLP,
        p->CH2N_Polarity_INV,p->CH2P_Polarity_INV);
    def_word=p->CH0N_default_output|(p->CH0P_default_output<<1)|
      (p->CH1N_default_output<<2)|(p->CH1P_default_output<<3)|
      (p->CH2N_default_output<<4)|(p->CH2P_default_output<<5);
    fail_mask=p->CH0N_FAIL_EN|(p->CH0P_FAIL_EN<<1)|
      (p->CH1N_FAIL_EN<<2)|(p->CH1P_FAIL_EN<<3)|
      (p->CH2N_FAIL_EN<<4)|(p->CH2P_FAIL_EN<<5);
}
void PWMOutputs(FuncState en)
{if(en)MCPWM0_FAIL012|=64u;else MCPWM0_FAIL012&=~64u;}
/* Model SDK IO layout and CH_DEF bypass of polarity. Deadtime is not modeled. */
static unsigned pad(unsigned phase,unsigned low,unsigned timer)
{
    unsigned pin=4+2*phase+low,word,level,bit=2*phase+!low;
    if(af[pin]!=3u)return !!(gpio[1]&(1u<<pin));
    if(!(MCPWM0_FAIL012&64u)&&(fail_mask&(1u<<bit)))
        return (def_word>>bit)&1u;
    word=phase<2?(MCPWM0_IO01>>(phase*8))&255u:MCPWM0_IO23&255u;
    if(low)level=(word&4u)?!!(word&16u):!timer;
    else level=(word&8u)?!!(word&32u):timer;
    return level^((word>>(low?0:1))&1u);
}
/* Check PRE-polarity requests as well as physical pins: a center-aligned
 * channel must never request both gates, even if physical inversion would
 * make its final HIN/LIN levels look superficially correct.
 */
static unsigned request(unsigned phase,unsigned low,unsigned timer)
{
    unsigned word=phase<2?(MCPWM0_IO01>>(phase*8))&255u:MCPWM0_IO23&255u;
    return low ? ((word&4u)?!!(word&16u):!timer) :
                 ((word&8u)?!!(word&32u):timer);
}
static void assert_off(void)
{
    unsigned p,t;
    for(p=0;p<3;p++)for(t=0;t<2;t++){
        assert(pad(p,0,t)==0u);assert(pad(p,1,t)==1u);
    }
}
static void assert_defaults(void)
{
    assert(def_word==0x15u);assert(fail_mask==0x3fu);
    assert(captured.TR0_AEC==0u);
}
static void reset_board(void)
{
    memset(gpio,0,sizeof(gpio));memset(af,0,sizeof(af));memset(&tick,0,sizeof(tick));
    stop_pressed=0;MCPWM0_FAIL012=0;def_word=fail_mask=0;
    board_init_safe();assert_off();
}
int main(void)
{
    static const unsigned pwm[]={0,0,1,1,2,2},sink[]={1,2,2,0,0,1};
    unsigned s,d,p,t,next;uint32_t io23;
    reset_board();bench_sixstep_output_init();assert_defaults();assert_off();
    for(d=0;d<2;d++)for(s=0;s<6;s++){
        next=d?(6-s)%6:s;
        assert(bench_sixstep_output_apply(next,250u));
        assert(MCPWM0_UPDATE==0x083fu);
        assert(MCPWM0_PRT==0xCAFEu);
        for(p=0;p<3;p++)for(t=0;t<2;t++){
            unsigned h=pad(p,0,t),n=pad(p,1,t);
            assert(request(p,0,t)==(p==pwm[next]?t:0u));
            assert(request(p,1,t)==(p==sink[next]?1u:0u));
            assert(!(request(p,0,t) && request(p,1,t)));
            /* Independent expected physical levels for actual board. */
            assert(h==(p==pwm[next]?t:0u));
            assert(n==(p==sink[next]?0u:1u));
            /* Driver truth table: HO=HIN&LIN; LO=!HIN&!LIN. */
            assert((h&&n)==(p==pwm[next]&&t));
            assert((!h&&!n)==(p==sink[next]));
        }
        /* Hardware MOE/Fail default, with AF still active: no GPIO rescue. */
        MCPWM0_FAIL012&=~64u;assert_off();
    }
    assert(!bench_sixstep_output_apply(6,250));assert_off();
    assert(!bench_sixstep_output_apply(0,99));assert_off();
    assert(!bench_sixstep_output_apply(0,501));assert_off();
    stop_pressed=1;assert(!bench_sixstep_output_apply(0,250));assert_off();
    stop_pressed=0;assert(bench_sixstep_output_apply(0,100));
    bench_sixstep_output_stop();assert_off();
    /* Preserve CH3 upper byte while applying a CH0/1/2 state. */
    io23=MCPWM0_IO23=0xa500u;
    assert(bench_sixstep_output_apply(0,500));assert((MCPWM0_IO23&0xff00u)==io23);
    reset_board();bench_mcpwm_single_init();assert_defaults();assert_off();
    bench_mcpwm_single_enable();
    for(p=0;p<3;p++)for(t=0;t<2;t++){
        assert(pad(p,0,t)==(p==0?t:0u));assert(pad(p,1,t)==1u);
    }
    MCPWM0_FAIL012&=~64u;assert_off();bench_mcpwm_single_disable();assert_off();
    reset_board();bench_mcpwm_complementary_init();assert_defaults();assert_off();
    bench_mcpwm_complementary_enable();
    for(p=0;p<3;p++)for(t=0;t<2;t++){
        assert(pad(p,0,t)==(p==0?t:0u));assert(pad(p,1,t)==(p==0?t:1u));
    }
    MCPWM0_FAIL012&=~64u;assert_off();bench_mcpwm_complementary_disable();assert_off();
    reset_board();bench_mcpwm_three_pair_init();assert_defaults();assert_off();
    bench_mcpwm_three_pair_enable();
    for(p=0;p<3;p++)for(t=0;t<2;t++){
        assert(pad(p,0,t)==t);assert(pad(p,1,t)==t);
    }
    MCPWM0_FAIL012&=~64u;assert_off();bench_mcpwm_three_pair_disable();assert_off();
    puts("PASS: real A0-A4 sources, LKS560 physical input/gate mapping, GPIO STOP, MOE defaults, invalid inputs and STOP-before-enable");
    return 0;
}
