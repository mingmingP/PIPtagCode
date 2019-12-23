/*******************************************************************************************************
 *  Functions to send and receive packets.                                                             *
 *******************************************************************************************************
 * Revisions:   01/22/2009  B. Firner       Initial version                                            *
 ******************************************************************************************************/
#ifndef _RFSUITE_H
#define _RFSUITE_H

#include "TI_CC_spi.h"
#include "TI_CC_CC1100-CC2500.h"
#include "TI_CC_hardware_board_2553.h"
#include "definitions.h"
//#include "msp430f5529.h"
#include "msp430.h"
//-------------------------------------------------------------------------------------------------------
// Defines
#define CRC_OK              0x80  
#define RSSI                0
#define LQI                 1
#define BYTES_IN_RXFIFO     0x7F        
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
//  void rfSendPacket(BYTE *txBuffer, uint8_t size)
//
//  DESCRIPTION:
//      This function can be used to transmit a packet with packet length up to 63 bytes.
//      To use this function, GD00 must be configured to be asserted when sync word is sent and 
//      de-asserted at the end of the packet => halSpiWriteReg(CCxxx0_IOCFG0, 0x06);
//      The function implements polling of GDO0. First it waits for GD00 to be set and then it waits
//      for it to be cleared.
//  WARNING:
//      This function does not time out. If a pin hangs then this function will never return.
//      
//  ARGUMENTS:
//      BYTE *txBuffer
//          Pointer to a buffer containing the data that are going to be transmitted
//
//      uint8_t size
//          The size of the txBuffer
//-------------------------------------------------------------------------------------------------------
void rfSendPacket(uint8_t *txBuffer, uint8_t size);

//-------------------------------------------------------------------------------------------------------
//  void rfSendPacketNonblock(uint8_t *txBuffer, uint8_t size)
//
//  DESCRIPTION:
//      Similar to rfSendPacket This function sends a packet and returns immediately.
//      
//  ARGUMENTS:
//      uint8_t *txBuffer
//          Pointer to a buffer containing the data that are going to be transmitted
//
//      uint8_t size
//          The size of the txBuffer
//-------------------------------------------------------------------------------------------------------
void rfSendPacket_tag(uint8_t *txBuffer, uint8_t size);

void rfSendPacketNonblock(uint8_t *txBuffer, uint8_t size);

//-------------------------------------------------------------------------------------------------------
//  BOOL halRfReceivePacket(uint8_t *rxBuffer, uint8_t *length)
//
//  DESCRIPTION: 
//      This function can be used to receive a packet of variable packet length (first uint8_t in the packet
//      must be the length uint8_t). The packet length should not exceed the RX FIFO size.
//      To use this function, GD00 must be configured to be asserted when sync word is sent and 
//      de-asserted at the end of the packet => halSpiWriteReg(CCxxx0_IOCFG0, 0x06);
//      Also, APPEND_STATUS in the PKTCTRL1 register must be enabled.
//      The function implements polling of GDO0. First it waits for GD00 to be set and then it waits
//      for it to be cleared.
//      After the GDO0 pin has been de-asserted, the RXuint8_tS register is read to make sure that there
//      are uint8_ts in the FIFO. This is because the GDO signal will indicate sync received even if the
//      FIFO is flushed due to address filtering, CRC filtering, or packet length filtering. 
//  
//  ARGUMENTS:
//      uint8_t *rxBuffer
//          Pointer to the buffer where the incoming data should be stored
//      uint8_t *length
//          Pointer to a variable containing the size of the buffer where the incoming data should be
//          stored. After this function returns, that variable holds the packet length.
//          
//  RETURN VALUE:
//      BOOL
//          TRUE:   CRC OK
//          FALSE:  CRC NOT OK (or no packet was put in the RX FIFO due to filtering)
//-------------------------------------------------------------------------------------------------------

void rfReceive(uint8_t *rxBuffer, uint8_t *length, uint8_t *status);

//-------------------------------------------------------------------------------------------------------
//  ARGUMENTS:
//      uint8_t *rxBuffer
//          Pointer to the buffer where the incoming data should be stored
//      uint8_t *status
//          Pointer to the buffer where the two status uint8_ts (RSSI and LQI) will be stored.
//      uint8_t *length
//          Pointer to a variable containing the size of the buffer where the incoming data should be
//          stored. After this function returns, that variable holds the packet length.
//      unsigned int wait
//          The total time allowed for this function to finish
//      unsigned int rx_wait
//          The total time to wait after the GDO0 pin has been set
//      unsigned int cur_time
//          A pointer to the current time.
//          
//  RETURN VALUE:
//      BOOL
//          TRUE:   A packet was received.
//          FALSE:  No packet was received.
//-------------------------------------------------------------------------------------------------------
bool rfReceiveTimeout(uint8_t *rxBuffer, uint8_t *status, uint8_t *length, unsigned int wait, unsigned int rx_wait, unsigned long* cur_time);

//-------------------------------------------------------------------------------------------------------
//  ARGUMENTS:
//      uint8_t *rxBuffer
//          Pointer to the buffer where the incoming data should be stored
//      uint8_t *status
//          Pointer to the buffer where the two status uint8_ts (RSSI and LQI) will be stored.
//      uint8_t length
//          The length of the expected fixed length packets
//      unsigned int wait
//          The total time allowed for this function to finish
//      unsigned int rx_wait
//          The total time to wait after the GDO0 pin has been set
//      unsigned int cur_time
//          A pointer to the current time.
//          
//  RETURN VALUE:
//      BOOL
//          TRUE:   A packet was received.
//          FALSE:  No packet was received.
//-------------------------------------------------------------------------------------------------------
bool rfReceiveFixedTimeout(uint8_t *rxBuffer, uint8_t *status, uint8_t length, unsigned int wait, unsigned int rx_wait, unsigned long* cur_time);

#endif
