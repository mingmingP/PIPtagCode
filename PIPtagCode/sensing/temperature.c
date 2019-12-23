/*
 * temperature.c
 *
 *  Created on: Apr 23, 2014
 *      Author: romoore
 */

#include "temperature.h"

extern uint16_t numTemp;
uint8_t* temp16_location;
uint16_t cached_temp16F = 0;

/*
 * Raw ADC value for temperature.
 */
int adc10_value;
/*
 * Temperature conversion slope value.
 */
double temperatureSlope = 0;

/*
 * Temperature conversion offset value.
 */
double temperatureOffset = 0;

//Use when creating code for the flasher utility.
//DEFAULT_SLOPE and DEFAULT_OFFSET are unique strings
//that can be searched and replaced using a hex editor without recompiling.
//Following two lines need to be commented out when programming with a
//device-specific calibration value for each.
unsigned long notTempSlope = DEFAULT_SLOPE;
unsigned long notTempOffset = DEFAULT_OFFSET;

float lastTemp = 0;
float lastCalibrateTemp = 0;

/*
 * Location where the 7 bit temperature should go if it is resampled
 * Used in the doTemp7Binary function.
 */
uint8_t* temp7_location = 0;

/*
 * Storage for the last sampled 7-bit temperature value
 */
uint8_t last_temp7;
/*
 * Initializes the temperature slope and offset values.
 */
void initTemperature() {
	//Following two lines are for the flasher.
	//Values are set with hex editor after compilation.
	//Comment out following two lines when using code for programming individual tags using Code Composer.
	temperatureSlope = *(double*) &notTempSlope;
	temperatureOffset = *(double*) &notTempOffset;

	//When programming with device specific calibration values,
	//comment out two lines above and uncomment out the following two lines.
	//Numerical calibration values are set in temperature.h

	//temperatureSlope = DEFAULT_SLOPE;
	//temperatureOffset = DEFAULT_OFFSET;
}

int recalRadioFromTemp() {
	float diff = lastTemp - lastCalibrateTemp;

	if (abs(diff) > CALIBRATE_TEMP_DIFF) {
		lastCalibrateTemp += diff;
		return 1;
	}
	return 0;
}

/*
 * Configure the ADC for temperature sensing.
 */
void configADC() {
	/*
	 * Comment Format:		//Bits modified	//Bits' function				//Bits modified to...
	 */

	ADC10CTL0 = SREF_1 //Bits 15-13	//Select Reference				//R+ = VREF+ and VR- = VSS
	+ REFON 	//Bit 5			//Reference generator on		//Reference on
			+ ADC10SR + ADC10ON //Bit 4			//ADC10 on						//ADC10 on
			+ ADC10SHT_3 //Bits 12-11	//ADC10 sample-and-hold time	//64 � ADC10CLKs
			+ ADC10IE; //Enable interrupts when the ADC10MEM register gets a conversion result

	ADC10CTL1 = INCH_10	//Bits 15-12	//Input channel select			//Temperature sensor
	+ ADC10DIV_3;		//Bits 7-5		//ADC10 clock divider			///4
}

// ADC10 interrupt service routine so we sleep while waiting for the conversion to finish
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void) {
	adc10_value = ADC10MEM;	//temporarily hold output value while shutting down adc.
	_BIC_SR_IRQ(CPUOFF);
}

/*
 * Retrieve the ADC value and calculate the current temperature.
 */
float getTemperature() {
	++numTemp;
//	shiftTempArray();
	configADC();

	__delay_cycles(100);	//wait for Vref to settle.  30 microseconds needed.

	ADC10CTL0 |= ENC//Bit 1		//Enable conversion				//ADC10 enabled
	+ ADC10SC;//Bit 0			//Start conversion				//Start sample-and-conversion

	_BIS_SR(CPUOFF + GIE);
	// LPM0, ADC12_ISR will force exit

	ADC10CTL0 &= ~ENC;  //Bit 1			//Enable conversion bit--ADC10 disabled

	ADC10CTL0 &= ~(REFON | ADC10ON); //turn off Reference and ADC.  Needed for low sleep power

	// Save quarter of new value in array
	lastTemp = (temperatureSlope * adc10_value + temperatureOffset
			+ 40);
	//the reported temperature is shifted up 40 degrees
	//Valid temperature range -40C to 87C
	//Matches chip temperature range.

	return lastTemp;
}

void updateTemp16F(bool updateCached) {
	if (updateCached) {
		cached_temp16F = to16Fixed(getTemperature());
	} else {
		cached_temp16F = to16Fixed(lastTemp);
	}
//	temp16_location[0] = ((uint8_t*) &cached_temp16F)[1];
//	temp16_location[1] = ((uint8_t*) &cached_temp16F)[0];
}

void doSenseTemp16F() {
	static bool first = true;

	if (first) {
		first = false;
		cached_temp16F = to16Fixed(getTemperature());
	}

	temp16_location = malloc(2);
	temp16_location[0] = ((uint8_t*) &cached_temp16F)[1];
	temp16_location[1] = ((uint8_t*) &cached_temp16F)[0];
}

void updateTemp7() {
	if (0 != temp7_location) {
		last_temp7 = round(getTemperature());
		temp7_location[0] = (last_temp7 << 1) | (temp7_location[0] & 0x01);
	}
}

void setLastTemp(float setVal) {
	lastTemp = setVal;
}
