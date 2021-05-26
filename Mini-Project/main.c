/*
 * main.c
 *
 *  Created on: May 26, 2021
 *      Author: James
 */

#include "DE1SoC_LT24/DE1SoC_LT24.h"
#include "HPS_Watchdog/HPS_Watchdog.h"
#include "HPS_usleep/HPS_usleep.h"
#include "DE1SoC_VGA/DE1SoC_VGA.h"

#include "sprite.h"

const unsigned int SCALER = 200 - 1;
const unsigned int PERIOD = 225000000/(SCALER+1);		// A9 Private timer freq. = 225MHz

volatile int * PS2_ptr = (int *) 0xFF200100; // PS/2 port address

volatile unsigned int *LED_ptr = (unsigned int *) 0xFF200000;	// LEDs base address

typedef void (*TaskFunction)(char, int*, int*);

// Initialises A9 Private Timer
// Loads in max timer value, sets control bits
void Timer_init()
{
	Timer_initialise(0xFFFEC600);
	Timer_setLoad(0xFFFFFFFF);
	Timer_setControl(SCALER, 0, 1, 1);
}

void check_input( char code, int* x, int* y )
{
	if (code == 0x23){
		*x = *x + 1;
	} else if (code == 0x1C) {
		*x = *x - 1;
	} else if (code == 0x1D) {
		*y = *y + 1;
	} else if (code == 0x1B) {
		*y = *y - 1;
	}

	VGA_drawSprite(mario, *x, *y, 50, 62);
}

int main () {

	int PS2_data, RVALID;
	int x = 0, y = 0, i;
	char byte1 = 0, byte2 = 0, byte3 = 0;

	unsigned int lastIncrTime[1] = {0};						// all timers start incrementing immediately
	unsigned int timeValues[1] = {0};						// all time values initialised to 0
	const unsigned int incrPeriod[1] = {PERIOD/60}; 	// set the increment period for all timer units
	TaskFunction taskFunctions[1] = {&check_input};	// define task function struct to call increment functions when required

	Timer_init();

	VGA_clearDisplay();

	VGA_drawSprite(dog, 0, 0, 320, 240);

	VGA_drawString("doge", 10, 10);

	while (1) {
		PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
		RVALID = PS2_data & 0x8000; // extract the RVALID field
		if (RVALID)
		{
			byte1 = byte2;
			byte2 = byte3;
			byte3 = PS2_data & 0xFF;
		}

		*LED_ptr = byte2;

		for (i = 0; i < 1; i++) {
			if ((lastIncrTime[i] - Timer_readValue()) >= incrPeriod[i]) {
				taskFunctions[i](byte2, &x, &y);
				lastIncrTime[i] -= incrPeriod[i];
			}
		}

		HPS_ResetWatchdog(); // reset the watchdog.
	}
}
