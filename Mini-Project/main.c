/*
 * main.c
 *
 *  Created on: May 26, 2021
 *      Author: James
 */

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "DE1SoC_LT24/DE1SoC_LT24.h"
#include "HPS_Watchdog/HPS_Watchdog.h"
#include "HPS_usleep/HPS_usleep.h"
#include "DE1SoC_VGA/DE1SoC_VGA.h"
#include "DE1SoC_PS2/DE1SoC_PS2.h"
#include "HPS_PrivateTimer/HPS_PrivateTimer.h"

#include "sprites/sprite.h"

const int SCREEN_WIDTH 	= 320;
const int SCREEN_HEIGHT = 240;

volatile unsigned int *LED_ptr = (unsigned int *) 0xFF200000;	// LEDs base address
volatile unsigned int *key_ptr = (unsigned int *) 0xFF20005C;	// key buttons edge capture base address

const unsigned int SCALER = 200 - 1;
const unsigned int PERIOD = 225000000/(SCALER+1);		// 60 Hz
const unsigned int NUM_TASKS = 3;
const unsigned int NUM_MISSILES = 10;
const unsigned int COOLDOWN_TIMER = 10;

typedef void (*TaskFunction) ( void );

bool left, right, up, down, shoot;

int shipX, shipY;
int ship_index = 0;

int bg_index;

int missiles[NUM_MISSILES][2];
int missile_enable[NUM_MISSILES];
int missile_count = 0;
int cooldown = 0;

void init()
{
	int i;

	PS2_setInterrupt(1);

	Timer_initialise(0xFFFEC600);
	Timer_setLoad(0xFFFFFFFF);
	Timer_setControl(SCALER, 0, 1, 1);

	VGA_clearDisplay();

	for (i = 0; i < NUM_MISSILES; i++){
		missiles[i][0] = 10000;
		missiles[i][1] = 10000;
		missile_enable[i] = 0;
	}

	shipX = (SCREEN_WIDTH - PLAYER_WIDTH)/2, shipY =  SCREEN_HEIGHT - PLAYER_HEIGHT - 10;

	srand(time(NULL));
	bg_index = (rand()) % 9;
}

void move_ship()
{
	if (left && shipX > 0) {
		left = 0;
		shipX = shipX - 1;
	} else if (right && shipX < SCREEN_WIDTH - PLAYER_WIDTH) {
		right = 0;
		shipX = shipX + 1;
	} else if (up && shipY > 0) {
		up = 0;
		shipY = shipY - 1;
	} else if (down && shipY < SCREEN_HEIGHT - PLAYER_HEIGHT) {
		down = 0;
		shipY = shipY + 1;
	}

	VGA_drawBGSprite(background[bg_index], player_ship[ship_index], shipX, shipY, PLAYER_WIDTH, PLAYER_HEIGHT);
}

void shoot_laser()
{
	if (shoot && cooldown == 0){
		if (missile_count % 2 == 0){
			missiles[missile_count][0] = shipX + (3*PLAYER_WIDTH)/4 - (MISSILE_WIDTH/2);
			missiles[missile_count][1] = shipY;
		} else {
			missiles[missile_count][0] = shipX + (PLAYER_WIDTH)/4 - (MISSILE_WIDTH/2);
			missiles[missile_count][1] = shipY;
		}

		missile_enable[missile_count] = 1;
		missile_count = (missile_count + 1) % (NUM_MISSILES);

		cooldown = COOLDOWN_TIMER;
	} else if (cooldown != 0) {
		cooldown--;
	}
}

void move_laser()
{
	int i;

	for (i = 0; i < NUM_MISSILES; i++){
		if(missile_enable[i]){
			missiles[i][1] = missiles[i][1] - 1;
			VGA_drawBGSprite(background[bg_index], missile, missiles[i][0], missiles[i][1], MISSILE_WIDTH, MISSILE_HEIGHT);

			if (missiles[i][1] <= 0){
				missile_enable[i] = 0;
				VGA_clearArea(background[bg_index], missiles[i][0], missiles[i][1], MISSILE_WIDTH, MISSILE_HEIGHT);
			}
		}
	}
}

void change_ship_sprite()
{
	if (*key_ptr & 0x1) {
		ship_index = (ship_index + 1) % 4;
		*key_ptr = 0xF;
	}
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
	} else if (scancode == 0x29F0) {
		shoot	= 1;
	} else {
		left = 0, right = 0, up = 0, down = 0, shoot = 0;
	}
}

void intro()
{
	unsigned int scancode;

	VGA_drawSprite(background[bg_index], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	VGA_drawBGSprite(background[bg_index], title, (SCREEN_WIDTH - TITLE_WIDTH)/2,50, TITLE_WIDTH, TITLE_HEIGHT);

	VGA_drawString("PRESS ANY KEY TO START", 30, 30);

	while (!scancode) {
		scancode = PS2_readInput();
		change_ship_sprite();
		VGA_drawBGSprite(background[bg_index], player_ship[ship_index], shipX, shipY, PLAYER_WIDTH, PLAYER_HEIGHT);
		HPS_ResetWatchdog(); // reset the watchdog.
	}

	VGA_drawString("                      ", 30, 30);
	Timer_setLoad(0xFFFFFFFF);
	VGA_drawSprite(background[bg_index], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

int main ()
{
	int i;

	unsigned int lastIncrTime[NUM_TASKS] = {0};												// all timers start incrementing immediately
	const unsigned int incrPeriod[NUM_TASKS] = {PERIOD/200, PERIOD/500, PERIOD/500}; 		// set the increment period for all timer units
	TaskFunction taskFunctions[NUM_TASKS] = {&move_ship, &shoot_laser, &move_laser};		// define task function struct to call increment functions when required

	init();

	intro();

	while (1) {
		PS2_input();

		for (i = 0; i < NUM_TASKS; i++) {
			if ((lastIncrTime[i] - Timer_readValue()) >= incrPeriod[i]) {
				taskFunctions[i]();
				lastIncrTime[i] -= incrPeriod[i];
			}
		}

		HPS_ResetWatchdog(); // reset the watchdog.
	}
}
