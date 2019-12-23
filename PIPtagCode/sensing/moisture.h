/*
 * moisture.h
 *
 *  Created on: Apr 23, 2014
 *      Author: romoore
 */

#ifndef MOISTURE_H_
#define MOISTURE_H_

#include "../CC110x/definitions.h"
#include "../CC110x/tuning.h"
#include "msp430.h"
#include "../settings.h"
#include "battery.h"

extern uint16_t cached_battery;

/*
 * Radio Crystal as Timer0_A clock source (TA0CLK)
 * Author: Jakub Kolodziejski
 *
 * Allows the use of the CC1150 radio's crystal oscillator as the clock source
 * TA0CLK on P1.0 (GDO0) for Timer0_A. Crystal oscillator frequency is 26MHz but
 * output to TA0CLK can be divided to desired frequency.
 */
void crystalOn(uint8_t divider);	// Wakes up radio and enables output of desired frequency to P1.0 (TA0CLK)
void crystalOff();					// Stops crystal oscillator output from radio and puts radio to sleep

/*
 * Moisture Sensing Probe
 * Author: Jakub Kolodziejski
 */
// Pins used for moisture sensing probe
#define CHARGE_PIN  BIT1	// P1.1, Tag AUX pin 8
#define SENSE_PIN	BIT2	// P1.2, Tag AUX pin 9
// Function definitions
void doMoistureSense(bool);
void cm_initMoistureSense(void);
uint16_t cm_getMoistureLevel(void);

#endif /* MOISTURE_H_ */
