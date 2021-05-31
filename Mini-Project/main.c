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

const unsigned int FPS = 30;
const unsigned int SCALER = 200 - 1;
const unsigned int PERIOD = 225000000/(SCALER+1);		// 60 Hz
const unsigned int NUM_TASKS = 5;
const unsigned int NUM_MISSILES = 15;
const unsigned int NUM_METEORS = 8;
const unsigned int COOLDOWN_TIMER = 100;

typedef void (*TaskFunction) ( void );

bool left, right, up, down, shootL, shootR, pause;

int shipX, shipY;
int ship_index;
int thruster_index;

int bg_index;

int meteors[NUM_METEORS][2];
int meteor_timer_start[NUM_METEORS];
int meteor_timer_elapsed[NUM_METEORS];

int meteor_index;

int explosion_enable[NUM_METEORS];
int explosions[NUM_METEORS][2];
int explosion_timer[NUM_METEORS];

int earth_index;

int missiles[NUM_MISSILES][2];
int missile_enable[NUM_MISSILES];
int missile_count;
int cooldown;

int health, score;

unsigned int scancode;

void init()
{
	srand(time(NULL));

	PS2_setInterrupt(1);

	Timer_initialise(0xFFFEC600);
	Timer_setLoad(0xFFFFFFFF);
	Timer_setControl(SCALER, 0, 1, 1);

	VGA_clearDisplay();

	bg_index = rand() % 9;
}

void reset()
{
	int i;
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

		explosions[i][0] = 10000;
		explosions[i][1] = 10000;
		explosion_enable[i] = 0;
	}

	shipX = (SCREEN_WIDTH - SHIP_WIDTH)/2, shipY =  (SCREEN_HEIGHT - SHIP_HEIGHT)/2;

	score = 0; health = 10;
	*LED_ptr = ~((signed int) -1 << health);
	DE1SoC_SevenSeg_SetSixDec(score);

	ship_index = 2, meteor_index = 0, thruster_index = 0, earth_index = 0;
}

void draw_ship()
{
	VGA_drawSprite(background[bg_index], ship[ship_index], shipX, shipY, SHIP_WIDTH, SHIP_HEIGHT);
	VGA_drawBackground(background[bg_index], shipX, shipY + SHIP_HEIGHT, SHIP_WIDTH, THRUSTER_HEIGHT);
	if (right) {
		VGA_drawSprite(background[bg_index], thruster[thruster_index], shipX+THRUSTER_OFS[ship_index]+(SHIP_WIDTH-THRUSTER_WIDTH)/2, shipY+THRUSTER_Y, THRUSTER_WIDTH, THRUSTER_HEIGHT);
		VGA_drawSprite(background[bg_index], thruster[thruster_index], shipX-THRUSTER_OFS[4-ship_index]+(SHIP_WIDTH-THRUSTER_WIDTH)/2, shipY+THRUSTER_Y, THRUSTER_WIDTH, THRUSTER_HEIGHT);
	} else {
		VGA_drawSprite(background[bg_index], thruster[thruster_index], shipX-THRUSTER_OFS[4-ship_index]+(SHIP_WIDTH-THRUSTER_WIDTH)/2, shipY+THRUSTER_Y, THRUSTER_WIDTH, THRUSTER_HEIGHT);
		VGA_drawSprite(background[bg_index], thruster[thruster_index], shipX+THRUSTER_OFS[ship_index]+(SHIP_WIDTH-THRUSTER_WIDTH)/2, shipY+THRUSTER_Y, THRUSTER_WIDTH, THRUSTER_HEIGHT);
	}
}

void move_ship()
{
	if (left && shipX > 0) {
		shipX = shipX - 1;
	} else if (right && shipX < SCREEN_WIDTH - SHIP_WIDTH) {
		shipX = shipX + 1;
	}

	if (up && shipY > 0) {
		shipY = shipY - 1;
	} else if (down && shipY < SCREEN_HEIGHT - SHIP_HEIGHT - THRUSTER_HEIGHT) {
		shipY = shipY + 1;
	}

	draw_ship();
}

void shoot_missile()
{
	if ((shootL || shootR) && cooldown == 0){
		if (shootR) {
			missiles[missile_count][0] = shipX + (4*SHIP_WIDTH)/5 - (MISSILE_WIDTH/2);
			missiles[missile_count][1] = shipY;
		} else if (shootL) {
			missiles[missile_count][0] = shipX + (SHIP_WIDTH)/5 - (MISSILE_WIDTH/2);
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
			VGA_drawSprite(background[bg_index], missile, missiles[i][0], missiles[i][1], MISSILE_WIDTH, MISSILE_HEIGHT);
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
				VGA_drawSprite(background[bg_index], meteor[(meteor_index + i)%8], meteors[i][0], meteors[i][1], METEOR_SIZE, METEOR_SIZE);
			}
		}

		if (explosion_enable[i] == 1) {
			VGA_drawSprite(background[bg_index], explosion[10-explosion_timer[i]], explosions[i][0], explosions[i][1], METEOR_SIZE, METEOR_SIZE);
		}
	}
}

void animation()
{
	int i;

	thruster_index = (thruster_index + 1) % 6;
	meteor_index = (meteor_index + 1) % 8;

	for (i = 0; i < NUM_METEORS; i++) {
		if (explosion_enable[i] == 1) {
			explosion_timer[i] = explosion_timer[i] - 1;

			if (explosion_timer[i] == -1) {
				explosion_enable[i] = 0;
			}
		}
	}

	if ((left && ship_index > 0) || (!right && ship_index > 2)) {
		ship_index--;
	} else if ((right && ship_index < 4) || (!left && ship_index < 2)) {
		ship_index++;
	}

}

void display_score(int x, int y)
{
	char str[10];
	sprintf(str, "%d", score);
	VGA_drawString("          ", x, y);
	VGA_drawString(str, x, y);

	DE1SoC_SevenSeg_SetSixDec(score);
}

void display_health()
{
	VGA_drawSquare(0x57EA, 135, SCREEN_HEIGHT - 25, health > 0 ? health*10:0, 10);

	*LED_ptr = ~((signed int) -1 << health);
}

void collision()
{
	int i, j;

	for (i = 0; i < NUM_METEORS; i++) {
		for (j = 0; j < NUM_MISSILES; j++) {
			if (missile_enable[j] == 1) {
				if (missiles[j][0] + MISSILE_WIDTH >= meteors[i][0] && missiles[j][0]  <= meteors[i][0] + METEOR_SIZE) {
					if (missiles[j][1] <= meteors[i][1] + METEOR_SIZE && missiles[j][1] + MISSILE_HEIGHT >= meteors[i][1]) {
						VGA_drawBackground(background[bg_index], meteors[i][0], meteors[i][1], METEOR_SIZE, METEOR_SIZE);
						VGA_drawBackground(background[bg_index], missiles[j][0], missiles[j][1], MISSILE_WIDTH, MISSILE_HEIGHT);

						// increase score based on meteor speed
						score = score + 50*(5-meteor_timer_start[i]);

						explosion_enable[i] = 1;
						explosion_timer[i] = 11;
						explosions[i][0] = meteors[i][0];
						explosions[i][1] = meteors[i][1];

						meteors[i][0] = rand() % (320 - METEOR_SIZE);
						meteors[i][1] = -1 * (rand() % 1000) - METEOR_SIZE;
						meteor_timer_start[i] = rand() % 5;

						missile_enable[j] = 0;

						display_score(65, 5);
					}

					if (missiles[j][1] + MISSILE_HEIGHT <= 0){
						missile_enable[j] = 0;
					}
				}
			}
		}

		if(meteors[i][1] >= SCREEN_HEIGHT){
			meteors[i][0] = rand() % (320 - METEOR_SIZE);
			meteors[i][1] = -1 * (rand() % 1000) - METEOR_SIZE;
			meteor_timer_start[i] = rand() % 5;

			health = health - 1;
			VGA_drawBackground(background[bg_index], 135, SCREEN_HEIGHT - 25, 100, 10);
		}

		if((shipX + 10<= meteors[i][0] + METEOR_SIZE && shipX + SHIP_WIDTH - 10 >= meteors[i][0]) && (shipY <= meteors[i][1] + METEOR_SIZE && shipY + SHIP_WIDTH >= meteors[i][1])){
			VGA_drawBackground(background[bg_index], meteors[i][0], meteors[i][1], METEOR_SIZE, METEOR_SIZE);
			meteors[i][0] = rand() % (320 - METEOR_SIZE);
			meteors[i][1] = -1 * (rand() % 1000) - METEOR_SIZE;
			meteor_timer_start[i] = rand() % 5;

			health = health - 3;
			VGA_drawBackground(background[bg_index], 135, SCREEN_HEIGHT - 25, 100, 10);
		}
	}
}

void PS2_input()
{
	scancode = PS2_readInput();

	if (scancode == 0xF06B) 			{ left 		= 0; }
	else if (scancode == 0xE06B) 		{ left		= 1; }

	if (scancode == 0xF074) 			{ right 	= 0; }
	else if (scancode == 0xE074) 		{ right		= 1; }

	if (scancode == 0xF075) 			{ up 		= 0; }
	else if (scancode == 0xE075) 		{ up		= 1; }

	if (scancode == 0xF072) 			{ down 		= 0; }
	else if (scancode == 0xE072) 		{ down		= 1; }

	if (scancode == 0xF01C) 			{ shootL	= 0; }
	else if ((scancode & 0xFF) == 0x1C) { shootL 	= 1; }

	if (scancode == 0xF023) 			{ shootR	= 0; }
	else if ((scancode & 0xFF) == 0x23) { shootR	= 1; }
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
	unsigned int lastIncrTime [3] = {0};
	const unsigned int incrPeriod [3] = {PERIOD/10, PERIOD/20, PERIOD/4};
	int earthX = (SCREEN_WIDTH - EARTH_WIDTH)/2, earthY = SCREEN_HEIGHT - EARTH_HEIGHT/3;
	int titleX = (SCREEN_WIDTH - TITLE_WIDTH)/2, titleY = 20;

	reset();
	Timer_setLoad(0xFFFFFFFF);

	while (scancode) { PS2_input(); HPS_ResetWatchdog(); }

	VGA_drawBackground(background[bg_index], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	VGA_drawSprite(background[bg_index], title, titleX, titleY, TITLE_WIDTH, TITLE_HEIGHT);
	VGA_drawString("PRESS ANY KEY TO START", 30, 15);

	while (!scancode) {
		if ((lastIncrTime[2] - Timer_readValue()) >= incrPeriod[2]) {
			earth_index = (earth_index + 1) % 8;
			VGA_drawSprite(background[bg_index], ship[ship_index], shipX, shipY, SHIP_WIDTH, SHIP_HEIGHT);
			VGA_drawSprite(background[bg_index], earth[earth_index], earthX, earthY, EARTH_WIDTH, EARTH_HEIGHT/3);
			lastIncrTime[2] -= incrPeriod[2];
			PS2_input();
		}
		HPS_ResetWatchdog(); // reset the watchdog.
	}

	while (scancode) { PS2_input(); HPS_ResetWatchdog(); }

	memset(lastIncrTime, 0, sizeof lastIncrTime);
	VGA_drawString("                      ", 30, 15);
	Timer_setLoad(0xFFFFFFFF);

	while (earthY < SCREEN_HEIGHT) {
		if ((lastIncrTime[0] - Timer_readValue()) >= incrPeriod[0]) {
			shipY = shipY - 1;
			animation();
			draw_ship();

			lastIncrTime[0] -= incrPeriod[0];
		}

		if ((lastIncrTime[1] - Timer_readValue()) >= incrPeriod[1]) {
			earthY = earthY + 1;
			titleY = titleY - 1;
			VGA_drawSprite(background[bg_index], earth[earth_index], earthX, earthY, EARTH_WIDTH, EARTH_HEIGHT/3);
			VGA_drawSprite(background[bg_index], title, titleX, titleY, TITLE_WIDTH, TITLE_HEIGHT);
			lastIncrTime[1] -= incrPeriod[1];
		}

		if ((lastIncrTime[2] - Timer_readValue()) >= incrPeriod[2]) {
			earth_index = (earth_index + 1) % 8;
			lastIncrTime[2] -= incrPeriod[2];
		}
		PS2_input();
		HPS_ResetWatchdog(); // reset the watchdog.
	}

	Timer_setLoad(0xFFFFFFFF);
	VGA_drawBackground(background[bg_index], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void defend_earth()
{
	int i;

	unsigned int lastIncrTime[NUM_TASKS] = {0};
	const unsigned int incrPeriod[NUM_TASKS] = {PERIOD/150, PERIOD/500, PERIOD/500, PERIOD/10, PERIOD/100};
	TaskFunction taskFunctions[NUM_TASKS] = {&move_ship, &shoot_missile, &move_missiles, &animation, &move_meteors};

	VGA_drawString("HEALTH:", 25, 55);
	display_health();
	display_score(65, 5);

	while (health > 0) {
		PS2_input();

		pause_screen();

		collision();

		for (i = 0; i < NUM_TASKS; i++) {
			if ((lastIncrTime[i] - Timer_readValue()) >= incrPeriod[i]) {
				taskFunctions[i]();
				lastIncrTime[i] -= incrPeriod[i];
			}
		}
		display_health();
		HPS_ResetWatchdog(); // reset the watchdog.
	}

	VGA_drawString("       ", 25, 55);
	VGA_drawString("       ", 65, 5);
}

void gameover()
{
	int i;
	unsigned int lastIncrTime [5] = {0};
	const unsigned int incrPeriod [5] = {PERIOD/10, PERIOD/50, PERIOD/4, PERIOD/50, PERIOD/80};
	int earthX = (SCREEN_WIDTH - EARTH_WIDTH)/2, earthY = SCREEN_HEIGHT + OVER_HEIGHT + 60;
	int overX = (SCREEN_WIDTH - OVER_WIDTH)/2, overY = SCREEN_HEIGHT + 30;
	unsigned int score_msg[6] = {51,35,47,50,37,26};

	Timer_setLoad(0xFFFFFFFF);

	VGA_drawBackground(background[bg_index], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	while (overY > 20) {
		if ((lastIncrTime[0] - Timer_readValue()) >= incrPeriod[0]) {
			animation();
			lastIncrTime[0] -= incrPeriod[0];
		}

		if ((lastIncrTime[1] - Timer_readValue()) >= incrPeriod[1]) {
			earthY = earthY - 1;
			overY = overY - 1;
			VGA_drawSprite(background[bg_index], destroyed_earth[earth_index], earthX, earthY, EARTH_WIDTH, EARTH_HEIGHT);
			VGA_drawSprite(background[bg_index], game_over, overX, overY, OVER_WIDTH, OVER_HEIGHT);
			lastIncrTime[1] -= incrPeriod[1];
		}

		if ((lastIncrTime[2] - Timer_readValue()) >= incrPeriod[2]) {
			earth_index = (earth_index + 1) % 8;
			lastIncrTime[2] -= incrPeriod[2];
		}

		if ((lastIncrTime[3] - Timer_readValue()) >= incrPeriod[3]) {
			shipY = shipY - 1;
			draw_ship();
			lastIncrTime[3] -= incrPeriod[3];
		}

		if ((lastIncrTime[4] - Timer_readValue()) >= incrPeriod[4]) {
			for (i = 0; i < NUM_METEORS; i++){
				if(meteors[i][1] >= 0) {
					meteors[i][1] = meteors[i][1] + 1;
					VGA_drawSprite(background[bg_index], meteor[(meteor_index + i)%8], meteors[i][0], meteors[i][1], METEOR_SIZE, METEOR_SIZE);
				}
			}
			lastIncrTime[4] -= incrPeriod[4];
		}
		PS2_input();
		HPS_ResetWatchdog(); // reset the watchdog.
	}

	memset(lastIncrTime, 0, sizeof lastIncrTime);

	VGA_drawMultiChar(score_msg, 6, 0xFFFF, (SCREEN_WIDTH - 70)/2, 20 + OVER_HEIGHT + 10, 5, 8, 1);
	VGA_drawDec(score, 0xFFFF, (SCREEN_WIDTH - 70)/2 + 40, 20 + OVER_HEIGHT + 10, 5, 8, 1);

	Timer_setLoad(0xFFFFFFFF);

	while (!scancode) {
		if ((lastIncrTime[2] - Timer_readValue()) >= incrPeriod[2]) {
			earth_index = (earth_index + 1) % 8;
			VGA_drawSprite(background[bg_index], destroyed_earth[earth_index], earthX, earthY, EARTH_WIDTH, EARTH_HEIGHT);
			lastIncrTime[2] -= incrPeriod[2];
			PS2_input();
		}
		HPS_ResetWatchdog(); // reset the watchdog.
	}

	while (scancode) { PS2_input(); HPS_ResetWatchdog(); }

	Timer_setLoad(0xFFFFFFFF);
	VGA_drawBackground(background[bg_index], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

int main ()
{
	init();

	while (1) {
		intro();
		defend_earth();
		gameover();
	}
}
