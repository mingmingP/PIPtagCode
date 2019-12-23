/*
 * battery.c
 *
 *  Created on: Apr 23, 2014
 *      Author: romoore
 */


#include "battery.h"


#define BATTERY_CAP_UJ = 2376000000;
extern uint32_t numBinary;
extern uint32_t numTemp;
extern uint32_t numHTU;
extern uint32_t numBattery;
extern uint32_t numRadio;
extern uint32_t numLight;
extern uint32_t numWakeup;
extern uint32_t numHistory;


uint8_t* battery_location;
uint16_t cached_battery = 0;

// Sense battery level and pack into the transmit buffer
void doBatterySense(void) {
	uint16_t batt_milliv = Msp430_GetSupplyVoltage();
	cached_battery = batt_milliv;
	uint16_t used_joules = getUsedJoules();
	battery_location = malloc(4);
	battery_location[0] = ((uint8_t*) &batt_milliv)[1];
	battery_location[1] = ((uint8_t*) &batt_milliv)[0];
	battery_location[2] = ((uint8_t*) &used_joules)[1];
	battery_location[3] = ((uint8_t*) &used_joules)[0];
}

// 0.38 uJ per binary sense
#define BIN_COST_X256 97
// 2.29 uJ per temperature sense
#define TEMP_COST_X128 293
// 0.10 uJ per battery voltage cost
#define BATT_COST_X512 51

/*
 * Header + Light, HTU21D (Temp/Humid)
 * Data: 6 bytes
 * Preamble, Sync, Length, ID, CRC
 * Overhead: 14 bytes
 * Total: 20 bytes @ 4usec/bit = 640usec
 */
#define RADIO_COST 54
/*
 * Header + Light, HTU21D (Temp/Humid), 1 History (6 bytes)
 * Data: 12 bytes
 * Preamble, Sync, Length, ID, CRC
 * Overhead: 14 bytes
 * Total: 26 bytes @ 4usec/bit = 832usec
 * Total Energy: 67 uJ
 */
#define RADIO_COST_HISTORY 13

// 2uJ to sense light in a dark room, more for warm room (40C -> ~4uJ)
#define LIGHT_COST 3
// 2.85uJ to sleep for 1 second
#define SLEEP_COST_1SECOND_x100 285
// 39 uJ using LPM3 sleep, No Hold Master. 2014/11/11
#define HTU21D_COST 39

uint16_t getUsedJoules() {

	uint32_t totalMJ = 0;

	//   477,967,500 @ 10 Years (4/second)
	totalMJ += (numBinary >> 8) * BIN_COST_X256;
	//    24,062,625 @ 10 Years (30 second)
	totalMJ += (numTemp >> 7) * TEMP_COST_X128;
	//         8,725 @ 10 Year  (1 hour)
	totalMJ += (numBattery * BATT_COST_X512) >> 9;
	// 1,136,073,600 @ 10 Year (15 second)
	totalMJ += (numRadio * RADIO_COST);
	//    45,552,000 @ 10 Year (90 second)
	totalMJ += (numHistory * RADIO_COST_HISTORY);
	//    63,115,200 @ 10 Year (15 second)
	totalMJ += (numLight * LIGHT_COST);
	//   546,998,400 @ 10 Year (15 second)
	totalMJ += (numHTU * HTU21D_COST);
	/*
	 * Sum = 2,293,778,050
	 * No overflow within 10 years with all enabled
	 */
	//   898,605,000 @ 10 Year (8/second)
	totalMJ += ((numWakeup / 1200000) * WAKE_INTVL * SLEEP_COST_1SECOND_x100);

	// totalMJ = 3,192,383,050
	return (uint16_t) (totalMJ / 1000000);
}

/*
 * Author: Michal Potrzebicz
 * URL http://blog.elevendroids.com/2013/06/code-recipe-reading-msp430-power-supply-voltage-level/
 */
uint16_t Msp430_GetSupplyVoltage(void) {
	++numBattery;
	uint16_t raw_value;
	// first attempt - measure Vcc/2 with 1.5V reference (Vcc < 3V )
	ADC10CTL0 = SREF_1 // Select reference  (default 1.5V)
	| REFON // Reference generator on
			| ADC10SHT_2 // 16x ADC Clock cycles
			| ADC10SR // Sampling rate 50ksps
			| ADC10ON // ADC10 on
			| ADC10IE; // Enable interrupts when ADC10MEM gets a result
	ADC10CTL1 = INCH_11 // Input channel select Vcc / 2
	| SHS_0 // ADC10 source clock
			| ADC10DIV_0 // ADC Clock divider select 0
			| ADC10SSEL_0; // ADC10 oscillator

	// start conversion and wait for it
	ADC10CTL0 |= ENC | ADC10SC;

	// Maybe this?
	_BIS_SR(CPUOFF + GIE);
	// LPM0, ADC12_ISR will force exit

//	while (ADC10CTL1 & ADC10BUSY) ;
	// stop conversion and turn off ADC
	ADC10CTL0 &= ~ENC;
	ADC10CTL0 &= ~(ADC10IFG | ADC10ON | REFON);
	raw_value = ADC10MEM;
	// check for overflow
	if (raw_value == 0x3ff) {
		// switch range - use 2.5V reference (Vcc >= 3V)
		ADC10CTL0 = SREF_1 | REF2_5V | REFON | ADC10SHT_2 | ADC10SR | ADC10ON
				| ADC10IE;
		// start conversion and wait for it
		ADC10CTL0 |= ENC | ADC10SC;
//		while (ADC10CTL1 & ADC10BUSY) ;
		_BIS_SR(CPUOFF + GIE);
		raw_value = ADC10MEM;
		// end conversion and turn off ADC
		ADC10CTL0 &= ~ENC;
		ADC10CTL0 &= ~(ADC10IFG | ADC10ON | REFON);
		// convert value to mV
		return ((uint32_t) raw_value * 5000) / 1024;
	} else
		return ((uint32_t) raw_value * 3000) / 1024;
}
