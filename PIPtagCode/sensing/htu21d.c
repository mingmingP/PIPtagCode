/*
 * htu21d.cpp
 *
 * Communicate with the HTU21D temperature and relative humidity sensor over IIC.
 *
 *  Created on: Jun 24, 2014
 *      Author: Bernhard Firner
 */

#include "htu21d.h"
#include "sensing.h"
#include "temperature.h"
#include "../Owl/mem_pool.h"
#include "../interrupt.h"

//Assembly function defined in AlarmClock-v1.2.asm that implements ACLK delay
extern void SleepLPM3(long ACLKDly);

//Cached results
float htu_lastTemp;
float lastRH;
uint8_t* htu_temp16_location;
uint16_t htu_cached_temp16F = 0;
uint8_t* htu_RH16_location;
uint16_t htu_cached_RH16F = 0;

//Remember if the HTU needs to be initialized
bool htu_initialized = false;

//The clock and data pins for IIC communication
const char DATA = BIT1;
const char SCK = BIT2;
const char HTU_ADDRESS = 0x80;
const char HTU_READ_FLAG = 0x01;
const char HTU_HOLD_TEMP = 0xE3;
const char HTU_HOLD_RH = 0xE5;
const char HTU_TEMP = 0xF3;
const char HTU_RH = 0xF5;
const char HTU_NO_HOLD = 0x10;
const char HTU_WRITE_REG = 0xE6;
const char HTU_READ_REG = 0xE7;
const char HTU_SOFT_RESET = 0xFE;
const char ACK = 0x0;
const char NACK = 0x1;

//Equations from page 15 of the HTU21D sensor datasheet
//Temp = -46.85 + 175.72*Sensed / 2^16
const double HTU_TEMP_SLOPE = 0.002681; //175.72 / 65536.0;
//For Merck want temp offset to read zero at -40C
//Change temp offset by 40.
//const double HTU_TEMP_OFFSET = -6.85;  //40.0 C offset so temp range starts at -40C
const double HTU_TEMP_OFFSET = -46.85;  //Direct temp measurement, but lowest value reported is 0C

//RH = -6 + 125*Sensed / 2^16
const double HTU_RH_SLOPE = 0.001907349; //125 / 65536.0;
const int HTU_RH_OFFSET = -6;

//Delay between clock pulses
//Zero delay with just a function call if sufficient
#define delay_after_send 0
#define delay_ack 0

//The number of times to attempt reading from the HTU21D before giving up
const int read_retries = 3;
//Constant values used to indicate failure
#define CRC_FAIL 0xFFF;
#define READ_FAIL 0xFFE;

void doSenseHTUTemp16F(bool updateCached, bool calibration) {
	static bool first = true;

	if (updateCached || first) {
		first = false;
		uint16_t status = senseHTU21DTemperature(calibration);
		//Store the error code on failure, convert to fixed point on success
		if (0 != status) {
			htu_cached_temp16F = status;
		} else {
			htu_cached_temp16F = to16Fixed(htu_lastTemp);
		}
	}

	htu_temp16_location = malloc(2);
	htu_temp16_location[0] = ((uint8_t*) &htu_cached_temp16F)[1];
	htu_temp16_location[1] = ((uint8_t*) &htu_cached_temp16F)[0];
}

void doSenseHTURH16F(bool updateCached) {
	static bool first = true;

	if (updateCached || first) {
		first = false;
		uint16_t status = senseHTU21DHumidity();
		//Store the error code on failure, convert to fixed point on success
		if (0 != status) {
			htu_cached_RH16F = status;
		} else {
			htu_cached_RH16F = to16Fixed(lastRH);
		}
	}

	htu_RH16_location = malloc(2);
	htu_RH16_location[0] = ((uint8_t*) &htu_cached_RH16F)[1];
	htu_RH16_location[1] = ((uint8_t*) &htu_cached_RH16F)[0];
}

//Output the given byte over the IIC pins. Returns the ack bit
char writeIICByte(char byte) {
	//Assuming that the clock was low here
	P1REN &= ~DATA;
	P1DIR |= DATA;

	volatile unsigned int shift = 0;
	for (; shift < 8; ++shift) {
		//Set data high or low depending upon the current bit
		if (0x80 & byte) {
			P1OUT |= DATA;
		} else {
			P1OUT &= ~DATA;
		}
		//Clock goes high after changing the data value
		P1OUT |= SCK;
		__delay_cycles(delay_after_send);
		//Clock goes low when the value is ready to be changed again
		P1OUT &= ~SCK;
		//The slave should have read the bit at this point, go on to the next
		//Shift once after every transmission
		byte <<= 1;
	}

	//When finished the data turns to input (to get an ACK) and the clock goes high
	//Have to make sure that we pull up the input pin
	P1DIR &= ~DATA;
	__delay_cycles(delay_ack);
	P1OUT |= SCK;
	//Now we get an ack if the data line is pulled low
	char ack = ACK;
	if (P1IN & DATA) {
		ack = NACK;
		//Leave data line in this state
		P1OUT |= DATA;
	} else {
		P1OUT &= ~DATA;
	}
	//Revert data pin to our control
	P1DIR |= DATA;

	//Data clock low
	//NOTE! Technically this should go back to pull-up high
	//but we are claiming the IIC bus as our own so we keep it low
	P1OUT &= ~SCK;

	return ack;
}

//Port 1 interrupt so we can sleep while the HTU sensor is sensing
#pragma vector=PORT1_VECTOR
__interrupt void p1interrupt() {
	//Turn off the interrupt, clear the interrupt flag, and exit LPM3
	P1IE &= ~SCK;
	P1IFG &= ~SCK;
	LPM3_EXIT;
}

//Read a byte and send either ack or nack depending upon ack_type
unsigned char readIICByte(char ack_type) {
	//Data should be input, with pull-up resistor enabled
	P1DIR &= ~DATA;
	P1OUT |= DATA;

	//Now wait for the CLK line to go high to allow for clock stretching from the slave
	P1DIR &= ~SCK;
	P1OUT |= SCK;

	//If the clock is being held low use an interrupt to wake up
	if (0 == (P1IN & SCK)) {
		//Interrupt on the clock pin, rising edge
		P1IFG &= ~SCK;
		P1IES &= ~SCK;
		P1IE |= SCK;
		//Enter LPM3 to sleep for the interrupt
		LPM3;
	}

	P1DIR |= SCK;
	P1REN &= ~SCK;

	//Read into in_char one bit at a time
	unsigned char in_char = 0;
	int shift = 0;
	for (; shift < 8; ++shift) {
		//Shift over 1
		in_char <<= 1;

		//Read in the next bit - set CLK low
		P1OUT |= SCK;
		__delay_cycles(delay_after_send);
		if (P1IN & DATA) {
			in_char |= 1;
		}
		P1OUT &= ~SCK;
	}
	//Now send the specified ACK type
	P1REN &= ~DATA;
	P1DIR |= DATA;
	//NACK bit (DATA high)
	if (NACK == ack_type) {
		P1OUT |= DATA;
	} else {
		P1OUT &= ~DATA;
	}
	//Clock goes high
	P1OUT |= SCK;
	//Give the chip time to read the bit
	__delay_cycles(delay_ack);
	P1OUT &= ~SCK;
	//Claim the bus again
	return in_char;
}

void IICWarmup() {
	/*
	 * Send the start condition (HPC199_5 HTU21D(F) Sensor Datasheet page 10)
	 * Data goes low, then sck.
	 */
	//Set clock and data high to start
	P1REN &= ~(SCK | DATA);
	P1DIR |= (SCK | DATA);
	P1OUT |= (SCK | DATA);
	//Go to LPM1 during the 15ms warmup time
	//Reset the state of the timer
	TA0CTL |= TACLR;
	//Count up to TA0CCR0 using the 6MHz SMCLK.
	//Divide SMCLK by 4 and wait for 22,500 clock ticks to reach 15ms
	TA0CCR0 = 22500;
	TA0CCR1 = 0;
	//Compare mode with interrupt enabled
	TA0CCTL1 = CM_0 + CCIE;
	//Up mode, divide by 4 using SMCLK
	TA0CTL = MC_2 | ID_2 | TASSEL_2 | TAIE;
	// Sleep until wake-up.
	initializingHTU = true;
	__bis_SR_register(LPM1_bits + GIE);
	initializingHTU = false;

	//Drop data, then SCK
	P1OUT &= ~(DATA);
	__delay_cycles(32);
	P1OUT &= ~(SCK);
}

void IICStart() {
	/*
	 * Send the start condition (HPC199_5 HTU21D(F) Sensor Datasheet page 10)
	 * Data goes low, then sck.
	 */
	//Set clock and data high to start
	P1REN &= ~(SCK | DATA);
	P1DIR |= (SCK | DATA);
	P1OUT |= (SCK | DATA);
	//Wait for more than the minimum high time (0.6 microseconds)
	__delay_cycles(4);
	//Drop data, then SCK
	P1OUT &= ~(DATA);
	__delay_cycles(4);
	P1OUT &= ~(SCK);
}

void IICStop() {
	//Clock and data low
	P1REN &= ~(SCK | DATA);
	P1DIR |= (SCK | DATA);
	P1OUT &= ~(SCK | DATA);
	//Wait for the same interval as in the IICStart function (0.6 microseconds)
	//Then raise clock, followed by data
	__delay_cycles(4);
	P1OUT |= SCK;
	__delay_cycles(4);
	P1OUT |= DATA;
}

bool initHTU21D() {
	htu_initialized = false;
	//Write a soft reset (command 0xFE)
	IICWarmup();
	//Keep trying to contact the chip until it is ready and gives an ACK
	int attempts = 0;
	IICStart();
	while (NACK == writeIICByte(HTU_ADDRESS)) {
		IICStop();
		IICStart();
		++attempts;
		if (read_retries < attempts) {
			htu_initialized = false;
			return false;
		}
	}

	/*
	if (writeIICByte(HTU_SOFT_RESET)) {
		IICStop();
		return false;
	}

	IICStop();

	//Wait 15ms after a soft reset
	//Go to LPM1 during the 15ms warmup time
	//Reset the state of the timer
	TA0CTL |= TACLR;
	//Count up to TA0CCR0 using the 6MHz SMCLK.
	//Divide SMCLK by 4 and wait for 22,500 clock ticks to reach 15ms
	TA0CCR0 = 22500;
	TA0CCR1 = 0;
	//Compare mode with interrut enabled
	TA0CCTL1 = CM_0 + CCIE;
	//Up mode, divide by 4 using SMCLK
	TA0CTL = MC_2 | ID_2 | TASSEL_2 | TAIE;
	// Sleep until wake-up.
	initializingHTU = true;
	__bis_SR_register(LPM1_bits + GIE);
	initializingHTU = false;

	//Now set 8 bit relative humidity and 12 bit temperature
	//First read the user register (need to write back the same reserved bits)
	IICStart();
	if (writeIICByte(HTU_ADDRESS)) {
		IICStop();
		return false;
	}
	*/

	//Skipping the reset command due to strange behavior
	if (writeIICByte(HTU_READ_REG)) {
		IICStop();
		return false;
	}
	IICStart();
	if (writeIICByte(HTU_ADDRESS | HTU_READ_FLAG)) {
		IICStop();
		return false;
	}

	char user_reg = readIICByte(NACK);

	//Need to set Bit7 to 0 and Bit 0 to 1 for 8 bit RH and 12 bit temp
	user_reg &= 0x7F; //Clear highest bit (bit 7)
	user_reg |= 0x01; //lowest bit (bit 0)

	IICStart();
	if (writeIICByte(HTU_ADDRESS)) {
		IICStop();
		return false;
	}
	if (writeIICByte(HTU_WRITE_REG)) {
		IICStop();
		return false;
	}
	if (writeIICByte(user_reg)) {
		IICStop();
		return false;
	}
	IICStop();

	htu_initialized = true;
	return true;
}

//Apply simple CRC algorithm (non-tabular method)
unsigned char checkCRC(unsigned long msg, unsigned char crc) {
	//The polynomial is x^8 + x^5 + x^4 + x^0, or 0x131
	//The polynomial is shifted left to move the x^8 into the upper bit
	//of the third byte so that we can XOR from the
	//head of the input data (for convenience), yielding 0x988000
	unsigned long poly = 0x988000;
	//The high bit is used to check if we do an XOR
	unsigned long high_bit = 0x800000;
	//The last place to check is x^8 (no fractions in bitwise division)
	unsigned long last_check = 0x100;
	//Left shift and add in the CRC. After running the polynomial
	//through we should be left with a 0 remainder
	msg <<= 8;
	msg |= crc;
	while (last_check <= high_bit) {
		//Check if upper bit is a 1
		if (high_bit & msg) {
			msg ^= poly;
		}
		//Rotate right
		poly >>= 1;
		high_bit >>= 1;
	}
	return msg & 0xFF;
}

//Get the temperature from the HTU21D sensor over the IIC bus
uint16_t senseHTU21DTemperature(bool calibration) {
	if (!htu_initialized) {
		htu_lastTemp = READ_FAIL
		;
		return htu_lastTemp;
	}

	// Always start with init sequence.
	IICStart();
	//Keep trying to contact the chip until it is ready and gives an ACK
	int attempts = 0;
	while (NACK == writeIICByte(HTU_ADDRESS)) {
		IICStop();
		IICStart();
		++attempts;
		//Read failure
		if (read_retries < attempts) {
			htu_lastTemp = READ_FAIL
			;
			//We require re-initialization
			htu_initialized = false;
			return htu_lastTemp;
		}
	}
	//Read 12 bit temperature using HOLD master mode
	if (writeIICByte(HTU_HOLD_TEMP)) {
		IICStop();
		htu_initialized = false;
		return htu_lastTemp = READ_FAIL;
	}

	IICStart();
	if (writeIICByte(HTU_ADDRESS | HTU_READ_FLAG)) {
		IICStop();
		htu_initialized = false;
		return htu_lastTemp = READ_FAIL;
	}
	//Storing msb into an int for a future left shift, lsb is safe as a u_char
	unsigned int msb = readIICByte(ACK);
	unsigned char lsb = readIICByte(ACK);
	char checksum = readIICByte(NACK);
	IICStop();

	//Lowest 2 bits are just status information
	unsigned int reading = (msb << 8) | (lsb & 0xFC);

	//Valid temperature range -46.85C to 128C
	htu_lastTemp = HTU_TEMP_SLOPE * reading + HTU_TEMP_OFFSET;

	//Leave the pins low, but set DATA low first to try and avoid bus errors
	P1DIR &= ~(DATA);
	P1OUT &= ~DATA;
	P1DIR &= ~(SCK);
	P1OUT &= ~SCK;

	//Check CRC
	if (0 != checkCRC(msb << 8 | lsb, checksum)) {
		//Indicate error if the CRC failed
		htu_lastTemp = CRC_FAIL
		;
		return htu_lastTemp;
	}
	//If this is being used for calibration then set the temperature
	else if (calibration) {
		setLastTemp(htu_lastTemp);
	}

	return 0;
}

//Get the temperature from the HTU21D sensor over the IIC bus
uint16_t senseHTU21DHumidity() {
	if (!htu_initialized) {
		lastRH = READ_FAIL
		;
		return lastRH;
	}
	// Init each time, were previously in crazy state.
	IICStart();
	//Keep trying to contact the chip until it is ready and gives an ACK
	int attempts = 0;
	while (NACK == writeIICByte(HTU_ADDRESS)) {
		IICStop();
		IICStart();
		++attempts;
		//Read failure
		if (read_retries < attempts) {
			lastRH = READ_FAIL
			;
			//We require re-initialization
			htu_initialized = false;
			return lastRH;
		}

	}
	//Read 12 bit temperature using HOLD master mode
	if (writeIICByte(HTU_HOLD_RH)) {
		IICStop();
		htu_initialized = false;
		return lastRH = READ_FAIL
	}
	IICStart();
	if (writeIICByte(HTU_ADDRESS | HTU_READ_FLAG)) {
		IICStop();
		htu_initialized = false;
		return lastRH = READ_FAIL;
	}
	//Storing msb into an int for a future left shift, lsb is safe as a u_char
	unsigned int msb = readIICByte(ACK);
	unsigned char lsb = readIICByte(ACK);
	char checksum = readIICByte(NACK);
	IICStop();

	//Lowest 2 bits are just status information
	unsigned int reading = (msb << 8) | (lsb & 0xFC);
	//Convert to value
	lastRH = HTU_RH_SLOPE * reading + HTU_RH_OFFSET;

	//Leave the pins low, but set DATA low first to try and avoid bus errors
	P1DIR &= ~(DATA);
	P1OUT &= ~DATA;
	P1DIR &= ~(SCK);
	P1OUT &= ~SCK;

	//Check CRC
	if (0 != checkCRC(msb << 8 | lsb, checksum)) {
		//Indicate error if the CRC failed
		lastRH = CRC_FAIL
		;
		return lastRH;
	}
	if (htu_lastTemp < 250) {
		// Compensate for temperatures away from 25C
		lastRH = lastRH - .15 * (25 - htu_lastTemp);
	}
	//return 0 for success
	return 0;
}

