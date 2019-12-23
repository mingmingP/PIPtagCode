/*
 * battery.h
 *
 *  Created on: Apr 23, 2014
 *      Author: romoore
 */

#ifndef BATTERY_H_
#define BATTERY_H_

#include "../CC110x/definitions.h"
#include "../Owl/mem_pool.h"
#include "../settings.h"
#include "msp430.h"

uint16_t getUsedJoules(void);
void doBatterySense(void);

extern uint16_t cached_battery;
/*
 * Author: Michal Potrzebicz
 * URL http://blog.elevendroids.com/2013/06/code-recipe-reading-msp430-power-supply-voltage-level/
 */
uint16_t Msp430_GetSupplyVoltage(void);

#endif /* BATTERY_H_ */
