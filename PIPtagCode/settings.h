/*
 * settings.h
 *
 *  Created on: Aug 15, 2013
 *      Author: romoore
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_
//Power Amplifier table size and settings
#define PA_TBL_SIZE 			1

#define PWR_9_9_dBm 		0xC0    // PATABLE (9.9 dBm output power, 29.3 mA)	per SWRA 150 design note.
#define PWR_8_0_dBm 		0xE5    // PATABLE (8.0 dBm output power, 26.0 mA)	per SWRA 150 design note.
#define PWR_6_0_dBm 		0xEB    // PATABLE (6.0 dBm output power, 23.4 mA)	per SWRA 150 design note.
#define PWR_4_0_dBm 		0x84    // PATABLE (4.0 dBm output power, 18.6 mA)	per SWRA 150 design note.
#define PWR_3_0_dBm 		0xA7    // PATABLE (3.0 dBm output power, 17.9 mA)	per SWRA 150 design note.
#define PWR_1_8_dBm			0x8A	// PATABLE (1.8 dBm output power, 17.1 mA)  per SWRA 150 design note.
#define PWR_0_5dBm 			0x8D	// PATABLE (0.5 dBm output power, 16.3 mA) per SWRA 150 design note.
#define PWR_m0_6dBm 		0x8E	// PATABLE (-0.6 dBm output power, 15.8 mA) per SWRA 150 design note.
#define PWR_m3_1dBm 		0x5B	// PATABLE (-3.1 dBm output power, 14.1 mA) per SWRA 150 design note.
#define PWR_m10_3dBm	  	0x6c	// PATABLE (-10.3 dBm output power, 12.9 mA)per SWRA 150 design note.
#define PWR_m20_0dBm  		0xF		// PATABLE (-20.0 dBm output power, 11.7 mA)per SWRA 150 design note.
#define PWR_m30_2dBm  		0x3		// PATABLE (-30.2 dBm output power, 11.1 mA)per SWRA 150 design note.
#define PWR_m60_4dBm  		0x0		// PATABLE (-60.4 dBm output power, 10.3 mA)per SWRA 150 design note.


/*
 * Timing values based on milliseconds.  Assumes the VLO gets
 * calibrated at some point against a better clock.
 */
#define MS_EIGHTH_SECOND	125
#define MS_QUARTER_SECOND	250
#define MS_HALF_SECOND		500
#define MS_ONE_SECOND		1000
#define MS_TWO_SECOND		2000
#define MS_FOUR_SECOND		4000
#define MS_SIX_SECOND		6000
#define MS_TEN_SECOND		10000
#define MS_FIFTEEN_SECOND	15000
#define MS_ONE_MINUTE		60000
#define MS_NINETY_SECOND	90000
#define MS_TWO_MINUTE		120000
#define MS_ONE_HOUR			3600000

#define GRAIL_FREQ 		902004500		// center of range for rcvr set to 902.1 MHz per measurement REH 1/21/2013
#define DEMO_FREQ  		907022500			// center of range for rcvr set to 907.1 MHz
#define MID_BAND_FREQ 	915000000		//Center of ISM band. For GPIP to TPIP testing
#define FCC_FREQ		922000000		//Approximate mid-range of the upper available band above GSM



//Change the next constants to alter the behavior of your pip

//Insert numerical value for manual assignment of ID.
#define TXER_ID 3377	//0xABBADABA is default ID for automatic flashing

//The maximum delay (in ms) for any of the sense types to complete
//and then wait 10ms to allow the capacitor to recharge.
#define MAX_SENSE_DELAY 150

#define ENABLE_BINARY 0			//Binary event from switch open/close (usually magnetic)
#define ENABLE_TEMP16 0			//Temp sensor on MSP
#define ENABLE_AMBIENT_LIGHT 1	//Using LED on board

	//Set ENABLE_MOISTURE to 1 if doing sensing with external moisture sensor.
#define ENABLE_MOISTURE 0

// How many VLO cycles to use when calibrating it against the 26 MHz crystal
#define VLO_CALIB_COUNTS 100

//Set to use temperature from the HTU21D (or equivalent) temperature and humidity module if available
#define ENABLE_HTU_SENSING 1

// Set to broadcast history
#define ENABLE_HISTORY 0

// Comment the below line to disable CRC in the packet
#define PROTOCOL_VERSION 2	// Version 1 = 21-bit + parity ID, no CRC
							// Version 2 = 24-bit, CRC enabled

// PA table settings for radio.
// Sets transmit power level.
#define DEFAULT_PATABLE PWR_6_0_dBm

// Radio frequency for transmissions
#define DEFAULT_FREQ FCC_FREQ

// How often MSP will wake up and check to see if sensing is required.
// If WAKE_INTVL < PACKTINTVL_MS then MSP can go back to sleep without transmission.
#define WAKE_INTVL MS_TEN_SECOND

// How frequently to transmit a packet when no data is "sensed"
// This will be the "heartbeat" transmission period.
#define PACKTINTVL_MS MS_TEN_SECOND

// If sensing event occurs after waking,
// multiple packets will be transmitted immediately to reduce latency.

// Number of times to repeat a transmission when "detected"
#define SENSE_TX_REPEAT 3
// Interval between "detection" transmissions
#define SENSE_REPEAT_INTVL_0 MS_EIGHTH_SECOND
#define SENSE_REPEAT_INTVL_1 MS_QUARTER_SECOND
#define SENSE_REPEAT_INTVL_2 MS_HALF_SECOND

// Recalibrate radio if temperature changes by this many degrees
#define CALIBRATE_TEMP_DIFF 3
// How frequently to recalibrate the radio even without temperature change.
#define RECAL_INTVL MS_ONE_HOUR

// How frequently to poll the sensed "binary" data
#define BINARY_INTVL MS_TEN_SECOND
// How frequently to poll the temperature sensor on the MSP
#define TEMP_INTVL MS_TEN_SECOND
// Interval between "battery" transmissions
#define SENSE_BATTERY_INTVL MS_ONE_HOUR
// Interval between ambient light level
#define AMBIENT_LIGHT_INTVL MS_TEN_SECOND
// Interval between moisture sensing
#define MOISTURE_INTVL MS_TEN_SECOND

// Interval between temperature and relative humidity measurements using
// the HTU21D (or equivalent) if available.
#define HTU_INTVL MS_TEN_SECOND

//True if the HTU21D (or equivalent) sensor is on-board and can be used for recalibration
#define HTU_ONBOARD true

//In production code where maximum lifetime is needed, disable LED flashing.
//When battery gets low, internal resistance is high enough that flashing shuts down the tag prematurely
#define PRODUCTION false

/*******************
 * History stuff
 *******************/
// 28-hour history version
#define HISTORY_28

#ifdef HISTORY_28
#define HISTORY_INTVL MS_NINETY_SECOND
#endif

/*******************
 * End history stuff
 *******************/

/* End user-configurable stuff */


/* How many wake-up counts between each sensing/calculation operation */
#define BINARY_STEPS (BINARY_INTVL/WAKE_INTVL)
#define TEMP_STEPS (TEMP_INTVL/WAKE_INTVL)
#define AMBIENT_LIGHT_STEPS (AMBIENT_LIGHT_INTVL/WAKE_INTVL)
#define MOISTURE_STEPS (MOISTURE_INTVL/WAKE_INTVL)
#define HTU_STEPS (HTU_INTVL/WAKE_INTVL)
#define BATTERY_STEPS (SENSE_BATTERY_INTVL/WAKE_INTVL)
#define RECAL_STEPS (RECAL_INTVL/WAKE_INTVL)
#define BACKGROUND_STEPS (PACKTINTVL/WAKE_INTVL)
#define HISTORY_STEPS (HISTORY_INTVL/WAKE_INTVL)


/* Moisture sensing */

//Use capacitive moisture sensor, either commercial unit or any kind of probe that uses the high dielectric constant of water.
//For TPIPK1.1, one end of the capacitor is ground (pin 1) and the other end is connected to sense pin (9).
//Sense pin is also connnected through a resistor to pin 8, which is used to charge the capacitor.
//For circuit board sensor, R~ 200-400kOhms.  Not critical, but can be optimized for noise, resolution, and energy.
//First, short capacitor to ground through sense pin and pull charge pin low to keep it low.
//Sense pin reset to sensing function; charge pin is kept low.
//To minimize effects of temperature and Vcc, TimerA clock is sourced from GD00 set to produce the output of the XTAL on the CC1150.
//The XTAL is divided by CM_CRYSTAL_DIVIDER.  A value of 4 gives 6.5 MHz for the clock, which works well for 390 Kohm resistor and soil probe.
//Start TimerA preset to a high value and have it count up.
//When overflows to zero use that to synchronize the start of the charge cycle.
//Charge capacitor with Vcc (circuit board sensor =45 pfd dry and 170 pfd wet) through resistor until voltage reaches the reference voltage (Vcc/2)
//as measured by the sense pin using the comparator.
		//Can choose Vcc/2 or Vcc/4 for reference.
		//Vcc/4 gives lower resolution, but 1/4 the energy of Vcc/2.
//Stop timer and add value to running sum to get average of several measurements.
//Discharge capacitor as before and then swap the inputs of the comparator and repeat.
	//This cancels the input offset voltage of the comparator to first order.
	//Measurements must be multiples of two.
//Note that the fundamental measurement is the time for the voltage to fall to a fraction of Vcc,
//and is not sensitive to Vcc to first order.
	//In practical situations, Vcc drops during a single measurement, making compensation necessary
	//Vcc compensation is done by measuring the nominal battery voltage and using a calibrated correction factor at each value of Vcc.
	//Note that the correction factor includes the fact that the voltage drops during a measurement,
	//so changes in the measurement timing changes the fudge factor.
	//Note that this is best done using an actual battery since the voltage/time profile depends on the charge state of the battery.
	//The internal resistance of the battery can vary from about 20 ohms fresh to about 200 ohms near end of life.
	//Calibrations are done down to 2.2V.
//Multiple pairs of measurements are averaged and then the compensation factor is used to correct for Vcc.
//A second compensation factor is used to correct for the capacitance of the circuit board, the cable, and the pin on the MSP430.
	//Could be set to correct for all of the dry capacitance if desired.
//Initial code returns raw timer counts (2 Bytes)--needs to use individual calibration values to convert to %RH.
	//Conversion to %RH will reduce packet length to 1 Byte.
	//Saturated salts will be used to calibrate humidity sensors.
	//Soil moisture sensor will use known moisture content.

// C_Meter moisture sensing related settings
// Preferred method of sensing.
//	IMPORTANT!!! Make sure that (expected maximum counts to Vcc/2) * NUM_OF_MEASUREMENTS < 65535
//	Running total when summing the measurements.
#define CM_NUM_OF_MEASUREMENTS	4		// Number of measurements to average, must be a factor of 2 to compensate for Comparator offset
#define CM_MEASUREMENT_TIME		0xFFFF	// Max number of timer counts to wait for measurement completion before it is considered invalid (NOTE: affected by clock frequency used for Timer0_A)
#define CM_MEASUREMENT_WAIT		130		// Number of timer counts to wait between measurements to ensure capacitor is properly discharged (NOTE: affected by clock frequency used for Timer0_A, pick a value that gives around 20us for the frequency you choose)

// 2x2-Point Calibrated Correction for Measurement Counts vs Battery Voltage
#define CM_RAW_MEASUREMENT		1		// If set to 1, data transmitted will be the raw timer count value without calibration and scaling applied (useful for testing and manual calibration)
#define CM_MIN_SCALED			0		// Absolute minimum scale value (anything less than this will be reported as this value)
#define CM_MAX_SCALED			500		// Absolute maximum scale value (anything greater than this will be reported as this value)
#define CM_SMALL_C_SCALED		47		// Lower cal. points scale value equivalent
#define CM_SMALL_C_HI_BATT		3081	// Lower cal. point "high battery voltage" measured battery voltage (mV)
#define CM_SMALL_C_HI_COUNT		258		// Lower cal. point "high battery voltage" measured count
#define CM_SMALL_C_LO_BATT		2701	// Lower cal. point "low battery voltage" measured battery voltage (mV)
#define CM_SMALL_C_LO_COUNT		261		// Lower cal. point "low battery voltage" measured count
#define CM_BIG_C_SCALED			180		// Upper cal. points scale value equivalent
#define CM_BIG_C_HI_BATT		3081	// Upper cal. point "high battery voltage" measured battery voltage (mV)
#define CM_BIG_C_HI_COUNT		848		// Upper cal. point "high battery voltage" measured count
#define CM_BIG_C_LO_BATT		2680	// Upper cal. point "low battery voltage" measured battery voltage (mV)
#define CM_BIG_C_LO_COUNT		851		// Upper cal. point "low battery voltage" measured count



// Radio Crystal as Timer_A clock source (TA0CLK)
//	NOTE: something like 0.3_ = 0.33333... (repeating decimal)
#define CRYSTAL_DIV_1		0x30	// Divider set to 1		->	 f = 26 MHz
#define CRYSTAL_DIV_1p5		0x31	// Divider set to 1.5	->	 f = 17.3_ MHz
#define CRYSTAL_DIV_2		0x32	// Divider set to 2		->	 f = 13 MHz
#define CRYSTAL_DIV_3		0x33	// Divider set to 3		->	 f = 8.6_ MHz
#define CRYSTAL_DIV_4		0x34	// Divider set to 4		->	 f = 6.5 MHz
#define CRYSTAL_DIV_6		0x35	// Divider set to 6		->	 f = 4.3_ MHz
#define CRYSTAL_DIV_8		0x36	// Divider set to 8		->	 f = 3.25 MHz
#define CRYSTAL_DIV_12		0x37	// Divider set to 12	->	 f = 2.16_ MHz
#define CRYSTAL_DIV_16		0x38	// Divider set to 16	->	 f = 1.625 MHz
#define CRYSTAL_DIV_24		0x39	// Divider set to 24	->	 f = 1.083_ MHz
#define CRYSTAL_DIV_32		0x3A	// Divider set to 32	->	 f = 812.5 kHz
#define CRYSTAL_DIV_48		0x3B	// Divider set to 48	->	 f = 541.6_ kHz
#define CRYSTAL_DIV_64		0x3C	// Divider set to 64	->	 f = 406.25 kHz
#define CRYSTAL_DIV_96		0x3D	// Divider set to 96	->	 f = 270.83_ kHz
#define CRYSTAL_DIV_128		0x3E	// Divider set to 128	->	 f = 203.125 kHz
#define CRYSTAL_DIV_192		0x3F	// Divider set to 192	->	 f = 135.416_ kHz
// Use the CC110x crystal as the Timer_A clock source for moisture measurements
//	0 = use SMClock, 1 = use crystal
#define CM_USE_CRYSTAL 1
// Radio crystal frequency divider to use for moisture measurement
// With XTAl of 26 MHz and divider of 4, get 6.5 MHz for TimerA clock.
// Be careful to note maximum frequency of MSP450 at the minimum Vcc expected.
// Experimentally, we see that the MSP430 can run up to 26 MHz--with a high current.  Reliability not guaranteed.
#define CM_CRYSTAL_DIVIDER		CRYSTAL_DIV_4

#endif /* SETTINGS_H_ */
