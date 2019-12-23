/*
 * optical_conn.c
 *
 *  Created on: Aug 29, 2013
 *      Author: Robert Moore
 */

#include "optical_conn.h"

int optBuff[MAX_OPTICAL_BYTES];
//extern double freq;
//extern long packet_interval;
//extern unsigned long boardID;

void flashLights(int x) {
	/* this function flashes the LED of the TPIP*/

	P2DIR |= 01; // set P2.0 and P3.0 as output
	P3DIR |= 01;

	int i = 0;
	int j = 0;
	for (j = 0; j < x; j++) //sets iteration of x flashes
			{
		for (i = 0; i < 50; i++) //duty cycle of 1/11
				{
			P2DIR |= 01; // set P2.0 and P3.0 as output
			P3DIR |= 01;

			P3OUT &= ~0x01;
			P2OUT |= 0x01; //P2.0 high, P3.0 low

			_delay_cycles(100);

			P2OUT &= ~0x01;

			_delay_cycles(1000);
		}
		_delay_cycles(100000);
	}
}

void lightsetup(void) {
	P2REN &= ~0x01; //disable resistors for P2.0
	P2OUT &= ~0x01; //make low when output
	P2DIR |= 0x01; //make P2.0 output

	P3SEL2 &= ~0x01;

	TA0CCTL2 = 0; //CLEAR TIMER
	TA0CTL = TACLR;
	unsigned int i = 0;
	for (i = 0; i < 5; i++) //delay
			{
	}

	//Start Timer_A (SMCLK, Continuous)
	TA0CTL = TASSEL_2 + MC_2; //SMCLK , continuous.  TA1CTL sets timer timer1_A3, not 0_A3
	TA0CCR2 = 0;

	//Set up CC for capture
	TA0CCTL2 = CM_2 + CCIS_0 + CAP + SCS;
	TA0CCTL2 &= ~CCIE;
}

/*
 * light sensing with Rev 1.1, TPIP-K returns a bool for optical sensing
 * make sure to run lightSetup() first
 */
int lightSense(void) {

	//Set Output Pin High
	P3SEL &= ~0x01;							//Select bit has to be 0 to be high
	P3DIR &= ~0x01;
	P3OUT |= 0x01;
	P3REN |= 0x01;						      //enable Pull up resistor for P3.0

	// Delay to fully charge
	int i;
	for (i = 0; i < 5; i++) {
	}

	//clear flag
	TA0CCTL2 &= ~CCIFG; 					//CCIFG is the flag

	//change pin P3.0 to input
	P3REN &= ~0x01;						//disable resistors for P3.0
	P3SEL |= 0x01;						//Select bits changed for CC

	_delay_cycles(1000);

	if (!(TA0CCTL2 & CCIFG)) {
		return 0;						// if no light
	}
	//return TA0CCR2;				// if you want to return discharge time
	return 1;						// if binary return
}

/**
 * Updates the tag's parameters via optical communication.
 */
void opticalReceive(volatile TagParameters* params) {
	/*
	 this function allows the TPIP to receive OPT_BYTES bytes of data via optical comm
	 Bytes 0 and 1 are an unlock key
	 Byte 2 is header
	 Bytes 3, 4, and 5 are ID
	 Byte 6 sets frequency:  put 0x16 (22 in decimal) for 922Mhz
	 Bytes 7 and 8 set packet interval.   packet interval = 600L*((optBuff[7]<<8)+optBuff[8])
	 The 9th byte is end code
	 */

	WDTCTL = WDTPW + WDTHOLD; // Stop WDT

	lightsetup(); //needs to be called to set up light sensing

	UCA0CTL1 |= UCSWRST; //set swrt....will change to ACLK
	UCA0CTL0 = UCPEN + UCPAR; //parity even, LSB first, 8 bits, asynchronous, UART mode

	P1DIR |= 0x01; //make P1.0 (LED) an output
	P1OUT &= ~0x01; //make P1.0 low, use an indicator later
	P3SEL &= ~0x80; //set for I/O
	P3SEL2 &= ~0x80;
	P3DIR |= 0x80; // make P3.7 an output
	P3REN &= ~0x80; //disable P3.7 resistors

	DCOCTL = 0; // Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_1MHZ; // Set DCO
	DCOCTL = CALDCO_1MHZ;

	/* Set P1.1 to UART RX-Ready, P1.2 to UART TX-Ready */
	P1SEL |= BIT1 + BIT2;
	P1SEL2 |= BIT1 + BIT2;
	UCA0CTL1 |= UCSSEL_2; // SMCLK

	UCA0BR0 = 0xCF; // sets baud rate
	UCA0BR1 = 0x00;
	UCA0MCTL = 0x11;

	UCA0CTL1 &= ~UCSWRST; // clear SWRT **Initialize USCI state machine**

	P1REN |= 0x02;

	int i = 0;
	int j = 0;

	for (j = 0; j < MAX_OPTICAL_BYTES; j++) {
		//TODO FIXME the & in the loop should really be a logical and (&&)
		//stay in loop until data received, or 5 SEC
		//~870 iterations per second
		while (!(IFG2 & UCA0RXIFG)&& (i < TIME_WAIT)){

		if (lightSense())
		{
			P1OUT |= 0x02;
		} else {
			P1OUT &= ~0x02;
		}
		i++;
	}
	optBuff[j] = UCA0RXBUF;
}

		//Make sure the unlock code was received and the last byte is the end code
	if ((optBuff[0] = OPTICAL_UNLOCK1) & (optBuff[1] == OPTICAL_UNLOCK2)
			& (optBuff[MAX_OPTICAL_BYTES - 1] == OPTICAL_END)) // if unlock and ending codes correct
			{
		//Start past the unlock code and scan for key-value pairs
		int index = 2;
		while (index < MAX_OPTICAL_BYTES && optBuff[index] != OPTICAL_END) {
			if (KEY_ID == optBuff[index]) {
				params->boardID = (((unsigned long) optBuff[index + 1]) << 16)
						+ (optBuff[index + 2] << 8) + optBuff[index + 3]; // sets board ID
				index += 4;
			} else if (KEY_FREQ == optBuff[index]) {
				params->freq = 900000000 + optBuff[index + 1] * 1000000; // sets Frequency
				index += 2;
			} else if (KEY_INTVL == optBuff[index]) {
				// sets packet interval, in multiples of 600 (1/20 of a sec)
				params->packet_interval = 600L
						* ((optBuff[index + 1] << 8) + optBuff[index + 2]);
				index += 3;
			} else if (KEY_HDR == optBuff[index]) {
				params->header = optBuff[index + 1];
				index += 2;
			} else {
				//Error -- unrecognized code.
				//For now just quit
				return;
			}
		}

		flashLights(20); //flash several times when received data
	} else { //else use defaults
//		freq = DEFAULT_FREQ;
//		packet_interval = PACKTINTVL;
		flashLights(2); //flash twice if defaults used
	}

	UCA0CTL1 |= UCSWRST; // Set UART Reset - Holds state machine
}

void opticalTransmit(void) {
	/*this function is for transmitting an optical signal using the TI Launchpad.
	 attach P1.2 to the gate of the MOSFET, 5V to the drain, ground the source.

	 Bytes 0 and 1 are an unlock key
	 Byte 2 is header
	 Bytes 3, 4, and 5 are ID
	 Byte 6 sets frequency:  put 0x16 (22 in decimal) for 922Mhz
	 Bytes 7 and 8 set packet interval.   packet interval = 600* (tbuff[8]*8+ tbuff[7])
	 The 9th byte is end code
	 */

	WDTCTL = WDTPW + WDTHOLD; // Stop WDT

	UCA0CTL1 |= UCSWRST; //set SWRT, send address
	UCA0CTL0 = UCPEN + UCPAR;

	P1DIR |= 0x01;
	DCOCTL = 0; // Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_1MHZ; // Set DCO
	DCOCTL = CALDCO_1MHZ;

	P1SEL |= BIT1 + BIT2; // P1.1 = RXD, P1.2=TXD
	P1SEL2 = BIT1 + BIT2;
	UCA0CTL1 |= UCSSEL_2; //SMCLK

	UCA0BR0 = 0xCF; //These set baud rate
	UCA0BR1 = 0x00;

	UCA0MCTL = 0x11; // Modulation UCBRSx = 5
	UCA0CTL1 &= ~UCSWRST; // **Initialize USCI state machine**

	_delay_cycles(60000);

	//What gets transmitted
	int optTxBuff[MAX_OPTICAL_BYTES] = { OPTICAL_UNLOCK1, OPTICAL_UNLOCK2,
			OPTICAL_END };

	//Start with the first two unlock bytes
	int count = 1;

	if (WRITE_ID) {
		optTxBuff[++count] = KEY_ID;
		optTxBuff[++count] = IDCODE1;
		optTxBuff[++count] = IDCODE2;
		optTxBuff[++count] = IDCODE3;
	}

	if (WRITE_FREQ) {
		optTxBuff[++count] = KEY_FREQ;
		optTxBuff[++count] = FREQCODE;
	}

	if (WRITE_INTVL) {
		optTxBuff[++count] = KEY_INTVL;
		optTxBuff[++count] = INTVL1;
		optTxBuff[++count] = INTVL2;
	}

	if (WRITE_HDR) {
		optTxBuff[++count] = KEY_HDR;
		optTxBuff[++count] = HDRCODE;
	}
	++count;

	//Fill the rest of the buffer with end bytes
	while (count < MAX_OPTICAL_BYTES) {
		optTxBuff[count] = OPTICAL_END;
		count++;
	}

	for (count = 0; count < MAX_OPTICAL_BYTES; count++) {
		while (!(IFG2 & UCA0TXIFG))
			; //transmitting
		UCA0TXBUF = optTxBuff[count];
		P1OUT ^= BIT0; // see led flash as sends
	}
	P1OUT &= ~0x01; //turn off led

	UCA0CTL1 |= UCSWRST; // Set UART Reset - Holds state machine
}

