/*******************************************************************************************************
 *  Functions to send and receive packets.                                                             *
 *******************************************************************************************************
 * Revisions:   01/22/2009  B. Firner       Initial version                                            *
 ******************************************************************************************************/
#include "rfsuite.h"


#define BYTES_IN_RXFIFO 0x7F

void rfSendPacket(uint8_t *txBuffer, uint8_t size) {

    TI_CC_SPIWriteBurstReg(TI_CCxxx0_TXFIFO, (char*)txBuffer, size);

    TI_CC_SPIStrobe(TI_CCxxx0_STX);
    
    // Wait GDO0 to go hi -> sync TX'ed
    while (!(TI_CC_GDO0_PxIN&TI_CC_GDO0_PIN));

    // Wait GDO0 to clear -> end of pkt
    while (TI_CC_GDO0_PxIN&TI_CC_GDO0_PIN);

}// rfSendPacket

void rfSendPacket_tag(uint8_t *txBuffer, uint8_t size) {

    int i;
    TI_CC_SPIWriteBurstReg(TI_CCxxx0_TXFIFO, (char*)txBuffer, size);

    TI_CC_SPIStrobe(TI_CCxxx0_STX);
    for(i = 0; i < 42; i++);

}// rfSendPacket

void rfSendPacketNonblock(uint8_t *txBuffer, uint8_t size) {
    TI_CC_SPIWriteBurstReg(TI_CCxxx0_TXFIFO, (char*)txBuffer, size);

    TI_CC_SPIStrobe(TI_CCxxx0_STX);

}// rfSendPacketTimeout

void rfReceive(uint8_t *rxBuffer, uint8_t *length, uint8_t *status) {
    uint8_t packetLength;
    
    //w1 is the time for receiving the sync, w2 is the time for receiving the packets after synced
    //750 for 500us (1.5M * 500us = 750) 
    //int w1 = 100, w2 = 40; //8000
    int w1 = 150, w2 = 90; //8000
    TI_CC_SPIStrobe(TI_CCxxx0_SRX);

    // Wait for GDO0 to be set -> sync received
    while (!(TI_CC_GDO0_PxIN&TI_CC_GDO0_PIN) && --w1);
    //for(i = 0; i < 1000; i++);
    // Wait for GDO0 to be cleared -> end of packet
    while (TI_CC_GDO0_PxIN&TI_CC_GDO0_PIN && --w2);

    // This status register is safe to read since it will not be updated after
    // the packet has been received (See the CC11xx and 25xx Errata Note)
    if ((TI_CC_SPIReadStatus(TI_CCxxx0_RXBYTES) & BYTES_IN_RXFIFO)) {

        //packetLength = TI_CC_SPIReadReg(TI_CCxxx0_RXFIFO);
        packetLength = 3;
        // Read data from RX FIFO and store in rxBuffer
        if (packetLength <= *length) {
            TI_CC_SPIReadBurstReg(TI_CCxxx0_RXFIFO, (char*)rxBuffer, packetLength); 
            *length = packetLength;
        
            // Read the 2 appended status uint8_ts (status[0] = RSSI, status[1] = LQI)
            TI_CC_SPIReadBurstReg(TI_CCxxx0_RXFIFO, (char*)status, 2); 
            TI_CC_SPIStrobe(TI_CCxxx0_SIDLE);

            // Flush RX FIFO
            TI_CC_SPIStrobe(TI_CCxxx0_SFRX);
        } else {
            *length = packetLength;

            // Make sure that the radio is in IDLE state before flushing the FIFO
            // (Unless RXOFF_MODE has been changed, the radio should be in IDLE state at this point) 
            TI_CC_SPIStrobe(TI_CCxxx0_SIDLE);

            // Flush RX FIFO
            TI_CC_SPIStrobe(TI_CCxxx0_SFRX);
			status[LQI] = 0;
        }
    }
    else{
        status[LQI] = 0;
        TI_CC_SPIStrobe(TI_CCxxx0_SIDLE);

        // Flush RX FIFO
        TI_CC_SPIStrobe(TI_CCxxx0_SFRX);
    }
}// rfReceivePacket

bool rfReceiveFixedTimeout(uint8_t *rxBuffer, uint8_t *status, uint8_t length, unsigned int wait, unsigned int rx_wait, unsigned long* cur_time) {
    //uint8_t packetLength;
    unsigned long start_time = *cur_time;

    TI_CC_SPIStrobe(TI_CCxxx0_SRX);

    // Wait for GDO0 to be set -> sync received
    while (!(TI_CC_GDO0_PxIN&TI_CC_GDO0_PIN) && *cur_time - start_time <= wait);

    {
	    unsigned long rx_start = *cur_time;
        // Wait for GDO0 to be cleared -> end of packet
        while (TI_CC_GDO0_PxIN&TI_CC_GDO0_PIN && *cur_time - start_time < wait && *cur_time - rx_start < rx_wait);
    }

    if (*cur_time - start_time >= wait) {
        // Force the radio idle
        TI_CC_SPIStrobe(TI_CCxxx0_SIDLE);
        // Flush RX FIFO
        TI_CC_SPIStrobe(TI_CCxxx0_SFRX);
        //powerDownRadio();
        status[LQI] = 0;
        return false;
    }

    // This status register is safe to read since it will not be updated after
    // the packet has been received (See the CC11xx and 25xx Errata Note)
    if ((TI_CC_SPIReadStatus(TI_CCxxx0_RXBYTES) & BYTES_IN_RXFIFO)) {

        // The packet has a fixed length of "length" uint8_ts.
    
        // Read data from RX FIFO and store in rxBuffer
        TI_CC_SPIReadBurstReg(TI_CCxxx0_RXFIFO, (char*)rxBuffer, length); 
    
        // Read the 2 appended status uint8_ts (status[0] = RSSI, status[1] = LQI)
        TI_CC_SPIReadBurstReg(TI_CCxxx0_RXFIFO, (char*)status, 2); 
        // Make sure that the radio is in IDLE state before flushing the FIFO
        // (Unless RXOFF_MODE has been changed, the radio should be in IDLE state at this point) 
        //TI_CC_SPIStrobe(TI_CCxxx0_SIDLE);

        // Flush RX FIFO
        //TI_CC_SPIStrobe(TI_CCxxx0_SFRX);
		status[LQI] = 0;
        
        return true;
    } else {
        status[LQI] = 0;
        return false;
    }
}// rfReceivePacketFixedTimeout
