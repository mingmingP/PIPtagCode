/*******************************************************************************************************
 *  Functions for controlling the frequency of the CC1100 radio.                                       *
 *******************************************************************************************************
 * Revisions:   01/22/2009  B. Firner       Initial version                                            *
 ******************************************************************************************************/
#include "tuning.h"

//Variables that were declared as extern in tuning.h
int lowchange;
int highchange;
unsigned int step; // How much we change the frequency by each step.
unsigned long cur_freq = 0;

/* Copying the following register values from CC1150 */
uint8_t CC1150RegTEST0 = 0;
uint8_t CC1150RegTEST1 = 0;
uint8_t CC1150RegTEST2 = 0;

uint8_t CC1150RegFSCAL0 = 0;
uint8_t CC1150RegFSCAL1 = 0;
uint8_t CC1150RegFSCAL2 = 0;
uint8_t CC1150RegFSCAL3 = 0;

sint8_t paTable[PA_TBL_SIZE];

//Return the current frequency value.
unsigned long curFreq() {
	return cur_freq;
}

//Return the actual frequency in Hz
unsigned long actualFreq() {
	return (cur_freq) * 396.7;
}

//Set the transmission frequency
bool setFreq(unsigned long desired) {
	if (desired > 950000000 || desired < 340000000) {
		return false;
	}

	//Set the frequency based upon the equation:
	//F_carrier = (f_xoscillator / 2^16) * (FREQ[23:0] + CHAN*((256 + CHANSP_M)*2^(CHANSP_E-2)))
	//ONLY good for CHAN=0, CHANSPC_M, CHANSPC_E don't matter

	cur_freq = (desired / 396.7);
	recal(0);
	return true;
}

// Recalibrate the radio with the given change
// from the base values.
// The magnitude of the change is
// frequency_oscillator/(2^16) ~= 396.7 Hz
// There is a base frequency of 86,020 * 396.7
void recal(unsigned long change) {
	cur_freq += change;

	/* Force the radio idle and flush the buffer before we retune the board. */
	TI_CC_SPIStrobe(TI_CCxxx0_SIDLE);
	TI_CC_SPIStrobe(TI_CCxxx0_SFRX);

	/* Write the new register settings.*/
	TI_CC_SPIWriteReg(TI_CCxxx0_FREQ0, cur_freq % 0x100);//lowest byte of FREQ[]
	TI_CC_SPIWriteReg(TI_CCxxx0_FREQ1, (cur_freq / 0x100) % 0x100);	//middle byte of FREQ[]
	TI_CC_SPIWriteReg(TI_CCxxx0_FREQ2, (cur_freq / 0x10000) % 0x40);//upper byte of FREQ[]
																	//Top two bits are always zero because fxosc<32 MHz (usually 26 MHz)

	/* Now calibrate the board.*/
	TI_CC_SPIStrobe(TI_CCxxx0_SCAL);
}

void Wake_up_CC1101() {
	TI_CC_CSn_PxOUT &= ~TI_CC_CSn_PIN;     // CSn enable

	TI_CC_Wait(40); /* 12 usec for avoid edge races between CSn and S0 */

	TI_CC_SPI_USCIB0_PxSEL &= ~TI_CC_SPI_USCIB0_SOMI;
	TI_CC_SPI_USCIB0_PxSEL2 &= ~TI_CC_SPI_USCIB0_SOMI;
	/* It takes about 221 usec for S0 to go low */
	while (TI_CC_SPI_USCIB0_PxIN & TI_CC_SPI_USCIB0_SOMI) {
	} //Check for SO to go low Wait until XOSC stable
	TI_CC_SPI_USCIB0_PxSEL |= TI_CC_SPI_USCIB0_SOMI;
	TI_CC_SPI_USCIB0_PxSEL2 |= TI_CC_SPI_USCIB0_SOMI;

	TI_CC_CSn_PxOUT |= TI_CC_CSn_PIN;     // CSn enable

	return;
}

void ReWriteCC1101Registers(void) {
	/* write the default RF settings from CC1100-CC2500.c */
	// Write register settings
	TI_CC_SPIWriteReg(TI_CCxxx0_FSCTRL1, 0x0B); // Freq synthesizer control.
	TI_CC_SPIWriteReg(TI_CCxxx0_FSCTRL0, 0x00); // Freq synthesizer control.
	TI_CC_SPIWriteReg(TI_CCxxx0_MDMCFG4, 0x2D); // Modem configuration.  DRATE_E = 45.  Calculate symbol rate.
	TI_CC_SPIWriteReg(TI_CCxxx0_MDMCFG3, 0x3B); // Modem configuration.  DRATE_M = 59.  Calculate symbol rate.
	TI_CC_SPIWriteReg(TI_CCxxx0_MDMCFG2, 0x73); // Modem configuration.  30/32 Sync word bits detected. No Manchester. MSK.
	TI_CC_SPIWriteReg(TI_CCxxx0_MDMCFG1, 0x22); // Modem configuration.  CHANSPC_E=4.  Number of preamble bytes=4.  FEC Disabled
	TI_CC_SPIWriteReg(TI_CCxxx0_MDMCFG0, 0xF8); // Modem configuration.  CHANSPC_M=248.
	TI_CC_SPIWriteReg(TI_CCxxx0_CHANNR, 0x00); // Channel number.
	TI_CC_SPIWriteReg(TI_CCxxx0_DEVIATN, 0x00); // For MSK, fraction of period for phase change.
	TI_CC_SPIWriteReg(TI_CCxxx0_FREND1, 0xB6); // Front end RX configuration.
	TI_CC_SPIWriteReg(TI_CCxxx0_FREND0, 0x10); // Front end TX configuration. Set current in TX LO buffer.  Use PATABLE index zero.
	TI_CC_SPIWriteReg(TI_CCxxx0_MCSM1, 0x00); // Go into IDLE mode after TX.
	TI_CC_SPIWriteReg(TI_CCxxx0_MCSM0, 0x08); // Manual calibration. Shortest timeout for XOSC stabilized voltage before CHP_RDY_N goes low.
	TI_CC_SPIWriteReg(TI_CCxxx0_FOCCFG, 0x1D); // Freq Offset Compens. Config
	TI_CC_SPIWriteReg(TI_CCxxx0_BSCFG, 0x1C); //  Bit synchronization config.
	TI_CC_SPIWriteReg(TI_CCxxx0_AGCCTRL2, 0xC7); // AGC control.
	TI_CC_SPIWriteReg(TI_CCxxx0_AGCCTRL1, 0x00); // AGC control.
	TI_CC_SPIWriteReg(TI_CCxxx0_AGCCTRL0, 0xB2); // AGC control.
	TI_CC_SPIWriteReg(TI_CCxxx0_FSTEST, 0x59); // Frequency synthesizer cal.  Needs to be set.  Undocumented--see SWRA359A

	//TI_CC_SPIWriteReg(TI_CCxxx0_IOCFG2,   0x0B); // GDO2 output pin config. Serial clock
	//TI_CC_SPIWriteReg(TI_CCxxx0_IOCFG0,   0x06); // GDO0 output pin config. Assert when sync sent/received
	/* Using ATEST to check PLL Status */
	//TI_CC_SPIWriteReg(TI_CCxxx0_IOCFG0,   0x0A); // GDO0 output pin config. Lock detector output
	TI_CC_SPIWriteReg(TI_CCxxx0_IOCFG0, 0x2E); // GDO0 output pin to high impedance 3-state

	//TI_CC_SPIWriteReg(TI_CCxxx0_IOCFG0,   0x29); // GDO0 output pin to CHIP_RDYn

	TI_CC_SPIWriteReg(TI_CCxxx0_PKTCTRL1, 0x04); // Packet automation control.
#if PROTOCOL_VERSION == 2
    TI_CC_SPIWriteReg(TI_CCxxx0_PKTCTRL0, TI_CCxxx0_PKT_LEN_VAR|TI_CCxxx0_PKT_CRC_EN|TI_CCxxx0_PKT_DAT_WHT); // Data whitening, CRC enabled, variable-length packets
#else // version 1
    TI_CC_SPIWriteReg(TI_CCxxx0_PKTCTRL0, TI_CCxxx0_PKT_LEN_VAR|TI_CCxxx0_PKT_DAT_WHT); // Data whitening,variable-length packets
#endif
	TI_CC_SPIWriteReg(TI_CCxxx0_ADDR, 0x00); // Device address.
	TI_CC_SPIWriteReg(TI_CCxxx0_PKTLEN, 0x03); // Packet length.	Not relevant if PKTCTRL0 set for variable length (01)

	return;
}

void storeCC11xxRegs() {

	CC1150RegTEST0 = TI_CC_SPIReadReg(TI_CCxxx0_TEST0);
	CC1150RegTEST1 = TI_CC_SPIReadReg(TI_CCxxx0_TEST1);
	CC1150RegTEST2 = TI_CC_SPIReadReg(TI_CCxxx0_TEST2);
	CC1150RegFSCAL0 = TI_CC_SPIReadReg(TI_CCxxx0_FSCAL0);
	CC1150RegFSCAL1 = TI_CC_SPIReadReg(TI_CCxxx0_FSCAL1);
	CC1150RegFSCAL2 = TI_CC_SPIReadReg(TI_CCxxx0_FSCAL2);
	CC1150RegFSCAL3 = TI_CC_SPIReadReg(TI_CCxxx0_FSCAL3);
}

void restoreCC11xxRegs() {
	TI_CC_SPIWriteReg(TI_CCxxx0_TEST0, CC1150RegTEST0);
	TI_CC_SPIWriteReg(TI_CCxxx0_TEST1, CC1150RegTEST1);
	TI_CC_SPIWriteReg(TI_CCxxx0_TEST2, CC1150RegTEST2);

	TI_CC_SPIWriteReg(TI_CCxxx0_FSCAL0, CC1150RegFSCAL0);
	TI_CC_SPIWriteReg(TI_CCxxx0_FSCAL1, CC1150RegFSCAL1);
	TI_CC_SPIWriteReg(TI_CCxxx0_FSCAL2, CC1150RegFSCAL2);
	TI_CC_SPIWriteReg(TI_CCxxx0_FSCAL3, CC1150RegFSCAL3);

	TI_CC_SPIWriteReg(TI_CCxxx0_FREQ0, cur_freq % 0x100); //lowest byte of FREQ[]
	TI_CC_SPIWriteReg(TI_CCxxx0_FREQ1, (cur_freq / 0x100) % 0x100); //middle byte of FREQ[]
	TI_CC_SPIWriteReg(TI_CCxxx0_FREQ2, (cur_freq / 0x10000) % 0x40); //upper byte of FREQ[]
}

void recalibrateCC11xx(unsigned long desired) {
	/* 'SetFreq' sends manual calibration strobe from inside */
	setFreq(desired);

	/* Wait till CC1150 State goes back to Idle */
	while (0x01 != TI_CC_SPIReadReg(0xF5)) {
		TI_CC_Wait(500);
	}

	/* Store the register values of CC1150 */
	storeCC11xxRegs();
}

void setupAndPowerDownCC11xx() {
	paTable[0] = DEFAULT_PATABLE;

	/* Setup up communications with CC1101 */
	TI_CC_SPISetup();

	/* Reset CCxxxx */

	TI_CC_PowerupResetCCxxxx();

	/* write the default RF settings from CC1100-CC2500.c */
	writeRFSettings();
	/* set the power amplifier settings */
	TI_CC_SPIWriteBurstReg(TI_CCxxx0_PATABLE, paTable, PA_TBL_SIZE);

	/* 'SetFreq' sends manual calibration strobe from inside */
	setFreq(DEFAULT_FREQ);

	/* Wait till CC1150 State goes back to Idle */
	while (0x01 != TI_CC_SPIReadReg(0xF5)) {
		TI_CC_Wait(500);
	}

	/* Store the register values of CC1150 */
	storeCC11xxRegs();

	TI_CC_SPIStrobe(TI_CCxxx0_SPWD);
}

void setPowerSettings() {
	/* set the power amplifier settings */
	TI_CC_SPIWriteBurstReg(TI_CCxxx0_PATABLE, paTable, PA_TBL_SIZE);
}
