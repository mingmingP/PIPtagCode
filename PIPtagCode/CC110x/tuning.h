/*******************************************************************************************************
 *  Functions for controlling the frequency of the CC1100 radio.                                       *
 *******************************************************************************************************
 * Revisions:   01/22/2009  B. Firner       Initial version                                            *
 ******************************************************************************************************/
#ifndef _TUNING_H
#define _TUNING_H

//Definitions for CC1101 interface
#include "TI_CC_spi.h"
#include "TI_CC_CC1100-CC2500.h"
#include "definitions.h"
//Device specific definitions
#include "TI_CC_msp430.h"
//Specific definitions for 5529 and prototype board
#include "TI_CC_hardware_board_2553.h"
//Definitions for interface between CC1101 and MSP430
#include "CC1100-CC2500.h"
// Configurable settings for the tag
#include "../settings.h"
// The amount we've raised or lowered the frequency during tuning.
extern int lowchange;
extern int highchange;
#define TUNE_STEP 0x18
extern unsigned int step; // How much we change the frequency by each step.

extern unsigned long cur_freq;

//Return the current frequency value.
unsigned long curFreq();

//Return the actual frequency in Hz
unsigned long actualFreq();

//Set the transmission frequency
bool setFreq(unsigned long desired);

void recal(unsigned long change);

void Wake_up_CC1101();

void ReWriteCC1101Registers(void);

void recalibrateCC11xx(unsigned long desired);

void restoreCC11xxRegs();

void storeCC11xxRegs();

void setupAndPowerDownCC11xx();

void setPowerSettings();

#endif
