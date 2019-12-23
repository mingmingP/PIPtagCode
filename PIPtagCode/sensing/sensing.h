/*
 * sensing.h
 *
 *  Created on: Aug 15, 2013
 *      Author: romoore
 */

#ifndef SENSING_H_
#define SENSING_H_
/*
#include "msp430.h"
#include "../CC110x/TI_CC_spi.h"
#include "../CC110x/definitions.h"
#include "../CC110x/tuning.h"
*/
#include "moisture.h"
#include "battery.h"
#include "light.h"
#include "temperature.h"
#include "htu21d.h"
/*
#include "../settings.h"
#include "../Owl/mem_pool.h"
*/

uint16_t to16Fixed(float f);
int round(double);

/*
 * Store the 7-bit temperature and 1-bit binary data.
 * Only updates the temperature value the first time it is called, and
 * otherwise restores the previous temperature value. To sense a new
 * temperature value use the updateTemp7 function.
 */
bool doTemp7Binary(bool forceUpdate);

int senseBinary();



#endif /* SENSING_H_ */
