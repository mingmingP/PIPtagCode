//******************************************************************************
//  Description:  This file contains definitions specific to the hardware board.
//  Specifically, the definitions include hardware connections with the
//  CCxxxx connector port, LEDs, and switches.
//
//  MSP430/CC1100-2500 Interface Code Library v1.1
//
//  W. Goh
//  Texas Instruments, Inc.
//  December 2009
//  Built with IAR Embedded Workbench Version: 4.20
//******************************************************************************
// Change Log:
//******************************************************************************
// Version:  1.1
// Comments: Added support for EXP5438 board
// Version:  1.00
// Comments: Initial Release Version
//******************************************************************************
//#define __MSP430G2553__

/*#define TI_CC_LED_PxOUT         P1OUT
#define TI_CC_LED_PxDIR         P1DIR
#define TI_CC_LED1              BIT0
#define TI_CC_LED2              BIT1*/

/*
#define TI_CC_SW_PxIN           P2IN
#define TI_CC_SW_PxIE           P2IE
#define TI_CC_SW_PxIES          P2IES
#define TI_CC_SW_PxIFG          P2IFG
#define TI_CC_SW_PxREN          P2REN
#define TI_CC_SW_PxOUT          P2OUT
#define TI_CC_SW1               BIT6
#define TI_CC_SW2               BIT7
*/
/* Modified for TPIP1.0 */
#define TI_CC_GDO0_PxOUT        P1OUT
#define TI_CC_GDO0_PxIN         P1IN
#define TI_CC_GDO0_PxDIR        P1DIR
#define TI_CC_GDO0_PxIE         P1IE
#define TI_CC_GDO0_PxIES        P1IES
#define TI_CC_GDO0_PxIFG        P1IFG
#define TI_CC_GDO0_PIN          BIT0
/* Modified for TPIP1.0 */
#define TI_CC_GDO1_PxOUT        P1OUT
#define TI_CC_GDO1_PxIN         P1IN
#define TI_CC_GDO1_PxDIR        P1DIR
#define TI_CC_GDO1_PIN          BIT6

/* CS is on P3.6 for TPIP 1.0 */
#define TI_CC_CSn_PxOUT         P3OUT
#define TI_CC_CSn_PxDIR         P3DIR
#define TI_CC_CSn_PIN           BIT6


//----------------------------------------------------------------------------
// Select which port will be used for interface to CCxxxx
//----------------------------------------------------------------------------
#define TI_CC_RF_SER_INTF       TI_CC_SER_INTF_USCIB0  // Interface to CCxxxx

/* Commented as SPI drivers for 5xx should not be used */
//#define TI_5xx                              // Using a 5xx
#define TI_Gxx                              // Using a 5xx
