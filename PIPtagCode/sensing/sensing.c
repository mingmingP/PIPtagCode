/*
 * sensing.c
 *
 *  Created on: Aug 15, 2013
 *      Author: romoore
 */

#include "sensing.h"

extern uint16_t numBinary;
extern uint8_t* temp7_location;
extern uint8_t last_temp7;

/*
 * Rounds a double-precision to an integer.
 */
int round(double number) {
	return (number >= 0) ? (int) (number + 0.5) : (int) (number - 0.5);
}

/*
 *   Signed
 *   Whole       Frac
 *   Component   Comp
 * +------------+----+
 * |      12b   | 4b |
 * +------------+----+
 */
uint16_t to16Fixed(const float floatVal) {
	return (uint16_t) (floatVal*16);
}

int senseBinary() {
	++numBinary;
	//For switch sensing output power on pin 1.2 and sense it on pin 3.7
	//Following can be used for binary switch or water sensor.
	//Connectivity is tested between pins 7 (MFIO, P3.7) and 9 (TXD0, P1.2) of "Arduino" connector.
	//Set the lowest bit in the data byte if the input pin sees power.
	//Disable pullup/pulldown on 3.7, set p1.2 to output, and output power
	P1DIR |= 0x04;
	P1REN &= ~(0x04);
	P1OUT |= 0x04;
	P3REN &= ~(0x80);

	TI_CC_Wait(10); //charge input capacitance with current through water.

//Read the sensing pin
//If water, input capacitance discharges through water resistivity.
//Values adjusted so with 47pFd capacitor between pin 7 and pin 1 ("Arduino" connector)
//tap water is sensed, but distilled water is not.
//If using switch (e.g. door sensor), capacitance discharges through contacts.
	int retVal = P3IN & 0x80 ? 0x01 : 0x00;

	//Reset the pins
	P1OUT &= ~(0x04);
	P1DIR &= ~(0x04);
	P1REN |= 0x04;
	P3REN |= 0x80;
	return retVal;
}

bool lastBinary = 0;

bool doTemp7Binary(bool forceUpdate) {
//Make temperature static
	static bool first_call = true;

	uint8_t oldVal = lastBinary;

//Byte for the temperature and binary data.
//temp7_location is kept global so that the updateTemp7
//function can modify the temperature value in the future.
//This supports filling in sensed data, then determining if
//a packet will be transmitted, and only after then deciding
//whether or not to sense temperature.
	temp7_location = malloc(1);
	if (forceUpdate || first_call) {
		lastBinary = senseBinary();
	}
	temp7_location[0] |= lastBinary;
	temp7_location[0] |= last_temp7 << 1;

//If this is the first time the function is called then temperature
//must be sensed.
	if (first_call) {
		first_call = false;
		updateTemp7();
	}

	/*
	 * oldVal is either 0x00 or 0x01,
	 * so if both are the same, return false/0
	 * if different return 1/true
	 */
	return oldVal ^ lastBinary;
}

