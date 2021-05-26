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

int main () {
	VGA_clearDisplay();

	VGA_drawSprite(dog, 0, 0, 320, 240);

	VGA_drawString("doge", 10, 10);

	while (1) {
		HPS_ResetWatchdog(); // reset the watchdog.
	}
}
