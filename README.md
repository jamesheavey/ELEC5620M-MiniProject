# ELEC5620M Mini-Project Repository

This repository contains source and header C files to that define the function of simple game called "DEFEND EARTH!". The code was designed for and implemented on the DE1-SoC board. This code requires a number of drivers for each of the utilised hardware components, including: the A9 private timer, the 6 available Seven Segment displays, PS2 decoding and the VGA. This drivers can be observed in my personal [ELEC5620 driver repository](https://github.com/leeds-embedded-systems/ELEC5620M-Student-jamesheavey). 

A video demonstration of the implemented code can be seen using the link provided: [VIDEO DEMO]()

## Game Functionality
The game designed during this project is a topdown shooter where the player must control the ship and destroy meteors as they fall vertically from off screen. The game utilises a PS2 keyboard for player control and displays all visuals via the VGA port. Upon starting the game, an introduction screen appears. The game will remain on the introduction screen until any key on the keyboard is pressed. When pressed the game transitions to the main game loop. Here player movement is controlled by keybaord arrow keys and the firing mechanism is controlled by the "A" and "D" keys. The game may be paused at anytime by pressing button 4 on the DE1SoC board. When all lives are lost, the game ends and a game over screen is displayed, indicating the total score accumulated during the session.

#### KEYBOARD

* **ARROW LEFT:** Move player ship left. 
* **ARROW RIGHT:** Move player ship right. 
* **ARROW UP:** Move player ship up. 
* **ARROW DOWN:** Move player ship down. 
* **"A":** Fire left missile. 
* **"D":** Fire right missile. 

#### BUTTON

* **BUTTON 4:** Toggle pause state while in game. 

This encompasses all the user requirements for a suitable digital stopwatch.

## File List

| FILE NAME | PURPOSE |
| --- | --- |
| `main.c` | Main source file containing main function. Initialises hardware and houses game loop. |
| `defend_earth.c` | File defining all functions related to the operation of the game. |
| `defend_earth.h` | File defining all headers and global variables for the defend_earth source file. |

## Function List
Each of the functions designed for this project are listed below, along with their inputs/outouts and purpose within the system.

### defend_earth.c
| FUNCTION NAME | PURPOSE |
| --- | --- |
| `init` |  Initialise hardware and set a seed for random generator. |
| `reset` | Set the initial values for the game objects and game variables. |
| `draw_ship` | Draw the current ship position and the animated thrusters. |
| `move_ship` | Update ship coordinates based on PS2 control bools. |
| `shoot_missile` | Fire a missile based on cooldown timer and PS2 control. |
| `move_missiles` | Update missile sprite positions. |
| `move_meteors` | Update meteor sprite positions. |
| `PS2_input` | Wrapper to decode PS2 scancodes into 6 player control signals. |
| `pause_screen` | Pause screen, trigger on key 4 press. |
| `intro` | Introduce the game with an animation sequence. |
| `defend_earth` | Houses main game processes. |
| `gameover` | Display the gameover screen with an animation sequence. |
| `collision` | Determine sprite collisions. |
| `animation` | Update sprite animation indices. |
| `display_score` | Display the score on the screen and 7 segments. |
| `display_health` | Display the player health on the screen and LEDs. |

---

#### By James Heavey

#### SID: 201198933

#### University of Leeds, Department of Electrical Engineering

