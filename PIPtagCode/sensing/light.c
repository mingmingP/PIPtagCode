/*
 * light.c
 *
 *  Created on: Apr 23, 2014
 *      Author: romoore
 */

#include "light.h"

uint8_t cached_light = 0;
uint8_t* light_location;

void doSenseAmbientLight(bool update) {
	static bool first = true;
	if (first || update) {
		first = false;
		cached_light = relativeLightLevel();
	}

	light_location = malloc(1);
	light_location[0] = ((uint8_t*) &cached_light)[0];

}

volatile uint8_t ambient_val = 0;

/*
 * Uses the onboard LED to determine a "rough" estimation of the
 * light level in the tag's local environment.  Because the LED has
 * a relatively narrow field of view, has a directional component.
 * Also relies on TIMER0_A1_VECTOR declared in interrupt.c
 */
uint8_t relativeLightLevel(void) {

	ambient_val = 0;

	P2REN &= ~0x01;				//disable resistors for P2.0
	P2OUT &= ~0x01;				//make low when output
	P2DIR |= 0x01;				//make P2.0 output

	P3SEL2 &= ~0x01;

	TA0CCTL2 = 0;	//CLEAR TIMER
	TA0CTL = TACLR;	// Clear counter
	unsigned int i = 0;
	for (i = 0; i < 5; i++)							//delay
			{
	}

	//Start Timer_A (SMCLK, Continuous)
	// TASSEL_1 = ACLK
	// TASSEL_2 = SMCLK
	// MC_0 = STOP
	// MC_1 = Up to TACCR0
	// MC_2 = Continuous up
	// ID_1 = Clock divider 2
	// ID_2 = Clock divider 4
	// ID_3 = Clock divider 8
	// 12kHz VLO/ACLK divide 2 * 512 steps ~100ms
	// or divide 4 * 256 steps
	TA0CTL = TASSEL_1 + MC_1 + ID_2;//ACLK , UP to TACCR. Use ISR to count iterations in sleep
	TA0CCR2 = 0;

	//Set up CC for capture
	TA0CCTL2 = CM_2 + CCIS_0 + CAP + SCS;
	TA0CCTL2 &= ~CCIE;

	//Setup Output Pin going to the LED
	P3SEL &= ~0x01;							//Select bit has to be 0 to be high
	P3DIR &= ~0x01;
	P3OUT |= 0x01;
	P3REN |= 0x01;						      //enable Pull up resistor for P3.0

	// Delay to fully charge the LED capacitance
	for (i = 0; i < 5; i++) {
	}

	//Clear the CCIFG interrupt flag
	TA0CCTL2 &= ~CCIFG;

	//change pin P3.0 to input
	P3REN &= ~0x01;						//disable resistors for P3.0
	P3SEL |= 0x01;						//Select bits changed for CC

	// Set the "time-out" for declaring "dark" level of light
	TACCR0 = OPTICAL_DELAY_DURATION;

	//TA0CCTL2 |= TAIFG + CCIE + TAIE;
	TA0CCTL2 |= CCIE;
	// Sleep until wake-up.
	__bis_SR_register(LPM3_bits + GIE);

	// Turn off Timer_A0 capture compare
	TA0CCTL2 = 0;
	//Shut down the timer system
	TA0CTL &= ~MC_1;
	TACCR0 = 0;
	TACCTL1 &= ~(CM_2 + CCIFG);	// Turn off TA1 captures, clear interrupt flag
	CACTL1 &= ~CAON;

	// Make sure pins are "off" with no leakage
	P2REN |= 0x01;	// enable resistors
	P2OUT &= ~0x01;	// pull-down
	P2DIR &= ~0x01;	// Set input
	P2SEL &= ~0x01; // Set I/O function
	P2SEL2 &= ~0x01; // Set I/O function

	P3REN |= 0x01;	// enable resistors
	P3OUT &= ~0x01;	// pull-down
	P3DIR &= ~0x01;	// Set input
	P3SEL &= ~0x01; // Set I/O function
	P3SEL2 &= ~0x01; // Set I/O function

	return ambient_val;
}



