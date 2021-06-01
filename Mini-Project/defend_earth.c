/*
 * defend_earth.c
 *
 *  Created on: May 31, 2021
 *      Author: James
 */

#include "defend_earth.h"

// Function to initialise hardware and set random seed
void init()
{
	// use the current time as a seed for the random number generator
	srand(time(NULL));

	// set the interrupt flag on the PS2 port
	PS2_setInterrupt(1);

	// initilaise the A9 private timer
	Timer_initialise(0xFFFEC600);
	Timer_setLoad(0xFFFFFFFF);
	Timer_setControl(SCALER, 0, 1, 1);

	// clear the VGA pixel colour output
	VGA_clearDisplay();

	// select a random background sprite from the 9 available
	bg_index = rand() % 9;
}

// Function to reset game variables
void reset()
{
	int i;

	// ensure all missiles are offscreen and disabled
	for (i = 0; i < NUM_MISSILES; i++){
		missiles[i][0] 		= 10000;
		missiles[i][1] 		= 10000;
		missile_enable[i] 	= 0;
	}

	// set all meteor positions to a random location above the screen display area
	// create a timer for each to allow them to move at different speeds.
	for (i = 0; i < NUM_METEORS; i++){
		meteors[i][0] 			= rand() % (320 - METEOR_SIZE);
		meteors[i][1] 			= -1 * (rand() % 1000) - METEOR_SIZE;
		meteor_timer_start[i] 	= rand() % 5;
		meteor_timer_elapsed[i] = meteor_timer_start[i];

		// ensure all explosion sprites are offscreen and disabled
		explosions[i][0] 	= 10000;
		explosions[i][1] 	= 10000;
		explosion_enable[i] = 0;
	}

	// set the ship position to the centre of the screen
	shipX = (SCREEN_WIDTH - SHIP_WIDTH)/2, shipY =  (SCREEN_HEIGHT - SHIP_HEIGHT)/2;

	// set the score to zero and health to 10
	score = 0; health = 10;

	// display the score and health on the LEDs and 7segs
	*LED_ptr = ~((signed int) -1 << health);
	DE1SoC_SevenSeg_SetSixDec(score);

	// set all sprite indices to starting values
	ship_index = 2, meteor_index = 0, thruster_index = 0, earth_index = 0;
}

// Function to draw the current ship position and the animated thrusters
void draw_ship()
{
	VGA_drawSprite(background[bg_index], ship[ship_index], shipX, shipY, SHIP_WIDTH, SHIP_HEIGHT);
	VGA_drawBackground(background[bg_index], shipX, shipY + SHIP_HEIGHT, SHIP_WIDTH, THRUSTER_HEIGHT);

	// if moving right, draw the left thruster ontop, else draw the right thruster on top
	if (right) {
		VGA_drawSprite(background[bg_index], thruster[thruster_index], shipX+THRUSTER_OFS[ship_index]+(SHIP_WIDTH-THRUSTER_WIDTH)/2, shipY+THRUSTER_Y, THRUSTER_WIDTH, THRUSTER_HEIGHT);
		VGA_drawSprite(background[bg_index], thruster[thruster_index], shipX-THRUSTER_OFS[4-ship_index]+(SHIP_WIDTH-THRUSTER_WIDTH)/2, shipY+THRUSTER_Y, THRUSTER_WIDTH, THRUSTER_HEIGHT);
	} else {
		VGA_drawSprite(background[bg_index], thruster[thruster_index], shipX-THRUSTER_OFS[4-ship_index]+(SHIP_WIDTH-THRUSTER_WIDTH)/2, shipY+THRUSTER_Y, THRUSTER_WIDTH, THRUSTER_HEIGHT);
		VGA_drawSprite(background[bg_index], thruster[thruster_index], shipX+THRUSTER_OFS[ship_index]+(SHIP_WIDTH-THRUSTER_WIDTH)/2, shipY+THRUSTER_Y, THRUSTER_WIDTH, THRUSTER_HEIGHT);
	}
}

// Function to move the ship position based on the PS2 bool outputs
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

// Function to enable missiles if shoot bool is high from PS2 controller
void shoot_missile()
{
	if ((shootL || shootR) && cooldown == 0){
		// if shoot bools are high and the cooldown is low, enable the next missile
		if (shootR) {
			missiles[missile_count][0] = shipX + (4*SHIP_WIDTH)/5 - (MISSILE_WIDTH/2);
			missiles[missile_count][1] = shipY;
		} else if (shootL) {
			missiles[missile_count][0] = shipX + (SHIP_WIDTH)/5 - (MISSILE_WIDTH/2);
			missiles[missile_count][1] = shipY;
		}

		missile_enable[missile_count] = 1;
		missile_count = (missile_count + 1) % (NUM_MISSILES);
		
		// set the cooldown timer to prevent missile from being fired for a period
		cooldown = COOLDOWN_TIMER;
	} else if (cooldown != 0) {
		// else decrement the cooldown timer until it reaches 0
		cooldown--;
	}
}

// Function to move all enabled missiles
void move_missiles()
{
	int i;

	// decrement all enable missile Y coordinate and redraw
	for (i = 0; i < NUM_MISSILES; i++){
		if(missile_enable[i]){
			missiles[i][1] = missiles[i][1] - 1;
			VGA_drawSprite(background[bg_index], missile, missiles[i][0], missiles[i][1], MISSILE_WIDTH, MISSILE_HEIGHT);
		}
	}
}

// Function to move all meteors and draw explosions
void move_meteors()
{
	int i;

	for (i = 0; i < NUM_METEORS; i++){
		// decrement all meteor timers
		meteor_timer_elapsed[i] = meteor_timer_elapsed[i] - 1;

		// if individual meteor timer reaches 0, move it down 1 pixel
		if (meteor_timer_elapsed[i] <= 0) {
			meteor_timer_elapsed[i] = meteor_timer_start[i];
			meteors[i][1] 			= meteors[i][1] + 1;

			if((meteors[i][1] <= SCREEN_HEIGHT) && (meteors[i][1] >= -METEOR_SIZE)) {
				VGA_drawSprite(background[bg_index], meteor[(meteor_index + i)%8], meteors[i][0], meteors[i][1], METEOR_SIZE, METEOR_SIZE);
			}
		}

		// if an explosion is enabled, draw it at the specified coordinates
		if (explosion_enable[i] == 1) {
			VGA_drawSprite(background[bg_index], explosion[10-explosion_timer[i]], explosions[i][0], explosions[i][1], METEOR_SIZE, METEOR_SIZE);
		}
	}
}

// Function to update sprite animation indices
void animation()
{
	int i;

	// always increment the thruster and meteor indices
	thruster_index = (thruster_index + 1) % 6;
	meteor_index = (meteor_index + 1) % 8;

	// check all explosions, if enabled, decrement the explosion timer
	// (explosion timer used as index)
	for (i = 0; i < NUM_METEORS; i++) {
		if (explosion_enable[i] == 1) {
			explosion_timer[i] = explosion_timer[i] - 1;

			// if explosion timer reaches -1, explosion has finished, disable
			if (explosion_timer[i] == -1) {
				explosion_enable[i] = 0;
			}
		}
	}

	// update the ship sprite index based on the ship motion direction
	if ((left && ship_index > 0) || (!right && ship_index > 2)) {
		ship_index--;
	} else if ((right && ship_index < 4) || (!left && ship_index < 2)) {
		ship_index++;
	}
}

// Function to display the score on the screen and 7segs
void display_score()
{
	char str[10];

	// convert the integer score to a string
	sprintf(str, "%d", score);

	// draw the score string to the VGA char buffer at the specified screen location
	VGA_drawString("          ", 65, 5);
	VGA_drawString(str, 65, 5);

	// display the score as a BCD value on the 7 segs
	DE1SoC_SevenSeg_SetSixDec(score);
}

// Function to display the player health on the screen and LEDs
void display_health()
{
	// draw the healthbar at the bottom of the screen
	VGA_drawSquare(0x57EA, 135, SCREEN_HEIGHT - 25, health > 0 ? health*10:0, 10);

	// display current health on LEDs
	*LED_ptr = ~((signed int) -1 << health);
}

// Function to determine sprite collisions
void collision()
{
	int i, j;

	for (i = 0; i < NUM_METEORS; i++) {
		// check all meteor locations
		for (j = 0; j < NUM_MISSILES; j++) {
			// check all missile locations
			if (missile_enable[j] == 1) {
				// if missile enable and within the hitbox of a meteor
				if (missiles[j][0] + MISSILE_WIDTH >= meteors[i][0] && missiles[j][0]  <= meteors[i][0] + METEOR_SIZE) {
					if (missiles[j][1] <= meteors[i][1] + METEOR_SIZE && missiles[j][1] + MISSILE_HEIGHT >= meteors[i][1]) {

						// remove missile and meteor from screen
						VGA_drawBackground(background[bg_index], meteors[i][0], meteors[i][1], METEOR_SIZE, METEOR_SIZE);
						VGA_drawBackground(background[bg_index], missiles[j][0], missiles[j][1], MISSILE_WIDTH, MISSILE_HEIGHT);

						// increase score based on meteor speed
						score 					= score + 50*(5-meteor_timer_start[i]);

						// begin explosion timer and set explosion to meteor location
						explosion_enable[i] 	= 1;
						explosion_timer[i]		= 11;
						explosions[i][0] 		= meteors[i][0];
						explosions[i][1] 		= meteors[i][1];

						// re-randomise meteor params offscreen
						meteors[i][0]			= rand() % (320 - METEOR_SIZE);
						meteors[i][1] 			= -1 * (rand() % 1000) - METEOR_SIZE;
						meteor_timer_start[i]	= rand() % 5;

						// disable the missile
						missile_enable[j] 		= 0;

						// update score display
						display_score();
					}

					// if missiles go off screen, disable them
					if (missiles[j][1] + MISSILE_HEIGHT <= 0){
						missile_enable[j] 		= 0;
					}
				}
			}
		}

		// if meteors go off screen, re-randomise parameters and reset above screen
		if(meteors[i][1] >= SCREEN_HEIGHT){
			meteors[i][0] 			= rand() % (320 - METEOR_SIZE);
			meteors[i][1] 			= -1 * (rand() % 1000) - METEOR_SIZE;
			meteor_timer_start[i] 	= rand() % 5;

			// lose 1 health
			health 					= health - 1;
			VGA_drawBackground(background[bg_index], 135, SCREEN_HEIGHT - 25, 100, 10);
		}

		// if meteor collides with ship, re-randomise and reset meteor above screen
		if((shipX + 10<= meteors[i][0] + METEOR_SIZE && shipX + SHIP_WIDTH - 10 >= meteors[i][0]) && (shipY <= meteors[i][1] + METEOR_SIZE && shipY + SHIP_WIDTH >= meteors[i][1])){
			VGA_drawBackground(background[bg_index], meteors[i][0], meteors[i][1], METEOR_SIZE, METEOR_SIZE);
			meteors[i][0] 			= rand() % (320 - METEOR_SIZE);
			meteors[i][1] 			= -1 * (rand() % 1000) - METEOR_SIZE;
			meteor_timer_start[i] 	= rand() % 5;

			// lose 3 health
			health 					= health - 3;
			VGA_drawBackground(background[bg_index], 135, SCREEN_HEIGHT - 25, 100, 10);
		}
	}
}

// Function to read scancodes from the PS2 keyboard and toggle variables appropriately
void PS2_input()
{
	// read the current code
	scancode = PS2_readInput();

	// if left arrow key is pressed, set left to 1, if released set to 0
	if (scancode == 0xF06B) 			{ left 		= 0; }
	else if (scancode == 0xE06B) 		{ left		= 1; }

	// if right arrow key is pressed, set right to 1, if released set to 0
	if (scancode == 0xF074) 			{ right 	= 0; }
	else if (scancode == 0xE074) 		{ right		= 1; }

	// if up arrow key is pressed, set up to 1, if released set to 0
	if (scancode == 0xF075) 			{ up 		= 0; }
	else if (scancode == 0xE075) 		{ up		= 1; }

	// if down arrow key is pressed, set down to 1, if released set to 0
	if (scancode == 0xF072) 			{ down 		= 0; }
	else if (scancode == 0xE072) 		{ down		= 1; }

	// if "A" key is pressed, set shootL to 1, if released set to 0
	if (scancode == 0xF01C) 			{ shootL	= 0; }
	else if ((scancode & 0xFF) == 0x1C) { shootL 	= 1; }

	// if "D" key is pressed, set shootR to 1, if released set to 0
	if (scancode == 0xF023) 			{ shootR	= 0; }
	else if ((scancode & 0xFF) == 0x23) { shootR	= 1; }
}

// Function to pause the game
void pause_screen()
{
	// if key4 is pressed
	if (*key_ptr & 0x8) {
		*key_ptr = 0xF;
		// disable timer
		Timer_setControl(SCALER, 0, 1, 0);

		// Draw "PAUSED" string to char buffer
		VGA_drawString("PAUSED", 40, 30);

		// wait for key4 press
		while (!(*key_ptr & 0x8)) { PS2_input(); HPS_ResetWatchdog(); }

		// clear "PAUSE" string
		VGA_drawString("      ", 40, 30);

		// reset key pos edge
		*key_ptr = 0xF;

		// re-enable timer
		Timer_setControl(SCALER, 0, 1, 1);
	}
}

// Function to introduce the game with an animation sequence
void intro()
{
	unsigned int lastIncrTime [3] 		= {0};
	const unsigned int incrPeriod [3] 	= {PERIOD/10, PERIOD/20, PERIOD/4};

	int earthX = (SCREEN_WIDTH - EARTH_WIDTH)/2, earthY = SCREEN_HEIGHT - EARTH_HEIGHT/3;
	int titleX = (SCREEN_WIDTH - TITLE_WIDTH)/2, titleY = 20;

	reset();
	Timer_setLoad(0xFFFFFFFF);
	
	// draw background texture, title and "PRESS ANY KEY TO START" string
	VGA_drawBackground(background[bg_index], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	VGA_drawSprite(background[bg_index], title, titleX, titleY, TITLE_WIDTH, TITLE_HEIGHT);
	VGA_drawSprite(background[bg_index], ship[ship_index], shipX, shipY, SHIP_WIDTH, SHIP_HEIGHT);
	VGA_drawString("PRESS ANY KEY TO START", 30, 15);

	// wait for PS2 keyboard input
	while (!scancode) {
		// rotate the earth sprite
		if ((lastIncrTime[2] - Timer_readValue()) >= incrPeriod[2]) {
			earth_index = (earth_index + 1) % 8;
			VGA_drawSprite(background[bg_index], earth[earth_index], earthX, earthY, EARTH_WIDTH, EARTH_HEIGHT/3);
			lastIncrTime[2] -= incrPeriod[2];
			PS2_input();
		}
		HPS_ResetWatchdog();
	}

	// wait for key release
	while (scancode) { PS2_input(); HPS_ResetWatchdog(); }

	// reset timer variables
	memset(lastIncrTime, 0, sizeof lastIncrTime);
	VGA_drawString("                      ", 30, 15);
	Timer_setLoad(0xFFFFFFFF);

	// intro sequence, earth moves off screen bottom, title moves offscreen top
	while (earthY < SCREEN_HEIGHT) {
		if ((lastIncrTime[0] - Timer_readValue()) >= incrPeriod[0]) {
			shipY = shipY - 1;
			animation();
			draw_ship();
			lastIncrTime[0] -= incrPeriod[0];
		}

		if ((lastIncrTime[1] - Timer_readValue()) >= incrPeriod[1]) {
			earthY = earthY + 1; titleY = titleY - 1;
			VGA_drawSprite(background[bg_index], earth[earth_index], earthX, earthY, EARTH_WIDTH, EARTH_HEIGHT/3);
			VGA_drawSprite(background[bg_index], title, titleX, titleY, TITLE_WIDTH, TITLE_HEIGHT);
			lastIncrTime[1] -= incrPeriod[1];
		}

		if ((lastIncrTime[2] - Timer_readValue()) >= incrPeriod[2]) {
			earth_index = (earth_index + 1) % 8;
			lastIncrTime[2] -= incrPeriod[2];
		}
		PS2_input();
		HPS_ResetWatchdog();
	}

	Timer_setLoad(0xFFFFFFFF);
	VGA_drawBackground(background[bg_index], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

// Function to house main game processes
void defend_earth()
{
	unsigned int lastIncrTime[NUM_TASKS] 		= {0};
	const unsigned int incrPeriod[NUM_TASKS] 	= {PERIOD/150, PERIOD/500, PERIOD/500, PERIOD/100, PERIOD/10};
	TaskFunction taskFunctions[NUM_TASKS] 		= {&move_ship, &shoot_missile, &move_missiles, &move_meteors, &animation};
	int i;

	VGA_drawString("HEALTH:", 25, 55);
	display_health();
	display_score();

	while (health > 0) {
		// read PS2 controller input
		PS2_input();

		// check pause screen transition
		pause_screen();
		
		// check sprite collisions
		collision();

		// display healthbar
		display_health();
		
		// game function task scheduler
		for (i = 0; i < NUM_TASKS; i++) {
			if ((lastIncrTime[i] - Timer_readValue()) >= incrPeriod[i]) {
				taskFunctions[i]();
				lastIncrTime[i] -= incrPeriod[i];
			}
		}
		HPS_ResetWatchdog(); // reset the watchdog.
	}

	// clear score display
	VGA_drawString("       ", 25, 55);
	VGA_drawString("       ", 65, 5);
}

// Function to display the gameover screen with an animation sequence
void gameover()
{
	int i;
	unsigned int lastIncrTime [5] 		= {0};
	const unsigned int incrPeriod [5]	= {PERIOD/10, PERIOD/50, PERIOD/4, PERIOD/50, PERIOD/150};
	unsigned int score_msg[6] 			= {51,35,47,50,37,26};

	int earthX = (SCREEN_WIDTH - EARTH_WIDTH)/2, earthY = SCREEN_HEIGHT + OVER_HEIGHT + 60;
	int overX = (SCREEN_WIDTH - OVER_WIDTH)/2, overY = SCREEN_HEIGHT + 30;

	Timer_setLoad(0xFFFFFFFF);
	VGA_drawBackground(background[bg_index], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Game over sequence, earth and gameover sprites move upwards from off screen. Ship moves off screen top,
	// meteors continue off screen bottom.
	while (overY > 20) {
		if ((lastIncrTime[0] - Timer_readValue()) >= incrPeriod[0]) {
			animation();
			lastIncrTime[0] -= incrPeriod[0];
		}

		if ((lastIncrTime[1] - Timer_readValue()) >= incrPeriod[1]) {
			earthY = earthY - 1; overY = overY - 1;
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
		HPS_ResetWatchdog();
	}

	// reset timer variables
	memset(lastIncrTime, 0, sizeof lastIncrTime);
	Timer_setLoad(0xFFFFFFFF);

	// display final game score using bitmapped font
	VGA_drawMultiChar(score_msg, 6, 0xFFFF, (SCREEN_WIDTH - 70)/2, 20 + OVER_HEIGHT + 10, 5, 8, 1);
	VGA_drawDec(score, 0xFFFF, (SCREEN_WIDTH - 70)/2 + 40, 20 + OVER_HEIGHT + 10, 5, 8, 1);

	// wait for PS2 keypress
	while (!scancode) {
		if ((lastIncrTime[2] - Timer_readValue()) >= incrPeriod[2]) {
			earth_index = (earth_index + 1) % 8;
			VGA_drawSprite(background[bg_index], destroyed_earth[earth_index], earthX, earthY, EARTH_WIDTH, EARTH_HEIGHT);
			lastIncrTime[2] -= incrPeriod[2];
			PS2_input();
		}
		HPS_ResetWatchdog();
	}

	// poll PS2 for key release
	while (scancode) { PS2_input(); HPS_ResetWatchdog(); }
	Timer_setLoad(0xFFFFFFFF);
}
