/*
 * Copyright (c) 2011, Make, Hack, Void Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the Make, Hack, Void nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL MAKE, HACK, VOID BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Fade an LED on Arduino pin 13
 * Uses a 16 bit timer for PWM and an 8 bit timer for animation
 */
#define OUTPUT_PIN	MHV_ARDUINO_PIN_13

// Bring in the MHV IO header
#include <MHV_io.h>

// Bring in the MHV 8 bit timer header
#include <MHV_Timer8.h>

// Bring in the MHV 16 bit timer header
#include <MHV_Timer16.h>

// Bring in the AVR interrupt header (needed for cli)
#include <avr/interrupt.h>

// Bring in the power management header
#include <avr/power.h>
#include <avr/sleep.h>

#include <avr/eeprom.h>

/* Declare an 8 bit timer - we will use Timer 2 since it is an 8 bit timer
 * on all microcontrollers used on Arduino boards
 */
MHV_Timer8 animationTimer(MHV_TIMER8_2);

/* Each timer module generates interrupts
 * We must assign the timer object created above to handle these interrupts
 * Since we only need one interrupt assigned, we can save some space by not
 * assigning the others
 */
MHV_TIMER_ASSIGN_1INTERRUPT(animationTimer, MHV_TIMER2_INTERRUPTS);

/* Declare a 16 bit timer for PWM output
 */
MHV_Timer16 pwmTimer(MHV_TIMER16_1);
MHV_TIMER_ASSIGN_2INTERRUPTS(pwmTimer, MHV_TIMER1_INTERRUPTS);

/* The maximum value of the PWM
 * This defines the resolution of the PWM, as well as the frequency
 */
#define PWM_TOP 10000

// How much to move the PWM duty cycle by each time the animation timer fires
#define PWM_INCREMENT 100


/* Our animation trigger function that will be called every time the animation
 * timer is triggered
 *
 * This will fade the LED up to 50% and back down to 0
 */
void animationTrigger(void *data) {
/* static variables are initialised once at boot, and persist between calls
 * What is the next action to take
 */
	static bool fadeUp = true;

/* Get the current output, and increment/decrement it based on the direction
 * Note that we are only going to 50% duty cycle as it is difficult to see
 * the difference between 50% & 100% duty cycle on LEDs. This could be fixed
 * by gamma correcting the output (see MHV_GammaCorrect)
 */
	uint16_t current = pwmTimer.getOutput2();

	if (fadeUp && current + PWM_INCREMENT >= PWM_TOP / 2) {
		fadeUp = false;
		pwmTimer.setOutput2(PWM_TOP / 2);
	} else if (!fadeUp && current <= PWM_INCREMENT) {
		fadeUp = true;
		pwmTimer.setOutput2(0);
	} else if (fadeUp) {
		pwmTimer.setOutput2(current + PWM_INCREMENT);
	} else {
		pwmTimer.setOutput2(current - PWM_INCREMENT);
	}
}

// Callbacks for the PWM timer
void ledOn(void *data) {
	mhv_pinOn(OUTPUT_PIN);
}

void ledOff(void *data) {
	mhv_pinOff(OUTPUT_PIN);
}


int main(void) {
	// Disable all peripherals and enable just what we need
	power_all_disable();
	power_timer2_enable();
	power_timer1_enable();
	set_sleep_mode(SLEEP_MODE_IDLE);

	// Set the initial EEPROM value if required
#define INITIAL_TIMEOUT 10
#define TIMEOUT_ADDRESS (uint8_t *)1023
	if (INITIAL_TIMEOUT != eeprom_read_byte(TIMEOUT_ADDRESS)) {
		eeprom_write_byte (TIMEOUT_ADDRESS, INITIAL_TIMEOUT);
	}


	/* Enable output on the output pin - see the declaration above
	 */
	mhv_setOutput(OUTPUT_PIN);

	/* Trigger the animation routine every 20ms
	 */
	(void) animationTimer.setPeriods(20000UL, 0);

	// Tell the timer to call our trigger function
	animationTimer.setTriggers(animationTrigger, 0, 0, 0);

	// Set the PWM mode to FAST PWM
	//pwmTimer.setMode(MHV_TIMER_16_PWM_FAST);

	// Tell the pwmTimer which functions to use when times elapse
	pwmTimer.setTriggers(ledOn, 0, ledOff, 0, 0, 0);

	// Set the PWM prescaler to 1 (no prescaler)
	pwmTimer.setPrescaler(MHV_TIMER_PRESCALER_5_1);

	/* Set the TOP value of the PWM timer - this defines the resolution &
	 * frequency of the PWM output
	 *
	 * PWM frequency = F_CPU / 2 / PWM_TOP
	 *               = 16,000,000 / 2 / 10000
	 *               = 800Hz
	 *
	 * PWM frequency = F_CPU / 2 / PWM_TOP
	 *               = 20,000,000 / 2 / 10000
	 *               = 1KHz
	 */
	pwmTimer.setOutput1(PWM_TOP);

	// Start with the PWM duty cycle set to 0
	pwmTimer.setOutput2(20);

	// Start the timers
	animationTimer.enable();
	pwmTimer.enable();

	// Enable interrupts
	sei();

	/* Do nothing forever - the timer will call the animationTrigger() function
	 * periodically
	 */
	for (;;) {
		sleep_mode();
	}

	// Main must return an int, even though we never get here
	return 0;
}
