/*
 * light.h
 *
 *  Created on: Apr 23, 2014
 *      Author: romoore
 */

#ifndef LIGHT_H_
#define LIGHT_H_

// Time-out for ambient light sensing
#define OPTICAL_DELAY_DURATION 255

#include "../CC110x/definitions.h"
#include "msp430.h"
#include "../Owl/mem_pool.h"



uint8_t relativeLightLevel();
void doSenseAmbientLight(bool);


#endif /* LIGHT_H_ */
