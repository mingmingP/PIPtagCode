#ifndef TPIP_SEND_MAIN_H_
#define TPIP_SEND_MAIN_H_

#include "Owl/mem_pool.h"
//Definitions for spi interface functions
#include "CC110x/tuning.h"
//Definitions for tuning functions--B. Firner
#include "CC110x/rfsuite.h"
//Define functions needed to load buffers and send packets
#include "msp430.h"

// Configurable settings for the tag
#include "settings.h"
// Code for external sensors
#include "sensing/sensing.h"
// Code for optical communcation
#include "optical_conn.h"

#include "interrupt.h"

// Code for saving history of temp/humidity/light
#include "Owl/history.h"

//Settings for the transmit packet buffer

//The size of the base packet is 4
//1 byte for the length and 3 bytes for ID + parity
#define BASE_TX_BUFF_SIZE 			4

typedef struct {
  //7 bits of temperature followed by 1 bit of binary
  uint8_t temp7_binary :1;
  uint8_t temp16_fixed :1;
  uint8_t relativeLight :1;
  uint8_t htuSensing :1;
  uint8_t moisture :1;
  uint8_t vivaristatHistory :1;
  uint8_t battery :1;
  uint8_t decode :1;
} __attribute__((packed)) DataHeader;
// The packed attribute requires GCC extensions
//In Code Composer Studio you must enable GCC extensions by going to:
//Project -> Properties in the menu
//Go to C/C++Build->Settings->Language Options
//Finally, check the box labeled "Enable support for GCC extensions"

uint8_t extraPacketLen(DataHeader* header);
void ReWriteCC1101Registers(void);
void transmitAndPwrDown(char* baseBuffer, DataHeader header);
int senseBinary(int isWater);
void prepBinary(void);
void finishBinary(void);

/******************************************************
 *               LOOK IN SETTINGS.H                   *
 ******************************************************/


#endif /*TPIP_SEND_MAIN_H_*/



