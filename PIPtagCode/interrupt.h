/*
 * interrupt.h
 *
 *  Created on: Apr 23, 2014
 *      Author: romoore
 */

#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#include "msp430.h"
#include "CC110x/definitions.h"
#include "settings.h"
#include "sensing/light.h"

/*
 * Variables that indicate which logic should be used in the interrupt
 * Different sensing modules may reuse the same timer and interrupt
 * module and require different behavior. Those modules control
 * the behavior of the interrupt by setting boolean values indicate
 * which one is running.
 * TODO FIXME The individual variables should be replaced with a
 * single  * numeric enum and the if statements in the interrupt
 * should be replaced with a case statement.
 */

// For moisture sensing code to take control of Timer0_A1 interrupt vector
//	0 allows normal code execution in Timer0_A1 interrupt vector
//	1 runs moisture probe code in Timer0_A1 interrupt vector
extern bool sensingMoisture;

// Set when using the timer to wait for HTU21D initialization
extern bool initializingHTU;

// Set when using the timer for wait for HTU21D reads
extern bool timedSleep;

#endif /* INTERRUPT_H_ */
