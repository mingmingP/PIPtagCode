#include "main.h"

/* Globals */

//Set bootstrap loader to all 0 (bsl enabled, do not erase on password error)
#pragma DATA_SECTION(bslcfg, ".bslcfg")
volatile const uint16_t bslcfg = 0;

#pragma DATA_SECTION(password1, ".int00")
volatile const uint16_t password1 = 0xB30F;
#pragma DATA_SECTION(password2, ".int01")
volatile const uint16_t password2 = 0xB612;
#pragma DATA_SECTION(password3, ".int04")
volatile const uint16_t password3 = 0xFC1A;

unsigned long boardID = TXER_ID; //Defined in settings.h so can be used as variable in bulk flash programming
volatile double freq = DEFAULT_FREQ; //Transmit RF frequency.  Defined in settings.h
volatile TagParameters params; //Data received via the optical communication link

//Buffer for the non-data portion of the packet
uint8_t baseBuffer[BASE_TX_BUFF_SIZE];

volatile float vloMsMult = 12; 	// VLO is imprecise.  Estimate an initial value (12kHz) and then
								// Recalibrate with using the crystal on the radio chip.

extern void SleepLPM3(long ACLKDly); //declare extern asm function to implement ACLK delay
									// This is the function that puts the tag to sleep between
									// activities like sensing or transmitting.
extern void AlrmClkStrt(); 			//declare extern asm function to start the alarm clock timer

//Set the state of the pins to minimize energy use and provide correct functionality
//In particular, unused input pins have to be pulled up or down to prevent crowbar current.
void setMSP430Pins() {
	//Port 1
	P1SEL2 = 0x00;			//  All pins set to I/O function.
	P1SEL = 0x00;//  Set P1 to I/O, not special function; Will be changed to SPI later in code
	P1DIR = 0xA0;		//	All pins to input except P1.5 (SCLK) and P1.7 (SI)
	P1REN = 0x5F;//  Enable all pullup/down resistors except P1.5 (SCLK) and P1.7 (SI)
	P1OUT = 0x01;//  All input resistors set to pulldown, except P1.0 (GDO0) set to pullup.  Outputs P1.5 (SCLK) and P1.7 (SI) set low.

	//Port 2
	P2SEL2 = 0x00;			//  All pins set to I/O function.
	P2SEL = 0x00;			//  Set P2 to I/O, not special function
	P2DIR = 0x00;           // 	All pins set to input.
	P2REN = 0xFF;			//  Enable all pullup/pulldown resistors.
	P2OUT = 0x40;//  All input resistors set to pulldown, except P2.6 (GDO0) set to pullup.

	//Port 3
	P3SEL2 = 0x00;			//  All pins set to I/O function.
	P3SEL = 0x00;			//  Set P3 to I/O, not special function
	P3DIR = 0x40;           //	All pins to input except P3.6 (CSn)
	P3REN = 0xBF;	//  Enable all pullup/pulldown resistors except P3.6 (CSn).
	P3OUT = 0x40;//  All input resistors set to pulldown.  Output P3.6 (CSn) set high.

	return;
}
//Configure clocks and clock sources
void set_UCS() {
	/* DCO Frequency selected */
	/* SCLK=SMCLK  P1.5  */
	/*	Maximum SPI frequency for burst mode is 6.5 MHz	*/
	//Set DCO to 12 MHz calibrated and use DCO/2 for SMCLK
	//Note that this is technically a bit too high for operation at 2.2Volts (end of battery life).  8 MHz is maximum recommended.
	//Does not seem to be a problem at room temperature.
	DCOCTL = CALDCO_12MHZ;		//0x7A  01111010
								//DCOx  011
								//MODx	11010

	/* External Crystal OFF, LFEXT1 is low frequency, ACLK is Divided by 1, RSELx=1110 */
	// Set range  0x8E  10001110
	BCSCTL1 = CALBC1_12MHZ;

	// DCO -> SMCLK (Default), SMCLK / 2; DCO -> MCLK, MCLK / 2; DCOR Internal Resistor select
	BCSCTL2 = DIVS_1 + DIVM_1 + SELM_0; // 0x52 = 00010010

	/* 0.4 to 1MHz crystal , Using VLO for ACLK, Cap selected 1pF */
	BCSCTL3 = LFXT1S_2;

	IE1 &= 0xFD; /* Disable UCS interrupt */

	return;
}
/*
int getBit(int decimal, int index) {
	int constant = 1 << (index);

	if (decimal & constant) {
		return 1;
	}

	return 0;
}
*/

void transmitAndPwrDown(char* baseBuffer, DataHeader header) {
	/*Load transmit FIFO registers with the packet length */
	TI_CC_SPIWriteBurstReg(TI_CCxxx0_TXFIFO, baseBuffer, 1);
	/*Transition from idle to transmit mode.*/
	TI_CC_SPIStrobe(TI_CCxxx0_STX);
	//Now transfer the rest of the packet (ID+parity and the temperature+binary byte)
	//Saves a bit of energy since transfer over SPI bus is much faster than transmission over the air.
	TI_CC_SPIWriteBurstReg(TI_CCxxx0_TXFIFO, baseBuffer + 1,
	BASE_TX_BUFF_SIZE - 1);
	//Use mempool to find out how much data there is to send
	if (0 < getAllSize()) {
		TI_CC_SPIWriteBurstReg(TI_CCxxx0_TXFIFO, (char*) (&header), 1);
		TI_CC_SPIWriteBurstReg(TI_CCxxx0_TXFIFO, (char*) getBase(),
				getAllSize());
	}

	/*Power down CC1101 when Csn goes high*/
	TI_CC_SPIStrobe(TI_CCxxx0_SPWD);

}

/*
 * TimerA0 -> source GD0 from radio, count known time unit
 * Using 26 MHz crystal divided by 192.
 * TimerA1 -> source from VLO, capture value when TimerA0 expires
 * VLO count is used to adjust the
 */
void recalibrateVLO(bool controlRadio) {

	// Configure P1.0 (GDO0) to use radio's crystal oscillator for TA0CLK
	P1IE &= ~BIT0;		// Disable interrupt
	P1DIR &= ~BIT0;		// Set as input
	P1REN &= ~BIT0;		// Disable pullup/pulldown resistor
	P1SEL |= BIT0;		// Set to TA0CLK (TimerA_0 clock source)
	P1SEL2 &= ~BIT0;	// ^ see above ^
	P1OUT &= ~BIT0;		// Set low
	CAPD &= ~BIT0;		// Enable GPIO buffer

	// Configure P2.6 so it doesn't interfere with clock signal to P1.0
	P2IE &= ~BIT6;		// Disable interrupt
	P2DIR &= ~BIT6;		// Set as input
	P2REN &= ~BIT6;		// Disable pullup/pulldown resistor
	P2SEL &= ~BIT6;		// Set to GPIO
	P2SEL2 &= ~BIT6;	// ^ see above ^
	P2OUT &= ~BIT6;		// Set low
	if (controlRadio) {
		Wake_up_CC1101();
	}
	/* 135,416 Hz */
	TI_CC_SPIWriteReg(TI_CCxxx0_IOCFG0, CRYSTAL_DIV_192);

	TA0CTL = TACLR;
	TA0CTL = TASSEL_0 + ID_0 + MC_0;// Use TA0CLK (crystal), no division, stopped mode
	/* 50ms = 6,771 output ticks, count from 0-6770 */
//	TA0CCR0 = 6770;
	TA0CCTL0 = CM_1 + CCIS_2 + SCS + CAP; // capture on rising edge, trigger is low, capture mode

	TA1CTL = TACLR;
	TA1CTL = TASSEL_1 + ID_0 + MC_0; // Use ACLK, no division, stopped mode
	TA1CCR0 = VLO_CALIB_COUNTS; // 4kHz = 4ms, 20kHz = 750us
	TA1CCTL0 = 0;
	TA1CCTL0 = CCIE;

	// Sleep

	TA0CTL |= MC_2; // Start crystal clock counting (135kHz)
	TA1CTL |= MC_1; // Start VLO clock counting (4kHz - 20kHz)

	// Sleep until wake-up.
	__bis_SR_register(LPM3_bits + GIE);
	TA0CTL = 0; // Stop crystal clock timer
	TA1CTL = 0; // Stop VLO clock timer

	// Reset GDO0 to high impedence tri-state as set by ReWriteCC1101Registers() in main.c
	TI_CC_SPIWriteReg(TI_CCxxx0_IOCFG0, 0x2E); // GDO0 output pin to high impedance 3-state

	uint16_t crystalCount = TA0CCR0;
	vloMsMult = (135.416 * VLO_CALIB_COUNTS) / crystalCount;
}

//	Number of times each operation is performed
//	Used for calculating total Joules consumed since startup.
uint32_t numBinary = 0;
uint32_t numTemp = 0;
uint32_t numHTU = 0;
uint32_t numBattery = 0;
uint32_t numRadio = 0;
uint32_t numLight = 0;
uint32_t numWakeup = 0;
uint32_t numHistory = 0;

//Flag indicating whether temp has been updated
bool updatedTemp = false;

//Sleep 10 ms to allow the capacitor to recharge
void sleep10ms() {
	//Go to LPM3 for 10ms
	//Reset the state of the timer
	TA0CTL |= TACLR;

	//Count up to 10ms using the VLO.
	TA0CCR0 = 10*vloMsMult;
	TA0CCR1 = 0;

	//Source from ACLK (VLO) in UP mode with no divider
	TA0CTL = TASSEL_1 | MC_1 | ID_0 | TAIE;
	// Sleep until wake-up.
	timedSleep = true;
	__bis_SR_register(LPM3_bits + GIE);
	timedSleep = false;
	//Shut down the timer
	TA0CTL &= ~MC_1;
	TACCR0 = 0;
}

void main(void) {
	/* Stop the Watchdog Timer (WDT) */
	WDTCTL = WDTPW + WDTHOLD;

	//The header to be used.
	DataHeader header;
	//Clear the header
	*(uint8_t*) (&header) = 0;
	//Going to send temperature+binary data.
	header.temp7_binary = ENABLE_BINARY; //7 bits:  1 degree C resolution, -40 to +87 C
	header.temp16_fixed = ENABLE_TEMP16; //16 bit fixed point.  4 bits fraction (1/16 degree C resolution).  12 bits integers.
	//Send relative light level
	header.relativeLight = ENABLE_AMBIENT_LIGHT;//0 turns off light sensing.  1  Turns on light sensing.

	// Send moisture level
	header.moisture = ENABLE_MOISTURE;// 0 turns off moisture sensing. 1 turns on moisture sensing.
	header.htuSensing = ENABLE_HTU_SENSING;

	//The total number of sense types that we'll be using
	//We need to allow enough time for each sensor to
	//finish and then have a 10ms delay before using the
	//radio or going to the next sensor so that the
	//capacitor can recharge.
	unsigned char enabled_sense_types = ENABLE_BINARY + ENABLE_TEMP16 +
			ENABLE_AMBIENT_LIGHT + ENABLE_MOISTURE + ENABLE_HTU_SENSING;

	/***  Battery Level ***
	 * Checking is done NO MATTER WHAT every BATTERY_STEPS
	 * as defined in settings.h
	 * Header bit is ONLY set when it will transmit.
	 */

	params.boardID = boardID;
	params.freq = freq;
	params.packet_interval = PACKTINTVL_MS;
	params.header = *(uint8_t*) (&header);

	//Counter that determines when to recalibrate the radio
	int recal_counter = 0;  //RECALINTVL / BINARY_INTVL;

	// Number of times to wake-up before sending a BACKGROUND packet
	int tx_counter = 1;

	//  Number of times to wake up before sensing the binary door/water status
	int binary_counter = BINARY_STEPS;

	// Number of times to wake up before sensing the temperature
	int temp_counter = 1;

	// Attempt to ensure reception of CRITICAL events.
	int repeat_counter = 0;
	// Number of remaining "repeats" to send
	int repeat_tx_remain = 0;

	// Number of times to wake up before checking battery
	int battery_counter = 1;

	// Number of times to wake up before checking battery
	int light_counter = 1;

	// Number of times to wake up before checking moisture
	int moisture_counter = 1;

	// Number of times to wake up before sending history, start with a history broadcast
	int history_counter = 1;

	// Prepare the temperature-related values
	initTemperature();

	// Number of times to wake up before doing temperature/relative humidity sensing
	int htu_counter = 1;

	/*
	 * For optical transmission of transmitter settings
	 */
	//opticalTransmit();
	//while(1){}
	/*
	 * End optical transmission segment.
	 */

	//Flash LED and look for optical reprogramming signal
	//If no optical signal is found then this uses default values.
	//In production code where maximum lifetime is needed, disable LED flashing.
	//When battery gets low, internal resistance is high enough that flashing shuts down the tag prematurely
	if (!PRODUCTION) {
			flashLights(2);
	}
	//opticalReceive(&params);
	//Re-assign the header
	header = *(DataHeader*) (&params.header);

	/* Configuring the ports: Set unused pins to input */
	setMSP430Pins();
	/* Setting up the system clocks: Setup ACLK, SMCLK, and MCLK */
	set_UCS();

	AlrmClkStrt();                        //Start the alarm clock
	SleepLPM3(3277);                    //sleep for 100ms to let the caps charge

#if PROTOCOL_VERSION == 2
	// Fill Tx buffer: Load packet information
	// The ID is 24 bits making the Length 3 bytes.
	//Hardware generated CRC of 2 bytes is also added, but not counted in the Length.
	// When the packet is going to be sent, use the getAllSize function
	// from the mem_pool header to see if any sensed data is to be sent.
	// If it was, modify the value of the Length.
	// The new Length comprises the ID plus all data to be sent, including the header.
	baseBuffer[0] = 0x03;
	/* MAC ID (24 bits) */
	baseBuffer[1] = 0xFF & (params.boardID >> 16);
	baseBuffer[2] = 0xFF & (params.boardID >> 8);
	baseBuffer[3] = 0xFF & (params.boardID);
#else // Version 1	// Fill Tx buffer: Load packet information	// The ID is 21 bits with 3 bits of parity for a total of 3 bytes	// 3 bytes for the ID plus any extra required for header and data	// When the packet is going to be sent, use the getAllSize function	// from the mem_pool header to see if any sensed data was taken.	// If it was, modify the value of the length.	baseBuffer[0] = 0x03;	/* High order bites of packet ID.  Bits 21-13 */	baseBuffer[1] = 0xFF & (params.boardID >> 13);	baseBuffer[2] = 0xFF & (params.boardID >> 5);	baseBuffer[3] = 0xFF & (params.boardID << 3);	//Now count parity	{	int shift;
	for (shift = 0; shift < 21; shift += 3) {
		baseBuffer[3] ^= 0x7 & (params.boardID >> shift);
	}
}
#endif

	setupAndPowerDownCC11xx();

	//Initialize the HTU21D if we are going to use it.
	//The chip will make this attempt five times at startup.
	//The code will retry initialization before each attempted read
	//until the chip is successfully initialized after that point.
	if (header.htuSensing) {
		uint8_t init_tries = 0;
		while (init_tries < 5 && !initHTU21D()) {
			++init_tries;
		}
	}

	initHistory();

	for (;;) {

		// Decrement the various counters
		--binary_counter;
		--tx_counter;
		--temp_counter;
		--recal_counter;
		--battery_counter;
		--light_counter;
		--moisture_counter;
		--htu_counter;
		--history_counter;
		++numWakeup;

		if (repeat_counter > 0) {
			repeat_counter--;
		}

		/*
		 * Outline of sensing and transmission:
		 *
		 * 1. If performing a "repeat", mark doTransmit = 1
		 * 2. If time to BACKGROUND transmit, mark doTransmit = 1
		 * 3. Perform any sensing necessary this round
		 * 4. If any CRITICAL data has changed, mark doTransmit = 1
		 * 5. If doTransmit==1, transmit
		 * 6. Sleep in LPM3 for WAKE_INTVL
		 */

		/*
		 * Check for a "repeat" transmission or BACKGROUND transmission.
		 */
		bool doTransmit = (repeat_tx_remain > 0 && repeat_counter == 0)
				|| (tx_counter <= 0);

		/*
		 * Check any sensing values appropriate for this round.
		 */
		if (!(header.decode)) {
			/**
			 * Update actual temperature reading if the counter has expired
			 */
			if (temp_counter <= 0) {
				if (header.temp7_binary) {
					updateTemp7();
					updatedTemp = true;
				}
				if (header.temp16_fixed) {
					updateTemp16F(!updatedTemp);
					updatedTemp = true;
				}
				temp_counter = TEMP_STEPS;
				//Sleep 10 ms to allow the capacitor to recharge
				sleep10ms();
			}
			if (header.temp7_binary) {
				if (doTemp7Binary(!binary_counter)) {
					repeat_tx_remain = SENSE_TX_REPEAT;
					doTransmit = true;
				}
				if (binary_counter == 0) {
					binary_counter = BINARY_STEPS;
				}
				//Sleep 10 ms to allow the capacitor to recharge
				sleep10ms();
			}
			if (header.temp16_fixed) {
				doSenseTemp16F();
				//Sleep 10 ms to allow the capacitor to recharge
				sleep10ms();
			}
			if (header.relativeLight) {
				if (light_counter) {
					doSenseAmbientLight(false);
				} else {
					doSenseAmbientLight(true);
					light_counter = AMBIENT_LIGHT_STEPS;
				}
				//Sleep 10 ms to allow the capacitor to recharge
				sleep10ms();
			}
			//HTU21D temperature and relative humidity sensor
			if (header.htuSensing) {
				//Initialize if successful communication did not occur
				if (!htu_initialized) {
					initHTU21D();
				}
				//Call the sensing functions even if initialization was not
				//successfull to fill in the data fields with error codes
				if (htu_counter) {
					doSenseHTUTemp16F(false,
							HTU_ONBOARD
									&& !(header.temp7_binary
											|| header.temp16_fixed));
					doSenseHTURH16F(false);
				} else {
					//Counter reached zero so do sensing and reset the counter
					doSenseHTUTemp16F(true,
							HTU_ONBOARD
									&& !(header.temp7_binary
											|| header.temp16_fixed));
					doSenseHTURH16F(true);
					updatedTemp = HTU_ONBOARD && true;
					htu_counter = HTU_STEPS;
				}
				//Sleep 10 ms to allow the capacitor to recharge
				sleep10ms();
			}
			if (header.moisture) {
				// Do 16-bit moisture sensing, leaving the
				//   data in the dynamic memory pool
				//Do not do moisture sensing every epoch.
				//Use moisture_counter (count down) to determine how often.
				//MOISTURE_STEPS is moisture interval/wake interval in settings.h
				//doMoistureSense in moisture.c
				//True does sensing and resets counter
				//False returns cached value
				if (moisture_counter) {
					doMoistureSense(false);
				} else {
					moisture_counter = MOISTURE_STEPS;
					doMoistureSense(true);
				}
				//Sleep 10 ms to allow the capacitor to recharge
				sleep10ms();
			}
		}

		//After doing the sensing types we need to sleep the rest of
		//the interval until it is time to transmit the packet
		SleepLPM3( MAX_SENSE_DELAY * enabled_sense_types * vloMsMult);



		/*
		 *  Update history every time sensing is performed.  This may result is a cached value
		 *  being "carried over" to the new hour.  Harder to determine if sensing is "fresh"
		 *  while keeping code simple.
		 *  TODO: Correct the carry-over problem described above.
		 */
		if (header.htuSensing || header.relativeLight) {
			extern float htu_lastTemp, lastRH;
			extern uint8_t cached_light;
			addHistory((int) htu_lastTemp, (int) lastRH, cached_light);
		}

		// Annoying check, but need to know if battery will be added later
		if (ENABLE_HISTORY && (history_counter < 1 && (doTransmit || battery_counter < 1))) {
			header.vivaristatHistory = ENABLE_HISTORY; // 0 turns off history, 1 turns on history sensing
			// Need extra byte for index value
			uint8_t* historyPtr = malloc(sizeof(HistoryUnit)+1);
			historyPtr[0] = historyIndex;
			HistoryUnit* oldestHistory = getHistory();

			int i = 0;
			for (; i < sizeof(HistoryUnit); ++i) {
				historyPtr[i+1] = ((uint8_t*) oldestHistory)[i];
			}
			++numHistory;
			history_counter = HISTORY_STEPS;
		}

		if (battery_counter < 1) {
			battery_counter = BATTERY_STEPS;
			header.battery = 1;
			doBatterySense();
			doTransmit = true;
		}

		if (doTransmit) {

			doTransmit = 0;
			tx_counter = params.packet_interval / WAKE_INTVL;
			++numRadio;

			// Reset the repeat-counter
			if (repeat_tx_remain) {
				--repeat_tx_remain;

				switch (repeat_tx_remain) {
				case 2:
					repeat_counter = SENSE_REPEAT_INTVL_0 / WAKE_INTVL;
					break;
				case 1:
					repeat_counter = SENSE_REPEAT_INTVL_1 / WAKE_INTVL;
					break;
				}
			}

			//Force temperature measurement if none are enabled
			if (!updatedTemp
					&& !(header.temp7_binary || header.temp16_fixed
							|| (HTU_ONBOARD && header.htuSensing))) {
				getTemperature();
			}

			/* Wake CC1101 and wait for oscillator to stabilize */
			Wake_up_CC1101();

			/* Re-write all the control registers */
			ReWriteCC1101Registers();

			int doRecalibrate = (recal_counter <= 1) || recalRadioFromTemp();

			if (doRecalibrate) {
				//Recalibrate with the current frequency setting
				recalibrateCC11xx(params.freq);

				recalibrateVLO(false);

				//Reset the calibration counter
				recal_counter = RECAL_STEPS;
			} else {
				/* Re-write all the control registers responsible for setting the PLL */
				restoreCC11xxRegs();
			}

			/*
			 * Load transmit FIFO registers with the packet length, which is
			 * 3, with possibly a header and sensed data
			 */
			if (0 < getAllSize()) {
				baseBuffer[0] = 3 + 1 + getAllSize();
			} else {
				baseBuffer[0] = 3;
			}

			setPowerSettings();

			/* TODO: Need 150 microseconds for the PLL to settle */
			/*TI_CC_Wait(100);*/
			//__delay_cycles(903);
			transmitAndPwrDown((char*) baseBuffer, header);
		}
		//Free the memory pool (which may have sensed data in it,
		//whether or not we sent the data).
		freeAll();

		/** Clear the battery header bit **/
		header.battery = 0;
		header.vivaristatHistory = 0;
		// Clear temp check flag
		updatedTemp = false;

		/* TODO: turn off the CC1150 supply */

		//Go to sleep for either the default time or the time specified with
		//optical programming, waking up a bit earlier to do sensing
		//before the radio trnsmit time
		SleepLPM3( (WAKE_INTVL - MAX_SENSE_DELAY*enabled_sense_types)* vloMsMult);
	}
}
