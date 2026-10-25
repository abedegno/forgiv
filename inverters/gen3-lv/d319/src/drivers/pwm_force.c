/* pwm_force.c: software forcing of the PWM outputs through the action qualifier's continuous
   software force register (AQCSFRC: 0 no force, 1 force low, 2 force high). The forces act
   behind the trip zones, which epwm_trip_force and epwm_trip_clear (device_init.c) handle.
   All six run from RAM (ramfuncs).

   The pairs, as InitEPwm sets them up (all at 19.2 kHz, up-down count, TBPRD 2343):
     ePWM3, ePWM4  with dead band (115 counts, active high complementary); pwm34_force_off
                   forces A low and B high, which the dead band unit turns into both outputs off
                   (my reading; the bridge they drive is the control code's)
     ePWM5         the same set-up as 3 and 4, alone
     ePWM7, ePWM8  no dead band; pwm78_force_low forces all four outputs low
   Entry points: pwm34_release, pwm34_force_off, pwm78_force_low, pwm78_release, pwm5_force_off,
   pwm5_release (from the control and state code). Data: none.
   Neighbours: device_init.c (InitEPwm), app/control/ (the callers).
   name: descriptive (map/filenames.tsv), name kept.
   at: pwm34_release 0x3F2274 .. pwm5_release 0x3F22C3 (load addresses; they run at 0x86BD ..) */
#include "F2806x_Device.h"
#include "F2806x_EPwm.h"
#include "control.h"
#include "util.h"

#pragma CODE_SECTION(pwm34_release, "ramfuncs");
/* Hands ePWM3 and ePWM4 back to their compare logic. */
void pwm34_release(void)
{
    EPwm4Regs.AQCSFRC.bit.CSFA = 0;
    EPwm4Regs.AQCSFRC.bit.CSFB = 0;
    EPwm3Regs.AQCSFRC.bit.CSFA = 0;
    EPwm3Regs.AQCSFRC.bit.CSFB = 0;
}

#pragma CODE_SECTION(pwm34_force_off, "ramfuncs");
/* ePWM3 and ePWM4: A forced low, B forced high. */
void pwm34_force_off(void)
{
    EPwm4Regs.AQCSFRC.bit.CSFA = 1;
    EPwm4Regs.AQCSFRC.bit.CSFB = 2;
    EPwm3Regs.AQCSFRC.bit.CSFA = 1;
    EPwm3Regs.AQCSFRC.bit.CSFB = 2;
}

#pragma CODE_SECTION(pwm78_force_low, "ramfuncs");
/* ePWM7 and ePWM8: both outputs forced low. */
void pwm78_force_low(void)
{
    EPwm8Regs.AQCSFRC.bit.CSFA = 1;
    EPwm8Regs.AQCSFRC.bit.CSFB = 1;
    EPwm7Regs.AQCSFRC.bit.CSFA = 1;
    EPwm7Regs.AQCSFRC.bit.CSFB = 1;
}

#pragma CODE_SECTION(pwm78_release, "ramfuncs");
/* Hands ePWM7 and ePWM8 back to their compare logic. */
void pwm78_release(void)
{
    EPwm8Regs.AQCSFRC.bit.CSFA = 0;
    EPwm8Regs.AQCSFRC.bit.CSFB = 0;
    EPwm7Regs.AQCSFRC.bit.CSFA = 0;
    EPwm7Regs.AQCSFRC.bit.CSFB = 0;
}

#pragma CODE_SECTION(pwm5_force_off, "ramfuncs");
/* ePWM5: A forced low, B forced high. */
void pwm5_force_off(void)
{
    EPwm5Regs.AQCSFRC.bit.CSFA = 1;
    EPwm5Regs.AQCSFRC.bit.CSFB = 2;
}

#pragma CODE_SECTION(pwm5_release, "ramfuncs");
/* Hands ePWM5 back to its compare logic. */
void pwm5_release(void)
{
    EPwm5Regs.AQCSFRC.bit.CSFA = 0;
    EPwm5Regs.AQCSFRC.bit.CSFB = 0;
}
