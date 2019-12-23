/*
 *	moisture.c
 *
 *  Created on: Apr 23, 2014
 *      Author: Jakub Kolodziejski
 *
 *
 *  Capacitive Moisture Sensing
 *
 *	Use a capacitive moisture sensor, either a commercial unit or any that uses the high dielectric
 *	constant of water to determine moisture level. Works also with air humidity sensors that act on
 *	the same principle.
 *
 *
 *	--- TPIPK1.1 Circuit Design ---
 *
 *	        R        probe
 *	  +---/\/\/---+----||----+
 *	  |           |          |
 *	charge      sense       GND
 *	 pin         pin
 *
 *	Description		MSP430 Pin		Tag AUX Pin
 *	Ground			GND				1
 *	CHARGE_PIN		1.1				8
 *	SENSE_PIN		1.2				9
 *
 *	A resistor "R" (SMD size 0603 or 0804) is placed between pins 8 and 9 of the tag's AUX connector.
 *	The moisture sensor is connected between pin 9 and pin 1 (GND) of the tag's AUX connector.
 *	If the moisture sensor is polarized, connect its ground to pin 1 and its positive side to pin 9.
 *
 *	For the circuit board soil moisture sensor (range 45-200pF), the R can be 200-400kOhms depending
 *	on Timer_A clock frequency, energy usage, noise, and resolution needed.
 *
 *	DO NOT use an R value greater than 1MOhm because the resulting readings will no longer fall on a
 *	normal distribution. Instead the values will start to form two separate peaks (two offset humps).
 *
 *
 *	--- How It Works ---
 *
 *	Short Version:
 *	Measure the time it takes to charge the probe from ground to a reference voltage (ie. 1/2 Vcc).
 *	Take and average multiple measurements (to reduce effecots of noise) in multiples of 2 (to remove
 *	Comparator related offsets). Translate the average measurement value based on 2x2-point calibration
 *	data and desired scale (ie. 0-100% moisture content) before transmitting.
 *
 *	Long Version:
 *	1)	The configuration state of all pins and modules to be used is saved ("original" state)
 *	2)	Pins and modules to be used are configured appropriately.
 *	3)	CHARGE_PIN and SENSE_PIN are shorted to ground to discharge the sensor quickly.
 *	4)	SENSE_PIN is reconfigured for sensing using the onboard comparator.
 *		CHARGE_PIN remains grounded to keep sensor discharged.
 *	5)	To minimize the effects of nominal battery voltage and temperature, Timer_A's clock is sourced
 *		from the CC1150 radio's crystal via GDO0. The frequency is selected using CM_CRYSTAL_DIVIDER.
 *		See "settings.h" for configuration and list of available frequencies.
 *	6)	Timer_A is set to start counting up from near its maximum allowed value (CM_MEASUREMENT_TIME)
 *		and sets the CHARGE_PIN high when it rolls over back to 0. This synchronizes the "start" of
 *		measurement to the moment charging of the moisture sensor beings.
 *	7)	Timer_A is started, CHARGE_ PIN is set high, sensor is being charged, MSP430 is put into LPM_3
 *		(LPM_1 if using the DCO instead of the radio's crystal)
 *	8)	When the Comparator detects the voltage across the probe has reached 1/2 Vcc (based on internal
 *		reference), it triggers a capture of Timer_A's current count to TA0CCR1 and wakes up the MSP430.
 *		The internal reference can be set to 1/4 Vcc if using capacitance values larger than a few
 *		hundred pF to ensure the measured region is fairly linear and to reduce measurement time which
 *		is also limited by the maximum value Timer_A can count to (65535 clock counts).
 *	9)	The value of TA0CCR1 is added to a running total and the counter of the number of measurements
 *		taken so far (measurement_count) is incremented. If the number of measurements already taken
 *		is less than CM_NUM_OF_MEASUREMENTS then go to step #3 (Comparator inputs are swapped on every
 *		measurement to remove the effects of Comparator offset when measurements are averaged in
 *		multiples of 2).
 *	10) Turn off the radio and its crystal clock output through GDO0
 *	11) The desired number of measurements has been taken (as defined by CM_NUM_OF_MEASUREMENTS in
 *		"settings.h"). Divide the running total by the number of measurements taken to get the average.
 *	12) The average measurement value is translated using 2x2-point calibration data for measurement count
 *		vs. the nominal battery voltage in mV (as measured and saved into cached_battery). This is then
 *		further translated to a scale defined for the calibration data (ie. lower 2 points correspond
 *		to 0% and upper 2 points to 100% moisture level). The scale values can be anything, such as 25%
 *		and 75% for a humidity sensor, or even actual capacitance values in pF.
 *	13) All pins and MSP430 modules used are returned to their "original" state as saved in step #1.
 *	14) The finalized and scaled measurement is then saved to the transmit buffer.
 *
 *
 *	--- Calibration & Scaling ---
 *
 *	Soil moisture:
 *	Use sensor in soils of known moisture content.
 *
 *	Humidity:
 *	Use saturated salts in enclosed container.
 *
 *	How It Works:
 *	A 2x2 point calibration and scaling function is used to remove offsets in the measurement values
 *	and to translate the resulting measurement onto a desired scale (eg. 0 to 100% humidity). There
 *	exist offsets between different pairs of tags and sensors due to differences in tag/sensor
 *	manufacture as well as those inherent in the different functional modules of the MSP430 such as
 *	the comparator and the measurement of Vcc.
 *
 *	The calibration depends on 4 data points, each corresponding to a known number of timer counts at
 *	a known nominal battery voltage (realistically this is the voltage across the large C3 capacitor on
 *	the TPIP-K 1.1). These 4 data points are split into 2 pairs. The first pair corresponds to a high
 *	capacitance value of the sensor close or equal to the top of the desired measurement scale. The
 *	second pair corresponds to a low capacitance value of the sensor close or equal to the bottom of the
 *	desired measurement scale. For each pair, one point will correspond to the number of timer counts at
 *	a relatively "low" battery voltage (eg. 2.2V) while the other will correspond to the number of timer
 *	counts at a relatively "high" battery voltage (eg. 3.2V) for the same real capacitance value of the
 *	sensor (ie. whatever the sensor's capacitance is at 25% and 75% RH). Additionally, each pair is also
 *	defined with a particular scale value that it corresponds to (eg. 25% RH for one pair and 75% RH for
 *	the other).
 *
 *	1)	Calculate the negative inverse of the slope for each calibration point pair. Negative inverse is
 *		necessary to maintain a positive large value so as not to lose precision in later calculations,
 *		this also allows the use of unsigned variables in calculation steps.
 *	2)	Calculate the "y-axis intercept" if a line were made through a point pair and the y-axis
 *		corresponded to the number of counts.
 *	3)	Having a known battery voltage (stored in cached_battery by doBatterySense()) calculate the
 *		points on the calibration pair lines (AB,CD in the figure below) that would correspond to that
 *		battery voltage.
 *	4)	Calculate the calc_scaler factor which prevents loss of precision in calculation steps that would
 *		otherwise result in very small numbers where the fractional component is important. Also, it
 *		prevents overflowing the bounds of unsigned integers in steps where very large numbers result.
 *	5)	Calculate the relative position of the averaged measurement count with respect to the two points
 *		found in step #3.
 *	6)	Using this relative position and the known scale position of the calibration points, calculate
 *		the corresponding position of the measurement value on the measurement scale.
 *	7)	Return the value resulting after calibration and scaling is applied.
 *
 *	        |
 *	        |	  high capacitance
 *	    c   |      A------------B
 *	    o   |
 *	    u   |
 *	    n   |
 *	    t   |     low capacitance
 *	    s   |      C------------D
 *	        |
 *	        |_________________________
 *	       0       |            |
 *	              2200         3200
 *	                voltage in mV
 *
 *
 *	Corresponding to some known position on the desired scale (ie. 75%) = CM_BIG_C_SCALED
 *
 *	A = at a known "low" battery voltage, how many counts does the high capacitance correspond to
 *	  = (CM_BIG_C_LO_BATT, CM_BIG_C_LO_COUNT)
 *
 *	B = at a known "high" battery voltage, how many counts does the high capacitance correspond to
 *	  = (CM_BIG_C_HI_BATT, CM_BIG_C_HI_COUNT)
 *
 *	Corresponding to some known position on the desired scale (ie. 25%) = CM_SMALL_C_SCALED
 *
 *	C = at a known "low" battery voltage, how many counts does the low capacitance correspond to
 *	  = (CM_SMALL_C_LO_BATT, CM_SMALL_C_LO_COUNT)
 *
 *	D = at a known "high" battery voltage, how many counts does the low capacitance correspond to
 *	  = (CM_SMALL_C_HI_BATT, CM_SMALL_C_HI_COUNT)
 *
 *
 *
 *
 *	TODO - write how to perform calibration
 *
 *
 *
 *	See "settings.h" for parameter definitions and configuration.
 */

#include "moisture.h"
#include "../interrupt.h"

uint16_t cached_moisture = 0;	// the saved value of the previous measurement
uint8_t* moisture_location;		// pointer to the location used in the transmit buffer for moisture data

/*
 *	Moisture Sensing Probe (main function)
 *	Author: Jakub Kolodziejski
 *
 *	Sense moisture level (capacitance) from probe and save value in transmit buffer.
 *
 *
 *	Call to doMoistureSense(true) performs a measurement and saves a new measurement value in the
 *	transmit buffer.
 *
 *	Call to doMoistureSense(false) puts the cached value of the last performed measurement in the
 *	transmit buffer.
 */

void doMoistureSense(bool update) {
	// If called with update=true, perform a measurement
	if (update) {
		cached_moisture = 0; // Considered an "invalid" value

		// *** Save state of all registers modified by the moisture sensing code ***
		uint16_t original_TA0CCR0 = TA0CCR0;
		uint16_t original_TA0CCR1 = TA0CCR1;
		uint16_t original_TA0CCTL0 = TA0CCTL0;
		uint16_t original_TA0CCTL1 = TA0CCTL1;
		uint16_t original_TA0CTL = TA0CTL;
		uint16_t original_CACTL1 = CACTL1;
		uint16_t original_CACTL2 = CACTL2;
		uint16_t original_CAPD = CAPD;
		uint16_t original_P1DIR = P1DIR;
		uint16_t original_P1IE = P1IE;
		uint16_t original_P1OUT = P1OUT;
		uint16_t original_P1REN = P1REN;
		uint16_t original_P1SEL = P1SEL;
		uint16_t original_P1SEL2 = P1SEL2;
		uint16_t original_P2DIR = P2DIR;
		uint16_t original_P2IE = P2IE;
		uint16_t original_P2OUT = P2OUT;
		uint16_t original_P2REN = P2REN;
		uint16_t original_P2SEL = P2SEL;
		uint16_t original_P2SEL2 = P2SEL2;

		// Initialize probe related pins and hardware
		// Note that the TimerA clock which is used for the measurement is sourced from the XTAL on the CC1150.
		// It can be sourced from SMCLK if desired--but there are issues of temperature and Vcc variation unless the MSP has a crystal of its own.
		cm_initMoistureSense();

		// Measure moisture level
		// Returns average counts with compensation for Vcc and optionally for capacitance offset.
		cached_moisture = cm_getMoistureLevel();

		// *** Restore all registers modified by the moisture sensing code back to their previously saved state ***
		TA0CTL = original_TA0CTL;
		TA0CCTL0 = original_TA0CCTL0;
		TA0CCTL1 = original_TA0CCTL1;
		TA0CCR0 = original_TA0CCR0;
		TA0CCR1 = original_TA0CCR1;
		CACTL1 = original_CACTL1;
		CACTL2 = original_CACTL2;
		CAPD = original_CAPD;

		P1IE = original_P1IE;
		P1DIR = original_P1DIR;
		P1REN = original_P1REN;
		P1SEL = original_P1SEL;
		P1SEL2 = original_P1SEL2;
		P1OUT = original_P1OUT;

		P2IE = original_P2IE;
		P2DIR = original_P2DIR;
		P2REN = original_P2REN;
		P2SEL = original_P2SEL;
		P2SEL2 = original_P2SEL2;
		P2OUT = original_P2OUT;
	}
	// Save value to transmit buffer
	moisture_location = malloc(2);
	moisture_location[0] = ((uint8_t*) &cached_moisture)[1];
	moisture_location[1] = ((uint8_t*) &cached_moisture)[0];

}

/*
 * Moisture Sensing Probe (comparator & charge to 1/2 Vcc based)
 * Author: Jakub Kolodziejski
 *
 *         R       probe
 *    +---/\/\/---+----||----+
 *	  |           |          |
 *	charge      sense       GND
 *	 pin         pin
 *
 *	Description		MSP430 Pin		Tag AUX Pin
 *	Ground			GND				1
 *	CHARGE_PIN		1.1				8
 *	SENSE_PIN		1.2				9
 *
 *	For mositure sensor, R ~ 200-400kOhms depending on clock frequency, energy usage, and resolution needed.
 *
 *	CHARGE_PIN and SENSE_PIN defined in "sensing.h"
 *
 *	NOTE: Comment out the call to opticalReceive(&params); in SendBeacon.c, otherwise there is an extra current draw
 *	      of ~0.5mA during subsequent individual measurements (seems to be a conflict in usage of Timer0_A).
 *	      Check status of this before further development.
 */

// Initialize probe related pins and hardware
void cm_initMoistureSense(void) {

	// *** Initialize probe related pins ***
	// Configure Charge pin...
	P1IE &= ~CHARGE_PIN;	// Disable interrupt
	P1REN &= ~CHARGE_PIN;	// Disable pullup/pulldown resistor
	P1DIR |= CHARGE_PIN;	// Set as output
	P1SEL |= CHARGE_PIN;	// Set to TA0.0 output (TA0.0. is used controls charge/discharge via Timer0_A)
	P1SEL2 &= ~CHARGE_PIN;	// ^ see above ^
	P1OUT &= ~CHARGE_PIN;	// Not necessary for functionality, just keeps the pin low in case it is not configured as TA0.0
	CAPD &= ~CHARGE_PIN; 	// Enable GPIO buffer on CHARGE pin
							// When configure pin to be comparator, disconnect GPIO buffer automatically
							// When reconfigure as I/O pin, need buffer configured.
							// This line guarantees that it has the buffer properly configured.
	// Configure Sense pin...
	P1IE &= ~SENSE_PIN;		// Disable interrupt
	P1REN &= ~SENSE_PIN;	// Disable pullup/pulldown resistor
	P1DIR |= SENSE_PIN;		// Set as output
	P1SEL &= ~SENSE_PIN;	// Set as GPIO
	P1SEL2 &= ~SENSE_PIN;	// ^ see above ^
	P1OUT &= ~SENSE_PIN;	// Set low, for quick discharge of capacitor between measurements
	CAPD &= ~SENSE_PIN;		// Enable GPIO buffer on SENSE pin.
							//Because it is set output low with no pulldown, it "instantly" (t < 50ns) discharges the capacitor

	// *** Configure Comparator_A+ for capacitor measurement ***
	CACTL1 = CAREF_2;		// 1/2 Vcc ref on + pin
	CACTL2 = P2CA2 + CAF;	// Input CA2 on - pin, filter output

	// *** Configure Timer0_A for capacitor measurement ***
	TA0CTL = TACLR;	// Stops Timer0_A and resets all parameters including TAR, clock divider, and count direction

	//Below is setup for using the XTAL on the radio through GDO0
	#if CM_USE_CRYSTAL
			TA0CTL = TASSEL_0 + ID_0 + MC_0;// Use TA0CLK, no division, stopped mode
#else
	TA0CTL = TASSEL_2 + ID_0 + MC_0;	// Use SMCLK, no division, stopped mode
#endif	// CM_USE_CRYSTAL?
	TA0CCTL0 = CCIS_2;// CCIS_2 set to GND, set OUTMOD_0, OUT bit to 0 (discharges capacitor through CHARGE pin)
	TA0CCTL1 = CCIS_1 + SCS + CAP;// Use CCI1B (comparator output) for capture/compare trigger, synchronize capture, set to capture mode
	TA0CCTL2 = CCIS_2;// CCIS_2 set to GND, all else cleared to prevent unexpected things from happening
	TA0CCR0 = CM_MEASUREMENT_TIME;// Maximum number of counts to permit for a measurement before it is considered a failure (too high capacitance or resistance) NOTE: Actual time is affected by the clock frequency used for Timer0_A
	TA0CCR1 = 0;// Clear capture register to prevent unexpected effects on first measurement
	TA0CCR2 = 0;			// Clear to prevent unexpected things from happening
}

// Measure moisture level
uint16_t cm_getMoistureLevel(void) {

#if CM_USE_CRYSTAL
	crystalOn(CM_CRYSTAL_DIVIDER);// Turn ON radio with crystal oscillator outputting desired frequency on GDO0 to P1.0
#endif	// CM_USE_CRYSTAL?
	// *** Take control of Timer0_A1 ***
	sensingMoisture = 1;	// Set to 1 to take control of Timer0_A1

	// Prepare measurement taking variables
	uint16_t measurement_count = 0;			// Number of measurements taken
	uint16_t measurement_sum = 0;// Sum of measurements taken (to be averaged)

	// Perform multiple measurements and record them into measurement_sum
	while (measurement_count < CM_NUM_OF_MEASUREMENTS) {

		// *** Prepare for charging and subsequent measurement following time CM_MEASUREMENT_WAIT ***
		//	Reconnect the SENSE pin to the Comparator (following the use of the SENSE pin to quickly discharge the capacitor)
		//	Configure Comparator for measurement
		//	Configure Timer0_A for charge control and count measurement
		CAPD |= SENSE_PIN;// Disable GPIO buffer on SENSE pin (this is just a precaution, according to the User's Guide this is unnecessary if a pin is defined as a Comparator input using P2CAx bits)
		CACTL2 |= P2CA2;					// Reconnect SENSE pin to Comparator
		CACTL1 ^= CAEX;	// Swap Comparator inputs and invert it's output (averaging 2^n successive measurements with this swap in between will cancel out the Comparator's offset voltage)
		CACTL1 |= CAON;							// Turn on comparator
		TA0R = 0xFFFF - CM_MEASUREMENT_WAIT;// Offset Timer0_A starting point to wait a given amount of counts before first measurement is taken (Used to ensure capacitor is discharged). NOTE: Actual time is affected by the clock frequency used for Timer0_A
		TACCTL1 |= CM_2 + CCIE;	// Set Timer_A1 capture on falling edge, enable interrupt
		TACCTL0 = OUTMOD_1;	// Set CHARGE pin high on next "overflow" to start charging the capacitor at the same time as the Timer starts counting from 0

		// *** Start Timer0_A ***
		//	While the timer is running, the capacitor is first held discharged (CHARGE pin is low) for CM_MEASUREMENT_WAIT counts.
		//	When the timer overflows, the CHARGE pin is set high and the Comparator will trigger a Timer count capture to TA0CCR1 when the voltage across the capacitor reaches Vcc/2
		//	This method synchronizes the Timer counting from 0 to the start of capacitor charging, eliminating the possiblility of software delay offsets.
		TA0CTL |= MC_1;			// Start Timer0_A in UP mode (count to TA0CCR0)

		// *** Sleep CPU while "discharge wait" and "charge/measurement" occur ***
#if CM_USE_CRYSTAL
		_BIS_SR(LPM3_bits + GIE);
		// Enter low power mode (LPM3) w/ CPU interrupts
#else
		_BIS_SR(LPM1_bits + GIE);
		// Using DCO, must stay in LPM1 or the SMCLK is shut down.
		// Enter low power mode (LPM1) w/ CPU interrupts
#endif	// CM_USE_CRYSTAL?
		// Waiting for capacitance to be measured...
		//	(when the Comparator triggers at Vcc/2 on the SENSE pin, Timer0_A1 interrupt captures the measurement count and wakes up the CPU for normal code execution)
		//  Note that the interrupt used is the same as used for the light sensor code.
		// *** Discharge capacitor ***
		//	The CHARGE pin is pulled low to discharge and hold the capacitor at ground until the next measurement period begins.
		//	The SENSE pin is disconnected from the Comparator and pulled low to "instantly" (t < 50ns) discharge the capacitor.
		TA0CCTL0 = OUTMOD_0;// Sets CHARGE pin low immediately to discharge capacitor
		CACTL2 &= ~P2CA2;// Disconnect SENSE pin from Comparator for quick discharge of capacitor when GPIO buffer is enabled
		CAPD &= ~SENSE_PIN;	// Enable GPIO buffer on SENSE pin. Because it is set output low with no pulldown, it "instantly" (t < 50ns) discharges the capacitor

		// *** Record measurement ***
		measurement_count++;		// Increment measurement counter
		measurement_sum += TA0CCR1;	// Add current measurement to sum total
		TA0CCR1 = 0;// Clear capture register to prevent unexpected effects on next measurement
	}

	// *** All measurements completed ***
	//	Timer0_A and Comparator are already turned off after the last measurement is taken
	//	Capacitor is already discharged after the last measurement is taken
	//	Reset all remaining hardware and pins used for measurement
#if CM_USE_CRYSTAL
	crystalOff();			// Turn OFF radio crystal oscillator on P1.0 (GDO0)
#endif	// CM_USE_CRYSTAL?
	sensingMoisture = 0;	// Release control of Timer0_A1

	TA0CTL = TACLR;			// Resets all parameters of Timer0_A including TAR, clock divider, and count direction

	// *** Process measurements ***
	uint16_t measurement = 0;		// Measurement value. Initialize to zero.

	// Average the measurements to remove comparator offset and reduce noise contributions
	measurement = measurement_sum / CM_NUM_OF_MEASUREMENTS;	// Take average of measurements

	// 2x2-Point Calibrated Correction for Measurement Counts vs Battery Voltage
	/*
	 * This section makes use of 2-point calibration data for a low and a high capacitance (for a total
	 * of 4 calibration points) to account for the dependence of the measured count value on Vcc.
	 * Additionally, the result is scaled over a desired range (ie. 0 to 100 for percentage in 1%
	 * increments) before being passed on for transmission.
	 *
	 * See "settings.h" for parameter definitions
	 */
	/* Calculate the negative inverse of the slope and the intercept of the line defined by
	 * the upper calibration bound points.
	 */
	uint16_t big_c_slope = (CM_BIG_C_HI_BATT - CM_BIG_C_LO_BATT) / (CM_BIG_C_LO_COUNT - CM_BIG_C_HI_COUNT);
	uint16_t big_c_intercept = CM_BIG_C_LO_COUNT + (CM_BIG_C_LO_BATT / big_c_slope);
	/* Calculate the negative inverse of the slope and the intercept of the line defined by
	 * the lower calibration bound points.
	 */
	uint16_t small_c_slope = (CM_SMALL_C_HI_BATT - CM_SMALL_C_LO_BATT) / (CM_SMALL_C_LO_COUNT - CM_SMALL_C_HI_COUNT);
	uint16_t small_c_intercept = CM_SMALL_C_LO_COUNT + (CM_SMALL_C_LO_BATT / small_c_slope);
	/* Calculate the upper and lower calibration bound points as defined by the above slopes
	 * and intercepts which correspond to the current known battery voltage (mV).
	 */
	uint16_t big_c_count = big_c_intercept - (cached_battery / big_c_slope);
	uint16_t small_c_count = small_c_intercept - (cached_battery / small_c_slope);

	// Measurement value translated to the scale defined in "settings.h". Initialize to zero.
	uint16_t scaled_value = 0;
	uint16_t Ptop = 0;
	int Pbottom = 0;
	uint16_t calc_scaler = 0;
	int P = 0;

	if(measurement == small_c_count) {	// measurement = lower calibration bound
		/* Just return the lower bound scaled equivalent because further math is unnecessary
		 * and this also avoids having to deal with a divide by zero in those calculations.
		 */
		scaled_value = CM_SMALL_C_SCALED;
	}
	else {	// measurement != lower calibration bound
		/* Calculate the normalized position of the measurement within the range defined by
		 * the upper and lower calibration bounds. The top and bottom half of the equation
		 * are calculated separately because Ptop is useful for calculating calc_scaler and
		 * this splitting reduces having to calculate it separately.
		 */
		Ptop = (big_c_count - small_c_count);

		/* calc_scaler is used in subsequent calculations to maximize precision lost with
		 * the use of integer variables and math when the results of individual calculation
		 * steps are very small (<10) and heavily depend on the fractional component of the
		 * calculations. It is necessary to limit the calc_scaler so that in multiplication
		 * it does not result in a value greater than +/- 32767 else it will be beyond the
		 * bounds of an int causing garbage results.
		 */
		calc_scaler = 65535 / Ptop;
		/* Calculate the normalized position of the measurement within the range defined by
		 * the upper and lower calibration bounds.
		 */
		if(measurement > small_c_count) {
			Pbottom = (measurement - small_c_count);
			P = (calc_scaler * Ptop) / Pbottom;
			// Translate the measured value to the desired scale defined in "settings.h"
			scaled_value = CM_SMALL_C_SCALED + (calc_scaler * (CM_BIG_C_SCALED - CM_SMALL_C_SCALED)) / P;
		}
		else {	// measurement < small_c_count, NOTE: the equal case is taken care of before
			Pbottom = (small_c_count - measurement);
			P = (calc_scaler * Ptop) / Pbottom;
			// Translate the measured value to the desired scale defined in "settings.h"
			scaled_value = CM_SMALL_C_SCALED - (calc_scaler * (CM_BIG_C_SCALED - CM_SMALL_C_SCALED)) / P;
		}
	}

#if CM_RAW_MEASUREMENT	// Return raw measurement value in timer counts WITHOUT calibration and scaling applied (useful for testing and manual calibration)
	return measurement;
#else
	// Perform min/max scale limit safety checks and force the value to within the limits if it is out of the lower and upper bounds (limits set in "settings.h")
	if(scaled_value < CM_MIN_SCALED) {
		return CM_MIN_SCALED;
	}
	else if(scaled_value > CM_MAX_SCALED) {
		return CM_MAX_SCALED;
	}

	// Return the measured value with calibration and scaling applied
	return scaled_value;
#endif
}

/*
 *	Radio Crystal as Timer0_A clock source (TA0CLK)
 *	Author: Jakub Kolodziejski
 *
 *	Allows the use of the CC1150 radio's crystal oscillator as the clock source TA0CLK on P1.0 for
 *	Timer0_A. The default crystal oscillator frequency is 26MHz but output to TA0CLK via GDO0 can be
 *	divided down to a desired frequency (see "settings.h" for proper settings). To wake up the radio
 *	and enable output of the desired clock frequency on GDO0, call crystalOn(x) where "x" is one of
 *	the CRYSTAL_DIV_# defined in "settings.h". When finished with using the radio's crystal, call
 *	crystalOff() to turn off the output to GDO0 and shutdown the radio.
 *
 *	Available Frequencies (and their respective dividers):
 *	NOTE: something like 0.3_ = 0.33333... (repeating decimal)
 *		Divider set to 1	->	 f = 26 MHz
 *		Divider set to 1.5	->	 f = 17.3_ MHz
 *		Divider set to 2	->	 f = 13 MHz
 *		Divider set to 3	->	 f = 8.6_ MHz
 *		Divider set to 4	->	 f = 6.5 MHz
 *		Divider set to 6	->	 f = 4.3_ MHz
 *		Divider set to 8	->	 f = 3.25 MHz
 *		Divider set to 12	->	 f = 2.16_ MHz
 *		Divider set to 16	->	 f = 1.625 MHz
 *		Divider set to 24	->	 f = 1.083_ MHz
 *		Divider set to 32	->	 f = 812.5 kHz
 *		Divider set to 48	->	 f = 541.6_ kHz
 *		Divider set to 64	->	 f = 406.25 kHz
 *		Divider set to 96	->	 f = 270.83_ kHz
 *		Divider set to 128	->	 f = 203.125 kHz
 *		Divider set to 192	->	 f = 135.416_ kHz
 *
 *
 *	--- Current Consumption ---
 *
 *	When the MSP430 is put into LPM_3 while the radio crystal is on, the sleep current can be estimated
 *	roughly as follows:
 *                                   GDO0 Frequency (MHz)
 *     Sleep Current (mA) = 1.16 +  ----------------------
 *                                            13
 *
 *	So for a 13MHz clock on GDO0 (CRYSTAL_DIV_2) the sleep current will be about 2.16mA
 *	This is only a rough estimate based on collected data fitted with a linear plot. Actual value may
 *	be off by as much as 0.1mA (0.04mA on average).
 */

// Wakes up radio and turns on output of desired frequency to P1.0 (TA0CLK)
void crystalOn(uint8_t divider) {
	// Configure P1.0 (GDO0) to use radio's crystal oscillator for TA0CLK
	P1IE &= ~BIT0;		// Disable interrupt
	P1DIR &= ~BIT0;		// Set as input
	P1REN &= ~BIT0;		// Disable pullup/pulldown resistor
	P1SEL |= BIT0;		// Set to TA0CLK (TimerA_0 clock source)
	P1SEL2 &= ~BIT0;	// ^ see above ^
	P1OUT &= ~BIT0;		// Set low
	CAPD &= ~BIT0;		// Enable GPIO buffer

	// Configure P2.6 so it doesn't interfere with clock signal to P1.0
	P2IE &= ~BIT6;		// Disable interrupt
	P2DIR &= ~BIT6;		// Set as input
	P2REN &= ~BIT6;		// Disable pullup/pulldown resistor
	P2SEL &= ~BIT6;		// Set to GPIO
	P2SEL2 &= ~BIT6;	// ^ see above ^
	P2OUT &= ~BIT6;		// Set low

	// Wake CC1101 and wait for oscillator to stabilize
	Wake_up_CC1101();
	// Turn on oscillator with desired frequency being output to P1.0 (GDO0)
	// (typ. current consumption 1.1mA)
	TI_CC_SPIWriteReg(TI_CCxxx0_IOCFG0, divider);
}

// Stops crystal oscillator output from radio and puts radio to sleep
void crystalOff(void) {
	// Reset GDO0 to high impedence tri-state as set by ReWriteCC1101Registers() in main.c
	TI_CC_SPIWriteReg(TI_CCxxx0_IOCFG0, 0x2E); // GDO0 output pin to high impedance 3-state
	// Turn off crystal oscillator (typ. current consumption 0.22 mA)
	TI_CC_SPIStrobe(TI_CCxxx0_SXOFF);
	// Power down CC1101 when Csn goes high (typ. current consumption 200nA)
	TI_CC_SPIStrobe(TI_CCxxx0_SPWD);
}
