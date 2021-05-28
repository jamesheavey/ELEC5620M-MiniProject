/*
 * test_sprite.h
 *
 *  Created on: May 26, 2021
 *      Author: James
 */

#ifndef SPRITE_H_
#define SPRITE_H_

extern const unsigned short background[4][76800];

extern const unsigned short player_ship[4][2304];

const int PLAYER_WIDTH 	= 48;
const int PLAYER_HEIGHT = 48;

extern const unsigned short laser[105];

const int LASER_WIDTH 	= 7;
const int LASER_HEIGHT = 15;

extern const unsigned short missile[360];

const int MISSILE_WIDTH 	= 12;
const int MISSILE_HEIGHT = 30;

#endif /* SPRITE_H_ */
