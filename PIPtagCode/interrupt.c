/*
 * interrupt.c
 *
 *  Created on: Apr 23, 2014
 *      Author: romoore
 */

#include "interrupt.h"
/*
 * These values must always be initialized to 0!
 * They should only be set true when that type
 * of sensing is actively using the timer interrupts.
 */
bool sensingMoisture = 0;
bool initializingHTU = 0;
bool timedSleep = 0;
extern uint8_t ambient_val;

#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void){
	TA0CCTL0 |= CCIS_3;
	TA0CTL = 0;
	_BIC_SR_IRQ(LPM3_bits);
}
/*
 * Interrupt Service Request (ISR) for Timer0_A1-3
 * Used for ambient light sensing (Timer0_A3)
 * Used for comparator based moisture sensing (Timer0_A1)
 * Used for initialization of HTU21D in LPM3
 */
#pragma vector=TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_ISR(void) {
	// Moisture probe code...
	if (sensingMoisture) {
		TACCTL1 &= ~(CM_2 + CCIFG);	// Turn off TA1 captures, clear interrupt flag
		TACTL &= ~MC_2;					// Stop Timer_A
		CACTL1 &= ~CAON;				// Turn off Comparator

		#if CM_USE_CRYSTAL
		LPM3_EXIT;					// Exit low power mode (LPM3)
		#else
		LPM1_EXIT;					// Exit low power mode (LPM1)
		#endif	// CM_USE_CRYSTAL?
	} else if (initializingHTU) {
		TA0CTL |= TACLR;					// Stop Timer_A
		TA0CTL &= ~MC_1;
		TA0CCTL1 = 0;
		LPM1_EXIT;
	} else if (timedSleep) {
		_BIC_SR_IRQ(LPM3_bits);
	} else {
		/*
		 * Ambient light sensing code
		 */
		uint16_t flags = TA0IV;
		uint16_t CCR_val = TA0CCR2;
		uint16_t tmp_val = 0;
		// LED discharged "enough" so store the value


		//In the dark, ambient_val should be 0.
		//In dark, Timer0_A1 never trigers the interrupt as the LED voltage does not drop to threshold.
		//In that case Timer0_A0 triggers when the count reaches 255.
		//interrupt counts to 255 and wakes up the processor.
		//In very bright light, ambient_val should saturate at 0xFF
		//Counter only gets to a small or zero count before the LED capacitance discharges.
		//First if is for the upper bound and the second is for the lower bound
		//Lower bound is not currently used.

		tmp_val = (OPTICAL_DELAY_DURATION + 1 - CCR_val);
		if (tmp_val > 0xFF) {
			ambient_val = 0xFF;
		}
		else if (tmp_val < 0x00) {
			ambient_val = 0x00;
		}
		else {
			ambient_val = tmp_val;
		}
		_BIC_SR_IRQ(LPM3_bits);
	}
}	// TIMER0_A1_ISR

