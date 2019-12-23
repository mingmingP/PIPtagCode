/*
 * temperature.h
 *
 *  Created on: Apr 23, 2014
 *      Author: romoore
 */

#ifndef TEMPERATURE_H_
#define TEMPERATURE_H_

#include "../CC110x/definitions.h"
#include "../settings.h"
#include "msp430.h"
#include "../Owl/mem_pool.h"
#include "sensing.h"

#define DEFAULT_SLOPE	0xACCADACA		//0xACCADACA default for automatic flashing
#define DEFAULT_OFFSET	0xADDADADA		//0xADDADADA default for automatic flashing
/*
 * Initialize the temperature values.
 */
void initTemperature();
/*
 * Configure the ADC for temperature readings.
 */
void configADC();
/*
 * Get the current temperature value.
 */
float getTemperature();
void updateTemp16F(bool);
void doSenseTemp16F();
int recalRadioFromTemp();
/*
 * Update the 7-bit temperature used in the doTemp7Binary function.
 * Call doTemp7Binary before calling updateTemp7; updateTemp7 updates
 * the memory where the doTemp7Binary function stored the temperature value.
 */
void updateTemp7();

/*
 * Used to set the temperature value if temperature is being sensed from
 * outside of this module.
 */
void setLastTemp(float setVal);

#endif /* TEMPERATURE_H_ */
