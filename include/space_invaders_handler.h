
/**
 * @file space_invaders_handler.h
 * @author  Martin Zimmermann
 * @date 15 July 2020
 * @brief This file combines all functions, which handle the logic of the Game Space Invaders.
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


#ifndef __SPACE_INVADERS_HANDLER_H__
#define __SPACE_INVADERS_HANDLER_H__

#include "constants.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "main.h"

extern QueueHandle_t DifficultyQueue;

extern TaskHandle_t InitGameTaks;
extern TaskHandle_t GameHandlerTask;
extern TaskHandle_t UDPControlTask;

typedef enum { INVADERS_WON, PLAYER_WON, RESET_PRESSED } end_game_reason_t;


extern invaders_t invaders;
extern bunker_t bunker;
extern game_wrapper_t game_wrapper;
extern mothership_t mothership;
extern player_t player;

/**
 * @brief Handler which receives and handles UDP messages and distribute them via NextKeyQueue.
 *
 * @param
 *
 */
void UDPHandler(size_t read_size, char *buffer, void *args);

void vUDPControlTask(void *pvParameters);


/**
 * @brief Receives handled UDP messages from binary and defines the movement of the mothership according
 * to the incoming information.
 *
 * @return 0 if successful execution
 */
unsigned char xCheckMothershipUDPInput();


/**
 * @brief Initialize set_score_flag in game_wrapper. Gets called in Cheat Mode.
 *
 * @param score_value_flag: Value to which the flag should be set.
 */
void vSetScoreFlag(unsigned char score_value_flag);

/**
 * @brief Increments level variable in game_wrapper. If level is greater than zero, a true
 * set_level_flag indicates a manual level selection. Gets called in Cheat Mode.
 */
void increment_level();

/**
 * @brief Decrements level variable in game_wrapper. If level is greater than zero, a true
 * set_level_flag indicates a manual level selection. Gets called in Cheat Mode.
 */
void decrement_level();

/**
 * @brief Initializes global invaders struct, when entering Game.
 *
 * @param speed: Velocity of Invaders movement.
 */
void vInitInvaders(double speed);

/**
 * @brief Initializes global player struct, when entering Game.
 */
void vInitPlayer(void);

/**
 * @brief Initializes global mothership struct, when entering Game.
 */
void vInitMothership(void);

/**
 * @brief Initializes global bunker struct, when entering Game.
 */
void vInitBunker(void);

/**
 * @brief Initialize global game_wrapper struct, when entering Game. According to different flags, different
 * variables got initialized.
 *
 * @param speed: defines speed of invaders for vInitInvaders(double speed).
 */
void vInitGameWrapper(double* speed);

/**
 * @brief Call all init functions, when entering Game.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vInitGameTaks(void *pvParameters);

/**
 * @brief Moves Alien bullet if alive, in respect to tick and speed. Detect and kill bullet if
 * reached lower end of Screen.
 *
 * @param bullet: element which should get moved
 * @param speed: scale of movement velocity
 */
void vMoveAlienBullet(bullet_t* bullet, short speed);

/**
 * @brief Unleash a bullet from a random alien of the back row, when receiving a shooting signal
 * from AlienShootsQueue.
 */
void vAlienShoot();

/**
 * @brief Move Player bullet if alive, in respect to tick and speed. Detect and kill bullet if
 * reached upper end of Screen.
 *
 * @param bullet: element which should get moved
 * @param speed: scale of movement velocity
 */
void vMovePlayerBullet(bullet_t* bullet, short speed);

/**
 * @brief Move player on x Axis and shoot according to PlayerQueue input. If AI_control_ON is true, current bullet mode
 * and player position gets sent to UDPControl Task (eventually to binary).
 *
 * @param moving_left: indicates the direction of the movement as left
 * @param moving_right: indicates the direction of the movement as right
 * @param AI_control_ON: indicates an active TwoPlayerMode and mothership
 */
void vMovePlayer(unsigned char* moving_left, unsigned char* moving_right, unsigned char AI_control_ON);

void vMoveInvaders(unsigned char* invaders_won, TickType_t * last_time);

void vMoveMothership(TickType_t* last_time_mothership, unsigned char* AI_control_ON);

void vKillPlayerBullet(void);

void vKillInvadersBullet(void);

void vKillAlien(short y, short x, unsigned char * player_won);

void vKillMothership(TickType_t * last_time_mothership, unsigned char* AI_control);

void vDestructBunkerBlockPlayerSide(short bunkerNumber, short player_front, short xBlock);

void vDestructBunkerBlockInvadersSide(short bunkerNumber, short aliens_front, short xBlock);

void vPlayerGotHit(unsigned char *invaders_won);

void vCheckBulletBulletCollision(short invaders_bullet_pos_x, short invaders_bullet_pos_y, short player_bullet_pos_x, short player_bullet_pos_y);

void vCheckIfPlayerGotHit(short invaders_bullet_pos_x, short invaders_bullet_pos_y, short player_pos_x, short player_pos_y, unsigned char *invaders_won);

void vCheckIfBunkerGotHitInvaders(short invaders_bullet_pos_x, short invaders_bullet_pos_y, short player_pos_x, short player_pos_y);

void vCheckAliensBulletCollision(unsigned char *invaders_won);

void vCheckIfInvadersGotHit(short invaders_pos_x, short invaders_pos_y, short player_bullet_pos_x, short player_bullet_pos_y, unsigned char * player_won, short * invaders_front);

void vCheckIfBunkerGotHitPlayer(short player_bullet_pos_x, short player_bullet_pos_y);

void vCheckIfMothershipGotHit(short player_bullet_pos_x, short player_bullet_pos_y, TickType_t * last_time_mothership, unsigned char* AI_control);

void vCheckPlayerBulletCollision(unsigned char * player_won, TickType_t * last_time_mothership, unsigned char* AI_control);

void vSetLastTimeAfterResume(TickType_t* last_time, TickType_t* last_time_mothership);

void vEndMatch(end_game_reason_t reason);

void vCheckForExtraLife();

void vGameHandlerTask(void *pvParameters);

int init_space_invaders_handler(void);



#endif
