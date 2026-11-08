#include <assert.h>
#include <stdio.h>
#include "sixstep_table.h"
int main(void)
{
    static const unsigned expected_pwm[]={0,0,1,1,2,2};
    static const unsigned expected_sink[]={1,2,2,0,0,1};
    SixstepState state; uint8_t next; unsigned s,d,i; unsigned seen;
    for (s=0;s<6;++s) {
        assert(sixstep_get(s,&state));
        assert(state.pwm_phase==expected_pwm[s]);
        assert(state.sink_phase==expected_sink[s]);
        assert(state.pwm_phase!=state.sink_phase);
        assert(state.floating_phase<3 && state.floating_phase!=state.pwm_phase &&
               state.floating_phase!=state.sink_phase);
        assert(sixstep_next(s,0,&next));
        assert(sixstep_next(next,1,&next) && next==s);
    }
    for (d=0;d<2;++d) {
        next=0;seen=0;
        for (i=0;i<6;++i) {seen|=1u<<next;assert(sixstep_next(next,d,&next));}
        assert(seen==63u && next==0);
    }
    assert(!sixstep_get(6,&state) && state.pwm_phase==255);
    assert(!sixstep_get(0xffffffffu,&state));
    assert(!sixstep_get(0,0));
    assert(!sixstep_next(6,0,&next) && next==255);
    assert(!sixstep_next(0,2,&next));assert(!sixstep_next(0,0,0));
    for (i=0;i<=1000;++i) assert(sixstep_duty_valid(i)==(i>=100 && i<=500));
    assert(!sixstep_duty_valid(0xffffffffu));
    puts("sixstep table, forward/reverse cycles, invalid inputs and duty limits: PASS");
    return 0;
}
