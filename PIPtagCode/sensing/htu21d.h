/*
 * htu21d.h
 *
 * Communicate with the HTU21D temperature and relative humidity sensor over IIC.
 *
 *  Created on: Jun 24, 2014
 *      Author: Bernhard Firner
 */

#ifndef HTU21D_H_
#define HTU21D_H_

//For the bool type
#include "../CC110x/definitions.h"


//Remember if the HTU initialized failed and the HTU needs to be initialized
extern bool htu_initialized;


//Initialize the board. Return true upon success or false upon failure
bool initHTU21D();

// Change HTU21D settings
void changeHTU21D(char new_settings);

//Read 12 bit temperature from the HTU21D
//Update the temperature in the on-board module if calibration is true
//Returns 0 on success, an error code on failure
//Stores the sensor reading in internal variable htu_lastTemp;
uint16_t senseHTU21DTemperature(bool calibration);

//Read 8-bit humidity from the HTU21D
//Returns 0 on success, an error code on failure
//Stores the sensor reading in internal variable lastRH;
uint16_t senseHTU21DHumidity();

//Used cached values unless updateCached is true.
//Always senses when called for the first time.
void doSenseHTURH16F(bool updateCached);
//Set calibration to true if this should update the temperature in the
//on-board temperature code module for recalibration after
//temperature changes.
void doSenseHTUTemp16F(bool updateCached, bool calibration);

#endif /* HTU21D_H_ */
