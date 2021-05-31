/*
 * test_sprite.h
 *
 *  Created on: May 26, 2021
 *      Author: James
 */

#ifndef SPRITE_H_
#define SPRITE_H_

extern const unsigned short background[9][76800];

extern const unsigned short ship[5][1980];

const int SHIP_WIDTH 		= 45;
const int SHIP_HEIGHT 		= 44;

extern const unsigned short thruster[6][182];

const int THRUSTER_WIDTH	= 13;
const int THRUSTER_HEIGHT	= 14;

const int THRUSTER_Y 		= 44;
const int THRUSTER_OFS[5]	= {3,5,7,6,6};

extern const unsigned short laser[105];

const int LASER_WIDTH 		= 7;
const int LASER_HEIGHT 		= 15;

extern const unsigned short missile[360];

const int MISSILE_WIDTH 	= 12;
const int MISSILE_HEIGHT 	= 30;

extern const unsigned short meteor[8][625];

const int METEOR_SIZE 		= 25;

extern const unsigned short title[4862];

const int TITLE_WIDTH 		= 187;
const int TITLE_HEIGHT 		= 26;

extern const unsigned short game_over[4056] ;

const int OVER_WIDTH 		= 156;
const int OVER_HEIGHT 		= 26;

extern const unsigned short earth[8][40000];
extern const unsigned short destroyed_earth[8][40000];

const int EARTH_WIDTH		= 200;
const int EARTH_HEIGHT		= 200;

extern const unsigned short explosion[11][625];

const int EXPLOSION_SIZE 	= 25;

#endif /* SPRITE_H_ */
