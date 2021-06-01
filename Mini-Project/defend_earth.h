/*
 * defend_earth.h
 *
 *  Created on: May 31, 2021
 *      Author: James
 */

#ifndef DEFEND_EARTH_H_
#define DEFEND_EARTH_H_

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

// ADDRESS DEFINITIONS
volatile unsigned int *LED_ptr = (unsigned int *)0xFF200000; // LEDs base address
volatile unsigned int *key_ptr = (unsigned int *)0xFF20005C; // key buttons edge capture base address

// CONSTANT DEFININTIONS
const int SCREEN_WIDTH              = 320;
const int SCREEN_HEIGHT             = 240;
const unsigned int FPS              = 30;
const unsigned int SCALER           = 200 - 1;
const unsigned int PERIOD           = 225000000 / (SCALER + 1); // 1 Hz
const unsigned int NUM_TASKS        = 5;
const unsigned int NUM_MISSILES     = 15;
const unsigned int NUM_METEORS      = 8;
const unsigned int COOLDOWN_TIMER   = 100;

// TYPE DEFINITION
typedef void (*TaskFunction)(void);

/*
 *	GAME VARIABLES
 */

// Variable to store current PS2 code
unsigned int scancode;

// Bool variables to determine ship motion
bool left, right, up, down, shootL, shootR, pause;

// Ship Variables
int shipX, shipY;
int ship_index;
int thruster_index;

// Background sprite index
int bg_index;

// Meteor object variables
int meteors[NUM_METEORS][2];
int meteor_timer_start[NUM_METEORS];
int meteor_timer_elapsed[NUM_METEORS];

int meteor_index;

// Explosion animation variables
int explosion_enable[NUM_METEORS];
int explosions[NUM_METEORS][2];
int explosion_timer[NUM_METEORS];

// Earth rotation sprite index
int earth_index;

// Missile object variables
int missiles[NUM_MISSILES][2];
int missile_enable[NUM_MISSILES];
int missile_count;
int cooldown;

// Game variables
int health, score;

/*
 *	INITIALISATION FUNCTIONS
 */

// initialise hardware and set a seed for random generator
void init(void);

// set the initial values for the game objects and game variables
void reset(void);

/*
 *	SHIP CONTROL
 */

// draw the ship sprite and thruster sprites
void draw_ship(void);

// update ship coordinates based on PS2 control bools
void move_ship(void);

/*
 *	MISSILE CONTROL
 */

// fire a missile based on cooldown timer and PS2 control
void shoot_missile(void);

// update missile sprite positions
void move_missiles(void);

/*
 *	METEOR CONTROL
 */

// update meteor sprite positions
void move_meteors(void);

/*
 *	PS2 CONTROLLER
 */

// Wrapper to decode PS2 scancodes into 6 player control signals
void PS2_input(void);

/*
 *	GAME FUNCTIONS
 */

// Pause screen, trigger on key 4 press
void pause_screen(void);

// Intro sequence
void intro(void);

// Main game function
void defend_earth(void);

// Game over sequence
void gameover(void);

// Collision detection function
void collision(void);

// Animation increment function
void animation(void);

// function to display score on screen
void display_score(void);

// function to display healthbar on screen
void display_health(void);

#endif /* DEFEND_EARTH_H_ */
