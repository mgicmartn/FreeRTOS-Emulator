/**
 * @file space_invaders_handler.h
 * @author  Martin Zimmermann
 * @date 15 July 2020
 * @brief This file combines all functions, which handle the logic of the
 * Game Space Invaders.
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 *
 */



#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__


#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)

#define STATE_ONE 0
#define STATE_TWO 1
#define STATE_THREE 2
#define STATE_FOUR 3
#define STATE_FIVE 4

#define MOVE_RIGHT 2
#define MOVE_LEFT 1
#define STOP 0
#define SHOOT 3

#define UDP_BUFFER_SIZE 1024
#define UDP_RECEIVE_PORT 1234
#define UDP_TRANSMIT_PORT 1235

#define BUTTON_DEBOUNCE_DELAY 50

// Draw definitions
#define LOBBY_BUTTON_WIDTH 400
#define LOBBY_BUTTON_HEIGHT 60
#define SHAPES_SIZE 60
#define POP_UP_PAGE_WIDTH 400
#define POP_UP_PAGE_HEIGHT 200

// Player definitions
#define PLAYER_SIZE_X 30
#define PLAYER_SIZE_Y 10
#define PLAYER_INIT_X (SCREEN_WIDTH/2)
#define PLAYER_INIT_Y (SCREEN_HEIGHT*5/6)
#define PLAYER_SPEED 4
#define BULLET_SIZE_X 2
#define BULLET_SIZE_Y 10
#define BULLET_SPEED 18

#define LIFE_SIZE_X 20
#define LIFE_SIZE_Y 20
#define LIFE_SIZE_DISTANCE (LIFE_SIZE_X + 10)

// Invaders definitions
#define ALIEN_SHOOT_DELAY 500
#define ALIEN_SIZE_X 20
#define ALIEN_SIZE_Y 20
#define ALIEN_DISTANCE (SCREEN_WIDTH/24)
#define ALIEN_TYPE_1_SCORE_REWARD 30
#define ALIEN_TYPE_2_SCORE_REWARD 20
#define ALIEN_TYPE_3_SCORE_REWARD 10
#define ALIEN_BULLET_SPEED 6
#define NUMBER_OF_ALIENS_X 10
#define NUMBER_OF_ALIENS_Y 5

#define MOTHERSHIP_SIZE_X 30
#define MOTHERSHIP_SIZE_Y 20
#define MOTHERSHIP_POS_Y (SCREEN_HEIGHT/6 - MOTHERSHIP_SIZE_Y/2)
#define MOTHERSHIP_SPEED 1
#define WAITING_TIME_FOR_MOTHERSHIP 10000

#define INVADERS_STRECH_X (SCREEN_WIDTH/3)
#define INVADERS_STEP_IN_Y (SCREEN_HEIGHT/50)
#define INVADERS_SIZE_X (ALIEN_DISTANCE * (NUMBER_OF_ALIENS_X - 1) + ALIEN_SIZE_X)
#define INVADERS_SIZE_Y (ALIEN_DISTANCE * (NUMBER_OF_ALIENS_Y - 1) + ALIEN_SIZE_Y)
#define INVADERS_INIT_POS_X ((SCREEN_WIDTH -  INVADERS_SIZE_X - INVADERS_STRECH_X) /2 )
#define INVADERS_INIT_POS_Y (SCREEN_HEIGHT/5)
#define INVADERS_MAX_SPEED (0.120)

#define LEFT_BORDER (INVADERS_INIT_POS_X)
#define RIGHT_BORDER (SCREEN_WIDTH - LEFT_BORDER)
#define BOTTOM_BORDER (PLAYER_INIT_Y - INVADERS_SIZE_Y)

#define BUNKER_BLOCK_SIZE_X 6
#define BUNKER_BLOCK_SIZE_Y 6
#define NUM_BUNKER_BLOCK_X 6
#define NUM_BUNKER_BLOCK_Y 4
#define BUNKER_POS_Y (SCREEN_HEIGHT/8 * 6)

#define SECS_TO_WAIT 3

#define SET_SCORE_100 1
#define SET_SCORE_1000 2
#define SET_SCORE_10000 3
#define MOTHERSHIP_SCORE_REWARD 50
#define NEW_LEVEL_GET_EXTRA_LIFE_SCORE_REWARD 20
#define NEXT_LEVEL_SPEED_CLIMAX 0.01


#define STATE_QUEUE_LENGTH 1
#define PLAYER_QUEUE_LENGTH 1
#define BUTTON_COUNT_QUEUE_LENGTH 1
#define SWAP_INVADERS_QUEUE_LENGTH 1
#define ALIENS_SHOOTS_QUEUE_LENGTH 1
#define PRINT_QUEUE_LENGTH 10

#define STARTING_STATE STATE_ONE
#define STATE_DEBOUNCE_DELAY 300

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR
#define PRINT_TASK_ERROR(task) PRINT_ERROR("Failed to print task ##task");


#ifdef TRACE_FUNCTIONS
#include "tracer.h"
#endif

enum color_t {red, green, purple};
enum direction_t {left, right, stationary};

// button handling
typedef struct buttons_buffer {
    unsigned char buttons[SDL_NUM_SCANCODES];
    SemaphoreHandle_t lock;
} buttons_buffer_t;

// state handling
typedef struct state {
	unsigned char currState;
	SemaphoreHandle_t lock;
} state_t;

// player handling
typedef struct bullet_type {
	short pos_x;
	short pos_y;
	unsigned char alive;
} bullet_t;

typedef struct player_type {
	short pos_x;
	short pos_y;
	bullet_t bullet;
    SemaphoreHandle_t lock;
} player_t;

// invaders handling
typedef struct enemy_type {
	enum color_t color;
	unsigned char alive;
	short points;
} enemy_t;

typedef struct invaders_type {
	enemy_t enemy[NUMBER_OF_ALIENS_Y][NUMBER_OF_ALIENS_X];
	short front[NUMBER_OF_ALIENS_X];
	short last_column_right;
	short last_column_left;
	short maxFront;
	short pos_x;
	double float_pos_x;
	short pos_y;
	enum direction_t direction;
	unsigned char resume;
	unsigned char paused;
	unsigned char killed;
	double speed;
	bullet_t bullet;
	SemaphoreHandle_t lock;

} invaders_t;

// bunkers handling
typedef struct single_bunker_type {
	unsigned char block_destruction_state[NUM_BUNKER_BLOCK_Y][NUM_BUNKER_BLOCK_X];
	short pos_x;
	short player_front[NUM_BUNKER_BLOCK_X];
	short aliens_front[NUM_BUNKER_BLOCK_X];
} single_bunker_t;

typedef struct bunker_type {
	short pos_y;
	single_bunker_t bunkers[4];
	SemaphoreHandle_t lock;
} bunker_t;


// game_wrapper handling
typedef struct game_wrapper_type {
	short score;
	short remaining_life;
	double speed;
	short level;
	unsigned char set_level_flag;
	unsigned char infinite_life_flag;
	unsigned char set_score_flag;
	unsigned char next_level_flag;
	short highscore;
	short get_extra_life_scores;
	char game_message[100];
	unsigned char next_state;
	SemaphoreHandle_t lock;
} game_wrapper_t;


// mothership handling
typedef struct mothership_type{
	unsigned char alive;
	short pos_x;
	unsigned int inc;
	unsigned int dec;
	unsigned char stop;
	unsigned char AI_control;
	SemaphoreHandle_t lock;
} mothership_t;






#endif
