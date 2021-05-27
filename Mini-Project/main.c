/*
 * main.c
 *
 *  Created on: May 26, 2021
 *      Author: James
 */

#include <stdbool.h>

#include "DE1SoC_LT24/DE1SoC_LT24.h"
#include "HPS_Watchdog/HPS_Watchdog.h"
#include "HPS_usleep/HPS_usleep.h"
#include "DE1SoC_VGA/DE1SoC_VGA.h"
#include "DE1SoC_PS2/DE1SoC_PS2.h"
#include "HPS_PrivateTimer/HPS_PrivateTimer.h"

#include "sprites/sprite.h"

const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 240;

volatile unsigned int *LED_ptr = (unsigned int *) 0xFF200000;	// LEDs base address

const unsigned int SCALER = 200 - 1;
const unsigned int PERIOD = 225000000/(SCALER+1);		// 60 Hz
const unsigned int NUM_TASKS = 1;

typedef void (*TaskFunction) (int*, int*);

bool left, right, up, down;

void init()
{
	PS2_setInterrupt(1);

	Timer_initialise(0xFFFEC600);
	Timer_setLoad(0xFFFFFFFF);
	Timer_setControl(SCALER, 0, 1, 1);

	VGA_clearDisplay();
}

void move_ship(int* x, int* y)
{
	if (left && *x > 0) {
		left = 0;
		*x = *x - 1;
	} else if (right && *x < SCREEN_WIDTH - PLAYER_WIDTH) {
		right = 0;
		*x = *x + 1;
	} else if (up && *y > 0) {
		up = 0;
		*y = *y - 1;
	} else if (down && *y < SCREEN_HEIGHT - PLAYER_HEIGHT) {
		down = 0;
		*y = *y + 1;
	}

	VGA_drawBGSprite(background, player_ship, *x, *y, PLAYER_WIDTH, PLAYER_HEIGHT);
}

void PS2_input()
{
	unsigned int scancode = PS2_readInput();

	if (scancode == 0xE06B) {
		left 	= 1;
	} else if (scancode == 0xE074) {
		right 	= 1;
	} else if (scancode == 0xE075) {
		up 		= 1;
	} else if (scancode == 0xE072) {
		down 	= 1;
	} else {
		left = 0, right = 0, up = 0, down = 0;
	}
}

int main ()
{
	int x = (SCREEN_WIDTH - PLAYER_WIDTH)/2, y =  SCREEN_HEIGHT - PLAYER_HEIGHT - 10;

	int i;

	unsigned int lastIncrTime[NUM_TASKS] = {0};						// all timers start incrementing immediately
	unsigned int timeValues[NUM_TASKS] = {0};						// all time values initialised to 0
	const unsigned int incrPeriod[NUM_TASKS] = {PERIOD/100}; 		// set the increment period for all timer units
	TaskFunction taskFunctions[NUM_TASKS] = {&move_ship};			// define task function struct to call increment functions when required

	init();

	VGA_drawSprite(background, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	VGA_drawString("STAR WARS", 10, 10);

	while (1) {
		PS2_input();

		for (i = 0; i < NUM_TASKS; i++) {
			if ((lastIncrTime[i] - Timer_readValue()) >= incrPeriod[i]) {
				taskFunctions[i](&x, &y);
				lastIncrTime[i] -= incrPeriod[i];
			}
		}

		HPS_ResetWatchdog(); // reset the watchdog.
	}
}
