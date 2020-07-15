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

typedef enum {
	INVADERS_WON, PLAYER_WON, RESET_PRESSED
} end_game_reason_t;

extern invaders_t invaders;
extern bunker_t bunker;
extern game_wrapper_t game_wrapper;
extern mothership_t mothership;
extern player_t player;

/**
 * @brief Handler which receives and handles UDP messages and their information
 * via the NextKeyQueue.
 *
 * @param read_size: size of chars to read in
 * @param buffer: Pointer to variable that holds the incoming message
 * @param args: args passed to the function upon execution.
 *
 */
void UDPHandler(size_t read_size, char *buffer, void *args);

/**
 * @brief Task which receives, handles and sends UDP messages to communicate to
 * the binary.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vUDPControlTask(void *pvParameters);

/**
 * @brief Receives handled UDP messages from binary and defines the movement of
 * the mothership according
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
 * @brief Increments level variable in game_wrapper. If level is greater than
 * zero, a true set_level_flag indicates a manual level selection. Gets called
 * in Cheat Mode.
 */
void increment_level();

/**
 * @brief Decrements level variable in game_wrapper. If level is greater than
 * zero, a true set_level_flag indicates a manual level selection. Gets called
 * in Cheat Mode.
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
 * @brief Initialize global game_wrapper struct, when entering Game. According
 * to different flags, different variables got initialized.
 *
 * @param speed: defines speed of invaders for vInitInvaders(double speed).
 */
void vInitGameWrapper(double *speed);

/**
 * @brief Calls every Init functions, when entering Game. Resumes the
 * intermediate pop up page and suspends itself afterwards.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vInitGameTaks(void *pvParameters);

/**
 * @brief Moves Alien bullet if alive, in respect to tick and speed. Detect
 * and kill bullet if reached lower end of Screen.
 *
 * @param bullet: element which should get moved
 * @param speed: scale of movement velocity
 */
void vMoveAlienBullet(bullet_t *bullet, short speed);

/**
 * @brief Unleash a bullet from a random alien of the back row, when
 * receiving a shooting signal from AlienShootsQueue.
 */
void vAlienShoot();

/**
 * @brief Move Player bullet if alive, in respect to tick and speed.
 * Detect and kill bullet if reached upper end of Screen.
 *
 * @param bullet: element which should get moved
 * @param speed: scale of movement velocity
 */
void vMovePlayerBullet(bullet_t *bullet, short speed);

/**
 * @brief Move player on x Axis and shoot according to PlayerQueue input.
 * If AI_control_ON is true, current bullet mode
 * and player position gets sent to UDPControl Task (eventually to binary).
 *
 * @param moving_left: indicates the direction of the movement as left
 * @param moving_right: indicates the direction of the movement as right
 * @param AI_control_ON: indicates an active TwoPlayerMode and mothership
 */
void vMovePlayer(unsigned char *moving_left, unsigned char *moving_right,
		unsigned char AI_control_ON);

/**
 * @brief Move invaders continuously in respect to invaders.speed. Handle
 * back and forth movement across the screen, descending towards player.
 * Check if invaders reached bottom line.
 *
 * @param invaders_won: Pointer to variable which indicates, that invaders_won
 * @param last_time: Pointer to last time function got called, to compute
 * movement
 */
void vMoveInvaders(unsigned char *invaders_won, TickType_t *last_time);

/**
 * @brief Define AI_control_ON variable. Move mothership if alive in respect
 * to tick and MOTHERSHIP_SPEED. In One Playermode only continuous movement
 * from left to right is implemented. Check and Kill mothership if
 * it reaches the right hand Screen border. In Two Playermode mothership
 * moves controled by the binary. Check and let mothership reappear on
 * opposing site, if it moves out of Screen borders. If mothership is not
 * alive it got revived after WAITING_TIME_FOR_MOTHERSHIP in ms.
 *
 * @param last_time_mothership: Pointer to variable which stores time since
 * mothership is dead.
 * @param AI_control_ON: Pointer to variable which indicates TwoPlayerMode
 * active and mothership alive.
 */
void vMoveMothership(TickType_t *last_time_mothership,
		unsigned char *AI_control_ON);

/**
 * @brief Set Player bullet alive to false.
 */
void vKillPlayerBullet(void);

/**
 * @brief Set Invaders bullet alive to false.
 */
void vKillInvadersBullet(void);

/**
 * @brief Kill Alien on position x,y. Adjust the front array, with checking
 * each column for Aliens which are alive.
 * If column of current x is empty (front of x less than zero) adjust left
 * and right border of invaders block.
 * This is needed, to get the invaders move across the whole screen.
 * Get the maximum front to define where Invader Block ends on the bottom
 * side, to indicate if invaders block reaches bunkers already.
 * According to number of aliens already killed increase speed of invaders
 * movement. Check number of aliens already killed to recognize when
 * invaders got eliminated. Increase Score according to type of Alien
 * got killed. Also increment separate counting system for getting extra
 * lifes back. Check for Extra Lives and Kill the Player bullet afterwards.
 *
 * @param y: y coordinate of alien in invaders grid
 * @param x: x coordinate of alien in invaders grid
 * @param player_won: Pointer to variable which indicates that invaders
 * got eliminated.
 */
void vKillAlien(short y, short x, unsigned char *player_won);

/**
 * @brief Increment score and kill Player Bullet. If TwoPlayerMode the bullet
 * status PASSIVE got sent once, before killing mothership and therefore
 * stop sending any player updates to binary.
 *
 * @param last_time_mothership: Pointer to variable that holds the last
 * time the mothership was alive.
 * @param AI_control_ON: Pointer to variable that indicates TwoPlayerMode
 * and an active mothership.
 */
void vKillMothership(TickType_t *last_time_mothership,
		unsigned char *AI_control_ON);

/**
 * @brief Check and decrement destruction state of single block in bunker,
 * which got hit by the Player bullet.
 * It takes three hits from a Player bullet to destroy a block. If destroyed,
 * player_front gets updated
 * on this x value. Finally kill the player bullet.
 *
 * @param bunkerNumber: Number of bunker, which got hit.
 * @param player_front: indicates the number of destroyed blocks
 * (from player side) at each x value of bunker.
 * @param xBlock: x value of block in bunker grid, that got hit from
 * player bullet.
 */
void vDestructBunkerBlockPlayerSide(short bunkerNumber, short player_front,
		short xBlock);

/**
 * @brief Check and decrement destruction state of single block in bunker,
 * which got hit by the Invaders bullet.
 * It takes one hit from a Invader bullet to destroy a block. If destroyed,
 * aliens_front gets updated
 * on this x value. Finally kill the invaders bullet.
 *
 * @param bunkerNumber: Number of bunker, which got hit.
 * @param aliens_front: indicates the number of destroyed blocks
 * (from aliens side) at each x value of bunker.
 * @param xBlock: x value of block in bunker grid, that got hit from
 * player bullet.
 */
void vDestructBunkerBlockInvadersSide(short bunkerNumber, short aliens_front,
		short xBlock);

/**
 * @brief Handles a hit between invaders bullet and player. Player looses
 * one life. Check if any life remains. If not set invaders_won to true.
 *
 * @param invaders_won: Pointer to variable that indicates that the invaders won.
 */
void vPlayerGotHit(unsigned char *invaders_won);

/**
 * @brief Checks if invaders and player bullet collide. If so, both bullets
 * got killed.
 *
 * @param invaders_bullet_pos_x: current invaders bullet x position
 * @param invaders_bullet_pos_y: current invaders bullet y position
 * @param player_bullet_pos_x: current player bullet x position
 * @param player_bullet_pos_y: current player bullet y position
 */
void vCheckBulletBulletCollision(short invaders_bullet_pos_x,
		short invaders_bullet_pos_y, short player_bullet_pos_x,
		short player_bullet_pos_y);

/**
 * @brief Checks if Player got hit by invaders bullet. If so, vPlayerGotHit
 * gets called.
 *
 * @param invaders_bullet_pos_x: current invaders bullet x position
 * @param invaders_bullet_pos_y: current invaders bullet y position
 * @param player_pos_x: current player x position
 * @param player_pos_y: current player y position
 * @param invaders_won: Pointer to variable that indicates that the invaders won.
 */
void vCheckIfPlayerGotHit(short invaders_bullet_pos_x,
		short invaders_bullet_pos_y, short player_pos_x, short player_pos_y,
		unsigned char *invaders_won);

/**
 * @brief Checks if bunker got hit by an invader bullet. If so, the destruct
 * bunker block method is called.
 *
 * @param invaders_bullet_pos_x: current invaders bullet x position
 * @param invaders_bullet_pos_y: current invaders bullet y position
 * @param player_pos_x: current player x position
 * @param player_pos_y: current player y position
 */
void vCheckIfBunkerGotHitInvaders(short invaders_bullet_pos_x,
		short invaders_bullet_pos_y, short player_pos_x, short player_pos_y);

/**
 * @brief Overall function that checks if the alien bullet collide with
 * bunker, player or player bullet. Fetches at first the current informations
 * from invader and player. Afterwards it calls every different collision
 * detection function.
 *
 * @param invaders_won: Pointer to variable that indicates that the invaders won.
 */
void vCheckAliensBulletCollision(unsigned char *invaders_won);

/**
 * @brief Check if Invaders got hit by player bullet. At first it checks if
 * the player bullet already reached the invader block itself and did not
 * pass it. Then for every x column it got checked if the player bullet is
 * a thread to it. If a column is in the same x range as the player bullet,
 * the next step is to check which y Alien is affected. This Alien is going
 * to be killed.
 *
 * @param invaders_pos_x: current invaders x position
 * @param invaders_pos_y: current invaders  y position
 * @param player_bullet_pos_x: current player bullet x position
 * @param player_bullet_pos_y: current player bullet y position
 * @param player_won: Pointer to variable that indicates that invaders got
 * eliminated
 * @param invaders_front: An array that indicates for every x the y of the
 * lowest alien.
 */
void vCheckIfInvadersGotHit(short invaders_pos_x, short invaders_pos_y,
		short player_bullet_pos_x, short player_bullet_pos_y,
		unsigned char *player_won, short *invaders_front);

/**
 * @brief Checks if bunker got hit by a player bullet. If so, the destruct
 * bunker block method is called.
 *
 * @param player_bullet_pos_x: current player bullet x position
 * @param player_bullet_pos_y: current player bullet y position
 */
void vCheckIfBunkerGotHitPlayer(short player_bullet_pos_x,
		short player_bullet_pos_y);

/**
 * @brief Check if Mothership got hit by a player bullet. If so,
 * vKillMothership is called.
 *
 * @param player_bullet_pos_x: current player bullet x position
 * @param player_bullet_pos_y: current player bullet y position
 * @param last_time_mothership: Pointer to variable that indicates the last
 * time the mothership was alive
 * @param AI_control_ON: Pointer to variable that indicates an active
 * TwoPlayerMode and an active mothership
 */
void vCheckIfMothershipGotHit(short player_bullet_pos_x,
		short player_bullet_pos_y, TickType_t *last_time_mothership,
		unsigned char *AI_control_ON);

/**
 * @brief Overall function that checks if the player bullet collides
 * with bunker, invaders or invaders bullet. Fetches at first the current
 * informations from invader and player. Afterwards it calls every
 * different collision detection function.
 *
 * @param player_won: Pointer that indicates that the invaders got eliminated.
 * @param last_time_mothership: Pointer to variable that indicates the last
 * time the mothership was alive
 * @param AI_control_ON: Pointer to variable that indicates an active
 * TwoPlayerMode and an active mothership
 */
void vCheckPlayerBulletCollision(unsigned char *player_won,
		TickType_t *last_time_mothership, unsigned char *AI_control_ON);

/**
 * @brief When getting in Pause mode. Time is running anyways. Therefore the
 * timebased movement of the invaders needs a new Last_time when the Pause
 * mode is exited (Resume got pressed). If not the invaders run out of the
 * screen boundaries. In order to restart the periodic time when to wait for
 * the mothership coming back, last_time_mothership gets also reseted.
 *
 * @param last_time: Pointer to variable that holds the last time the
 * vMoveInvaders function got called
 * @param last_time_mothership: Pointer to variable that holds the last time
 * the mothership was alive
 */
void vSetLastTimeAfterResume(TickType_t *last_time,
		TickType_t *last_time_mothership);

/**
 * @brief Completes the exit tasks that must be completed when leaving a game
 * for whatever reason. If Invaders won, level gets reseted, it gets checked
 * if the current score is a new highscore and the intermediate pop up
 * page gets initialized.
 * Otherwise when the Player won, only the next_level flag is set to true, in
 * order to control the init functions. If a pressed reset button was the reason
 * to end the match, score gets compared to highscore, but no pop up page
 * will show up.
 *
 * @param reason: Indicates why the match ended and therefore defines the
 * following steps
 */
void vEndMatch(end_game_reason_t reason);

/**
 * @brief Function to check if the Limit score for an extra life got reached.
 * If so, an extra life is applied and the get_extra_life_score gets reset.
 * The number of life's never extends three.
 */
void vCheckForExtraLife();

/**
 * @brief Task to handle every update of the Space Invader Game logic. Gets
 * refreshed every 20ms. If Players won a change to the fourth (Init) state
 * occurs, to enter the next level.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vGameHandlerTask(void *pvParameters);

/**
 * @brief initialize Semaphores, Queues and Tasks defined on this file.
 *
 * @returns 0 if successful and -1 if error occurred.
 */
int init_space_invaders_handler(void);

#endif
