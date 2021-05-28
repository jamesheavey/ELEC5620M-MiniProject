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
#include "DE1SoC_SevenSeg/DE1SoC_SevenSeg.h"
#include "HPS_PrivateTimer/HPS_PrivateTimer.h"

#include "sprites/sprite.h"

const int SCREEN_WIDTH 	= 320;
const int SCREEN_HEIGHT = 240;

volatile unsigned int *LED_ptr = (unsigned int *) 0xFF200000;	// LEDs base address
volatile unsigned int *key_ptr = (unsigned int *) 0xFF20005C;	// key buttons edge capture base address

const unsigned int SCALER = 200 - 1;
const unsigned int PERIOD = 225000000/(SCALER+1);		// 60 Hz
const unsigned int NUM_TASKS = 5;
const unsigned int NUM_MISSILES = 10;
const unsigned int NUM_METEORS = 5;
const unsigned int COOLDOWN_TIMER = 100;

typedef void (*TaskFunction) ( void );

bool left, right, up, down, shoot, pause;

int shipX, shipY;
int ship_index;
int thruster_index = 0;

int bg_index;

int meteors[NUM_METEORS][2];
int meteor_timer_start[NUM_METEORS];
int meteor_timer_elapsed[NUM_METEORS];

int meteor_index = 0;

int missiles[NUM_MISSILES][2];
int missile_enable[NUM_MISSILES];
int missile_count = 0;
int cooldown = 0;

void init()
{
	int i;
	srand(time(NULL));

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

	for (i = 0; i < NUM_METEORS; i++){
		meteors[i][0] = rand() % (320 - METEOR_SIZE);
		meteors[i][1] = -1 * (rand() % 1000) - METEOR_SIZE;
		meteor_timer_start[i] = rand() % 5;
		meteor_timer_elapsed[i] = meteor_timer_start[i];
	}

	shipX = (SCREEN_WIDTH - PLAYER_WIDTH)/2, shipY =  SCREEN_HEIGHT - PLAYER_HEIGHT - 10;

	bg_index = rand() % 9;
	ship_index = rand() % 4;
}

void move_ship()
{
	if (left && shipX > 0) {
		shipX = shipX - 1;
	} else if (right && shipX < SCREEN_WIDTH - PLAYER_WIDTH) {
		shipX = shipX + 1;
	}

	if (up && shipY > 0) {
		shipY = shipY - 1;
	} else if (down && shipY < SCREEN_HEIGHT - PLAYER_HEIGHT) {
		shipY = shipY + 1;
	}

	VGA_drawBGSprite(background[bg_index], player_ship[ship_index], shipX, shipY, PLAYER_WIDTH, PLAYER_HEIGHT);

	if (ship_index != 2) {
		VGA_drawBGSprite(background[bg_index], thruster[thruster_index], shipX+(PLAYER_WIDTH-THRUSTER_WIDTH+1)/2, shipY+thruster_y_offset[ship_index], THRUSTER_WIDTH, THRUSTER_HEIGHT);
	} else {
		VGA_drawBGSprite(background[bg_index], thruster[thruster_index], shipX-6+(PLAYER_WIDTH-THRUSTER_WIDTH+1)/2, shipY+thruster_y_offset[ship_index], THRUSTER_WIDTH, THRUSTER_HEIGHT);
		VGA_drawBGSprite(background[bg_index], thruster[thruster_index], shipX+5+(PLAYER_WIDTH-THRUSTER_WIDTH+1)/2, shipY+thruster_y_offset[ship_index], THRUSTER_WIDTH, THRUSTER_HEIGHT);
	}
}

void shoot_missile()
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

void move_missiles()
{
	int i;

	for (i = 0; i < NUM_MISSILES; i++){
		if(missile_enable[i]){
			missiles[i][1] = missiles[i][1] - 1;
			VGA_drawBGSprite(background[bg_index], missile, missiles[i][0], missiles[i][1], MISSILE_WIDTH, MISSILE_HEIGHT);

			if (missiles[i][1] + MISSILE_HEIGHT <= 0){
				missile_enable[i] = 0;
			}
		}
	}
}

void move_meteors()
{
	int i;

	for (i = 0; i < NUM_METEORS; i++){
		meteor_timer_elapsed[i] = meteor_timer_elapsed[i] - 1;

		if (meteor_timer_elapsed[i] <= 0) {
			meteor_timer_elapsed[i] = meteor_timer_start[i];
			meteors[i][1] = meteors[i][1] + 1;

			if((meteors[i][1] <= SCREEN_HEIGHT) && (meteors[i][1] >= -METEOR_SIZE)) {
				VGA_drawBGSprite(background[bg_index], meteor[(meteor_index + i)%8], meteors[i][0], meteors[i][1], METEOR_SIZE, METEOR_SIZE);
			}
		}

		if(meteors[i][1] >= SCREEN_HEIGHT){
			meteors[i][0] = rand() % (320 - METEOR_SIZE);
			meteors[i][1] = -1 * (rand() % 1000) - METEOR_SIZE;
			meteor_timer_start[i] = rand() % 5;
			meteor_timer_elapsed[i] = meteor_timer_start[i];

			// lose life
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

void animation()
{
	thruster_index = (thruster_index + 1) % 4;
	meteor_index = (meteor_index + 1) % 8;
}

void PS2_input()
{
	unsigned int scancode = PS2_readInput();

	if (scancode == 0xF06B) 			{ left 	= 0; }
	else if (scancode == 0xE06B) 		{ left	= 1; }

	if (scancode == 0xF074) 			{ right = 0; }
	else if (scancode == 0xE074) 		{ right	= 1; }

	if (scancode == 0xF075) 			{ up 	= 0; }
	else if (scancode == 0xE075) 		{ up	= 1; }

	if (scancode == 0xF072) 			{ down 	= 0; }
	else if (scancode == 0xE072) 		{ down	= 1; }

	if (scancode == 0xF029) 			{ shoot	= 0; }
	else if ((scancode & 0xFF) == 0x29) { shoot = 1; }

	DE1SoC_SevenSeg_SetDoubleHex(0, scancode & 0xFF);
	DE1SoC_SevenSeg_SetDoubleHex(2, (scancode & 0xFF00) >> 8);

	*LED_ptr = (right) | (left<<1) | (up<<2) | (down<<3);
}

void pause_screen()
{
	if (*key_ptr & 0x8) {
		*key_ptr = 0xF;
		Timer_setControl(SCALER, 0, 1, 0);

		VGA_drawString("PAUSED", 40, 30);

		while (!(*key_ptr & 0x8)) { PS2_input(); HPS_ResetWatchdog(); }

		VGA_drawString("      ", 40, 30);

		*key_ptr = 0xF;
		Timer_setControl(SCALER, 0, 1, 1);
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
	const unsigned int incrPeriod[NUM_TASKS] = {PERIOD/200, PERIOD/500, PERIOD/500, PERIOD/10, PERIOD/100}; 		// set the increment period for all timer units
	TaskFunction taskFunctions[NUM_TASKS] = {&move_ship, &shoot_missile, &move_missiles, &animation, &move_meteors};		// define task function struct to call increment functions when required

	init();

	intro();

	while (1) {
		PS2_input();

		pause_screen();

		for (i = 0; i < NUM_TASKS; i++) {
			if ((lastIncrTime[i] - Timer_readValue()) >= incrPeriod[i]) {
				taskFunctions[i]();
				lastIncrTime[i] -= incrPeriod[i];
			}
		}

		HPS_ResetWatchdog(); // reset the watchdog.
	}
}
