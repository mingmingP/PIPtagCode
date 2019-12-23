/*
 * optical_conn.h
 *
 *  Created on: Aug 29, 2013
 *      Author: romoore
 */

#ifndef OPTICAL_CONN_H_
#define OPTICAL_CONN_H_

#include "CC110x/tuning.h"
#include "sensing/sensing.h"

//Key values for the key-value transmission
#define KEY_ID		0x01 //3 Byte; ID (21 bits)
#define KEY_FREQ	0x02 //1 Byte; Number of MHz frequency offset (900 + byte value)
#define KEY_INTVL	0x03 //2 Bytes; Packet interval, in multiples of 600 (corresponds to 1/20 of a sec)
#define KEY_HDR		0x04 //1 Byte; Data header setting
//Set to true to write these values
#define WRITE_ID	1
#define WRITE_FREQ	1
#define WRITE_INTVL	1
#define WRITE_HDR	1

#define IDCODE1 0x00
#define IDCODE2 0x55
#define IDCODE3 0xf5
#define FREQCODE 22
#define INTVL1 0x02 //Upper byte
#define INTVL2 0x58 //Lower byte
#define HDRCODE 0x04

// Optical transmission constants
#define MAX_OPTICAL_BYTES  14 //3 framing bytes, 4 keys, 7 total data bytes
#define OPTICAL_UNLOCK1 0x11
#define OPTICAL_UNLOCK2 0x22
#define OPTICAL_END 0xAA

#define TIME_WAIT 4350						//5 seconds, ~870 iterations per second, with 6Mhz clock

// Programmable parameters for the tag
typedef struct {
	unsigned long boardID;
	double freq;
	long packet_interval;
	uint8_t header;
}__attribute__((packed)) TagParameters;

void flashLights(int);
void lightsetup(void);
int lightSense();
void opticalTransmit(void);

/*
 * Perform optical reception to fill in the TagParameters struct.
 * Any fields not sent will be left unchanged.
 */
void opticalReceive(volatile TagParameters*);

#endif /* OPTICAL_CONN_H_ */
