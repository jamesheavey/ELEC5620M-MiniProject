/*
 * main.c
 *
 *  Created on: May 26, 2021
 *      Author: James
 * 
 * DESCRIPTION:
 * This code implements a functional game "DEFEND EARTH", operating on the
 * DE1-SoC development board. The A9 private timer is used to measure
 * elapsed time and trigger game functions that control both the player and game
 * objects. Game is displayed on VGA and played with a PS2 keyboard. Additional
 * features displayed on 7 segment displays and LEDs.
 * 
 */

int main()
{
	// initialise hardware
	init();

	// enter main game loop
	while (1)
	{
		intro();
		defend_earth();
		gameover();
	}
}
