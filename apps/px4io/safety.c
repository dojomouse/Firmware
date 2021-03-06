/****************************************************************************
 *
 *   Copyright (C) 2012 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

 /**
  * @file Safety button logic.
  */

#include <nuttx/config.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <debug.h>
#include <stdlib.h>
#include <errno.h>

#include <nuttx/clock.h>

#include <drivers/drv_hrt.h>

#include "px4io.h"

static struct hrt_call arming_call;

/*
 * Count the number of times in a row that we see the arming button
 * held down.
 */
static unsigned counter;

#define ARM_COUNTER_THRESHOLD	10
#define DISARM_COUNTER_THRESHOLD	2

static bool safety_led_state;
static bool safety_button_pressed;
static void safety_check_button(void *arg);

void
safety_init(void)
{
	/* arrange for the button handler to be called at 10Hz */
	hrt_call_every(&arming_call, 1000, 100000, safety_check_button, NULL);
}

static void
safety_check_button(void *arg)
{
	/* 
	 * Debounce the safety button, change state if it has been held for long enough.
	 *
	 */
	safety_button_pressed = BUTTON_SAFETY;

	if(safety_button_pressed) {
		//printf("Pressed, Arm counter: %d, Disarm counter: %d\n", arm_counter, disarm_counter);
	}

	/* Keep pressed for a while to arm */
	if (safety_button_pressed && !system_state.armed) {
		if (counter < ARM_COUNTER_THRESHOLD) {
			counter++;
		} else if (counter == ARM_COUNTER_THRESHOLD) {
			/* change to armed state and notify the FMU */
			system_state.armed = true;
			counter++;
			system_state.fmu_report_due = true;
		}
	/* Disarm quickly */
	} else if (safety_button_pressed && system_state.armed) {
		if (counter < DISARM_COUNTER_THRESHOLD) {
			counter++;
		} else if (counter == DISARM_COUNTER_THRESHOLD) {
			/* change to disarmed state and notify the FMU */
			system_state.armed = false;
			counter++;
			system_state.fmu_report_due = true;
		}
	} else {
		counter = 0;
	}

	/* when armed, toggle the LED; when safe, leave it on */
	if (system_state.armed) {
		safety_led_state = !safety_led_state;
	} else {
		safety_led_state = true;
	}
	LED_SAFETY(safety_led_state);
}
