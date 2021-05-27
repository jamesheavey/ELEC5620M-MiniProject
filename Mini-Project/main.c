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

void init()
{
	PS2_setInterrupt(1);

	VGA_clearDisplay();
}

void move_character(int* x)
{
	unsigned int scancode = PS2_readInput();

	if (scancode == 0xE06B && *x > 0) {
		*x = *x - 1;
	} else if (scancode == 0xE074 && *x < SCREEN_WIDTH - PLAYER_WIDTH) {
		*x = *x + 1;
	}
}

//void copytoscreenbuffer(const unsigned short* sprite, unsigned int xleft, unsigned int ytop, unsigned int width, unsigned int height)
//{
//	int x, y;
//
//	for (y = ytop; y < ytop + height; y++) {
//		for (x = xleft; x < xleft + width; x++) {
//			screen_buffer[y*(SCREEN_WIDTH) + x] = *sprite++;
//		}
//	}
//}

int main ()
{
	int x = 100, y = 0;
	char byte1 = 0, byte2 = 0;
	volatile unsigned int byte12;

	init();

	VGA_drawSprite(background, 0, 0, 320, 240);

	VGA_drawString("STAR WARS", 10, 10);

	while (1) {
		move_character(&x);

		VGA_drawBGSprite(background, player_ship, x, SCREEN_HEIGHT - PLAYER_HEIGHT - 10, PLAYER_WIDTH, PLAYER_HEIGHT);

		HPS_ResetWatchdog(); // reset the watchdog.
	}
}
