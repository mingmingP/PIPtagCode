;*******************************************************************************
;   Alarm Clock Software
;
;   G. Vannucci
;   February, 2012
;*******************************************************************************
;            QUICK START
; -- Use this routine to put the CPU to sleep.  The argument specifies when the
;    CPU should wake up, as a time interval since the previous wake-up time.
; -- This routine is useful for writing a program that performs a sequence of
;    actions that must occur, very precisely, at specified time intervals.
; -- Write your program as a sequence of actions separated by sleep intervals.
;    You need not worry about how long the CPU takes to execute each action.
;    When an action is done, your program should calculate the precise time
;    inrerval that must elapse between the *beginning* of the just-completed
;    action and the beginning of the next one.
; -- Call this function with the calculated value as argument.  The function
;    will return at precisely the time when the next action must start.
; -- If you need a logic-level output change to occur at the wake-up time (for
;    example, to trigger an external device through an output pin) you can set
;    up Timer-A for that (read more details below).
;*******************************************************************************
;             DETAILED DESCRIPTION
;  This software implements an alarm clock.  The Watchdog Timer and Timer-A are
;used together to keep track of time.  You can use this software to put the CPU
;to sleep in LPM3 mode, and to make it wake up at a specified time.
;  The main goal is to achieve a very low power consumption when the CPU is
;asleep while retaining the flexibility of specifying wake-up time with fine
;granularity, even if the time intervals are very long (e.g., hundreds of
;seconds).
;  The goal of low power is achieved by using the Watchdog Timer (WDT) as an
;interval timer to generate interrupts; but, to have fine granularity, you would
;have to choose a short time interval because, then, you would be able to specify
;wake-up time as a multiple of that time interval.
;  You could, for example, choose the shortest time interval implemented by the
;WDT, which is 64 ACLK cycles, and then you could count an integer number
;of WDT interrupts before waking up the CPU.  However, if you calculate the power
;consumed by the interrupt service routine, you will find that it is too much.
;  To solve that problem, this software takes advantage of the fact that the
;Watchdog Timer can implement several time intervals.  The desired wake-up time
;interval is broken into a sequence of WDT time intervals, beginning with the
;longest time intervals allowed by the WDT (32,768 cycles of ACLK) followed by
;the shorter implemented time intervals.  This way we can have the
;granularity of the shortest WDT intervals, but long intervals are implemented
;mosty with infrequent interrupts, which keeps power consumption low.
;  To achieve even finer granularity, after the WDT intervals are done, we add
;a delay implemented with Timer-A counting cycles of ACLK.  Since the
;granularity of the shortest WDT time interval is 64 cycles of ACLK, it means
;that Timer-A is used for, at most, 64 counts, which keeps the added power
;consumption low; but we get the granularity of one ACLK cycle.
;  There is another benefit in using Timer-A: The output of Timer-A can be
;connected to a pin, so that, when the alarm clock "rings", and the CPU is woken
;up, you can have a logic-signal change at that pin at the very precise time
;when the ringing occurs.  You can use that signal to control an external device
;where you need very precise timing.
;
;  The software is implemented as two functions that you can call from C.  (See
;the accompanying test program in C for the function prototypes).
;  The function "AlrmClkStrt" must be called to initialize the alarm clock.  The
;function "SleepLPM3" can then be called with a long argument that specifies
;the time interval, in units of ACLK cycles, for how long the CPU should sleep.
;  SleepLPM3 puts the CPU to sleep in LPM3 mode, and returns when the specified
;time interval has elapsed.  The time interval is interpreted as beginning not
;from the present time (i.e., the time when SleepLPM3 is called) but, rather,
;from the time of the previous wake-up, as implemented by the previous call to
;SleepLPM3 or to AlrmClkStrt.
;  This system makes it easy to achieve precise timing of events because the
;time it takes to execute a program becomes irrelevant: when the CPU is
;awakened (i.e., when SleepLPM3 function returns) the CPU can do what it needs
;to do without concern for the execution time that is required.  When done, the
;program should calculate the time when the next thing must happen, starting from
;the time when the last thing happened, and then call SleepLPM3 with that time as
;the argument.  The SleepLPM3 routine will return at precisely the specified time.
;
;  To simplify software for the microcontroller, there are certain constraints
;that must be met, but the software does not check that these constraints are
;respected.  Violate them at your peril:
;   -- It is the responsibilty of the calling program to set up ACLK and MCLK.
;      The interrupt handlers require that there be >=40 cycles of MCLK for each
;      cycle of ACLK.  For example, when ACLK=32,768 Hz, the frequency of MCLK
;      should be no less than ~1.32MHz.  Make sure that the combination of ACLK
;      and MCLK is such that there are at least 40 MCLK cycles for each ACLK cycle.
;   -- The Watchdog Timer is used by AlrmClkStrt and SleepLPM3 to keep track of
;      time.  All other programs should leave the Watchdog Timer alone.
;   -- The SleepLPM3 program uses Timer-A, but only while the CPU is sleeping
;      (i.e., while the call to SleepLPM3 is in effect).  It is the responsibility
;      of the calling program to make sure that Timer-A is available, and not
;      running when SleepLPM3 is called.  The calling program can, however,
;      configure the three OUTMOD bits in CCR0 as desired to make the EQU0 event
;      appear on an output pin.  The SleepLPM3 program preserves the OUTMOD bits.
;   -- This software uses the Timer-A CCR0 interrupt vector, but it's only used
;      while SleepLPM3 is running.  If other programs need to use CCR0 interrupts,
;      you will have to modify the interrupt handler.  For this software, the
;      functionality of the interrupt handler is very simple: it wakes up the CPU;
;      and it does not matter how long it takes to execute, as long as it's
;      repeatable.  So, feel free to overwrite the interrupt handler, just make
;      sure that, when an interrupt occurs during SleepLPM3, it wakes up the CPU
;      in a repeatable amount of time.
;   -- It is the responsibility of the calling program to shut down all
;      peripherals and subsystems to achieve low power consuption during LPM3
;      sleep.  SleepLPM3 will put the CPU into LPM3 sleep, but will not change
;      the status of peripherals.
;   -- If other interrupts (other than Watchdog Timer and Timer-A interrupts,
;      which are managed by SleepLPM3) occur during LPM3 sleep, you should add
;      the execution time of the longest interrupt handler to the 40-CPU-cycles
;      requirement stated above.  If this modified requirement is met, the
;      Timer-A transition will still occur at precisely the scheduled time,
;      and the software will work without problem, but the return from SleepLPM3
;      can no longer be guaranteed to occur in a repeatable time because of the
;      presence of the other interrupts.
;   -- SleepLPM3 must be called no later than 65535 ACLK cycles after the
;      previous event (the last time SleepLPM3 returned, or the call to
;      AlrmClkStrt).  Note: this is not a constraint on the argument passed to
;      SleepLPM3 which can be as large as you want (see next item), this is a
;      constraint on how long the CPU can stay awake before going back to sleep
;      by calling SleepLPM3 again.  That next call must occur before 65535 ACLK
;      cycles have passed since the previous call to SleepLPM3, or else the 16-bit
;      counter that keeps track of time overflows.  This constraint means, for
;      example, that, with an ACLK frequency of 32768 Hz, the CPU can stay
;      awake for up to about two seconds before going back to sleep by calling
;      SleepLPM3 again.
;   -- The argument passed to SleepLPM3 is a (signed) long.  It must be positive,
;      but it can be as large as the max allowed (2^31 - 1).  Therefore, the
;      maximum sleeping time can be very long (e.g., with ACLK=32768 Hz, max
;      sleeping time is >18 hrs.).
;   -- The argument passed to SleepLPM3 must be large enough that there is enough
;      time to set up the alarm clock.  When the CPU is awake, the Watchdog
;      Timer is running and is configured to generate interrupts every 64 ACLK
;      cycles.  Each of those interrupts is an opportunity to set the next
;      wake-up time, and the earliest wake-up time that can be set must be
;      at least one ACLK cycle after the next interrupt.  Therefore, if the CPU
;      has been awake less than 64 ACLK cycles, the earliest possible next wake-up
;      time, is 132 ACLK cycles from the previous wake-up time, which means that
;      the argument must always be >=132.  It will need to be larger if the CPU has
;      been awake longer than 64 ACLK cycles, meaning that the first opportunity
;      for setting the alarm clock has already passed.  For example, if the CPU has
;      been awake 64 or more ACLK cycles, but less than 128 ACLK cycles, the argument
;      must be >=132+64, and so on.
			.global SleepLPM3				;entry point from C
			.global AlrmClkStrt				;entry point from C
			.cdecls C,LIST,  "msp430.h"
;------------------------------------------------------------------------------
            .text                           ; Flash Memory
AlrmClkStrt:	;  ---------  Subroutine to initialize the alarm clock ---------
;The Watchdog Timer is configured and started.
;The interrupt-handler status is initialized at level 0, where it is idling at the
;highest rate, waiting for the next alarm time to be set by calling AlrmClk.
			clr		Counter					;start counting interrupts
			mov		#WDT_ADLY_1_9,&WDTCTL	;reset and start the WDT with ratio=64
			bis.b	#WDTIE,&IE1				;Enable WDT interrupt
			eint							;enable all interrupts
			ret			
;----------------------------------------------------------------------------			
SleepLPM3:	;  ---------  Subroutine to set the alarm clock ---------
;  This subroutine is used to put the CPU to sleep in LP3 mode while waiting for the
;time in the future for when the CPU is to be woken up.  
;  It takes one argument (long) which specifies the wake-up time as a time
;interval since the last previously-specified wake-up time.  It is in units
;of ACLK cycles.
;  After the specified number of ACLK cycles has elapsed, since the previous wake-up
;time, the CPU is taken out of LP3 mode, and the subroutine returns.  It is designed
;to return in a constant number of CPU cycles; so that, if no other interrupts occur,
;control is returned to the calling program in a repeatable amount of time.
;  This subroutine uses Timer-A.  The calling program must make sure that Timer-A
;is not being used for something else when this subroutine is called.
;  The EQU0 event of Timer-A is what marks the wake-up time.  Therefore,
;if EQU0 is connected to an output pin, the pin will experience a level transition
;at the wake-up with excellent precision.  The calling program can set up
;Timer-A with the desired OUTMODx configuration to get the desired EQU0 signal out.
;This program will not change the OUTMODx configuration.  Note: make sure Timer-A
;is not running
;
;  While the CPU is awake, the Watchdog Timer keeps running to keep track of elapsed
;time.  It runs with the lowest divide ratio of 64, so that the WDT interrupts are
;occurring every 64 cycles of ACLK and the interrupt handler decrements Counter through
;negative values.   
			;when this program is called, we don't know how long it will be before
			;the next WDT interrupt.  It might happen very soon.  To make sure we
			;have enough time to do what we need to do without a WDT interrupt occurring
			;while we manipulate parameters, at the beginning, we just go to sleep and
			;wait for the next WDT interrupt without doing anything.  When the interrupt
			;comes, we know we have a good chunk of time (64 ACLK cycles) to do
			;what we have to do before the next interrupt comes.  We then set up the
			;Watchdog Timer and Timer-A to implement a series of sleep intervals whose
			;combined total is the desired sleep interval specified by the argument.
			dint							;to avoid interferences
			mov		Counter,R14				;get the value of Counter
			mov		#1,Counter				;and ask to be woken up at next interrupt
            bis		#LPM3+GIE,SR            ;enter LPM3 sleep, enable interrupts
            ;we were just woken up by a WDT interrupt.
			;here we calculate how many ACLK cycles we need to wait, after the next
			;interrupt, before we return to the calling program.  Then we implement
			;that wait with a combination of WDT interrupts and Timer-A interrupts.
			rla		R14						;multiply by 64 ...
			rla		R14						;  to convert Counter value...
			rla		R14						;  into the number of ACLK cycles...
			rla		R14						;  that will have elapsed...
			rla		R14						;  since the last wake-up...
			rla		R14						;  (it's a negative value)
			sub 	#132,R14				;to account for the fact that we copied R14
					;two interrupts early, and for the four cycles to be added later
					;to circumvent the TA12 bug (see Errata) of Timer-A
			add 	R14,R12					;add this negative number to the argument
			sbc 	R13						;don't forget the high bits
			;now we have converted the argument into the number of ACLK cycles to wait
			;until next wake-up.  This number must be broken up into five pieces, one for 
			;each counting ratio.  The top four pieces will be implemented by the WDT
			;timer; their bit sizes are, in order from MSB to LSB: 16, 2, 4, 3, for the
			;four divide ratios, respectively: /32768, /8192, /512, /64.  The lowest six
			;bits we'll use in CCR0 of Timer-A.  Let's do them first.
			mov		R12,R14					;copy the low 16 bits
			and		#0x3F,R14				;but keep only 6 bits
			add		#4,R14					;must be in the range 4-67 (see TA12 bug)
			mov		#TASSEL_1+TACLR,&TACTL	;reset & stop Timer-A, ACLK, /1, no int
			mov		R14,&TACCR0				;put the value where we'll need it
			;while we are at it, finish setting up Timer-A
			bic		#~OUTMOD_7,&CCTL0		;clear CCTL0 except for the OUTMOD bits
			clr		&CCTL1					;clear CCTL1 too, to be sure
			bis		#CCIE,&CCTL0			;enable Timer-A CCR0 interrupt
			;let's now get the four pieces for the WDT
			mov		R12,R14					;copy the low 16 bits
			rla		R14						;get the most significant 16 bits...
			rlc		R13						;into R13 - will be for /32768
			rla		R14						;place the 3 bits for /64 ...
			swpb	R14						;  into the low byte
			and		#0x7,R14				;keep only 3 bits for /64
			;the WDT is already counting with a /64 right now.  We can implement
			;this divide ratio right now
			inc		R14						;to account for the current interval
			mov		R14,Counter				;and it's implemented
			;let's do the remaining two pieces
			mov		R12,R14					;copy the low 16 bits
			rra		R14						;place the 4 bits for /512 ...
			swpb	R14						;  into the low byte
			and		#0xF,R14				;keep only 4 bits for /512
			rlc		R12						;place the 2 bits for /8192 ...
			rlc		R12						;
			rlc		R12						;
			rlc		R12						;  into the low byte
			and		#0x3,R12				;keep only 2 bits for /8192
			;now we implement the counting sequences specified by the various pieces
			jz		No8192					;if =0, we won't need to do /8192
			bis		#LPM3+GIE,SR            ;but if we do, wait for the end of /64
			mov		#WDT_ADLY_250,&WDTCTL	;reset and start WDT with ratio=8192
			mov 	R12,Counter				;do the prescribed number of /8192
No8192		tst		R14						;will we need to do /512
			jz		No512					;if =0, we won't need to do /512
			bis		#LPM3+GIE,SR            ;but if we do, wait to end this counting
			mov		#WDT_ADLY_16,&WDTCTL	;reset and start WDT with ratio=512
			mov 	R14,Counter				;do the prescribed number of /512
No512		tst		R13						;will we need to do /32768
			jz		No32768					;if =0, we won't need to do /32768
			bis		#LPM3+GIE,SR            ;but if we do, wait to end this counting
			mov		#WDT_ADLY_1000,&WDTCTL	;reset and start WDT with ratio=32768
			mov 	R13,Counter				;do the prescribed number of /32768
No32768		bis		#LPM3+GIE,SR            ;wait till the end of WDT counting
			mov		#WDTPW+WDTHOLD,&WDTCTL	;Stop WDT
			bis		#MC_2,&TACTL			;start Timer-A, contmode
			bis		#LPM3+GIE,SR            ;go to sleep, wait for Timer-A interrupt
			;this is the final wake-up call.  The time to sleep has passed.  We set
			;up the WDT timr to run with /64 while the CPU is awake.  Counter is now
			;at zero, so it will count how long the CPU is awake.
			bic		#MC_3,&TACTL			;stop Timer-A without changing anything
			mov		#WDT_ADLY_1_9,&WDTCTL	;reset and start the WDT with ratio=64
			;we are leaving the Watchdog Timer running with /64.  Counter is at zero,
			;and the interrupt handler will keep decrementing it, which will tell us
			;how much time has elapsed when this program is called next time.
			ret								;go back to the calling program
;---------------------------------------------------------------------------			
WDT_ISR:	;  ---------  Interrupt handler for Watchdog Timer ---------
;  This handler simply counts how many interrupts have occured and, at the end
;of the count, it wakes up the CPU.  The Counter is in RAM, no registers are
;used, and the execution time is very small, so that this handler can continue
;running (keeping track of elapsed time) even during normal CPU activity without
;causing excessive interference to the software that it interrupts.
			dec		Counter					;Count the interrupts
			jnz		JustRtrn				;if not done, go back to sleep
			bic		#LPM3,0(SP)             ;if done, wake up the CPU
JustRtrn	reti					
;----------------------------------------------------------------------------			
TA0_ISR:	;  ---------  Interrupt handler for TimerA ---------
;It's time to wake up the CPU.
			bic		#LPM3,0(SP)             ;wake up the CPU
            reti 
;-------------------------------------------------------------------------------
;			RAM variables
            .data                          
 			.bss	Counter,2,2             ;For counting interrupts
;------------------------------------------------------------------------------
;           Interrupt Vectors
            .sect   WDT_VECTOR				;WDT Vector
            .short  WDT_ISR                 ;
            .sect   TIMER0_A0_VECTOR		;TimerA Vector
            .short  TA0_ISR                 ;        
            .end
