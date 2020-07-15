/**
 * @file space_invaders_handler.h
 * @author  Martin Zimmermann
 * @date 15 July 2020
 * @brief This file combines all functions, which are related to drawing
 * Tasks of the Game Space Invaders.
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


#ifndef __DRAW_H__
#define __DRAW_H__

#include "constants.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "main.h"

extern TaskHandle_t DrawLobbyMainTask;
extern TaskHandle_t DrawLobbyCheatTask;
extern TaskHandle_t DrawLobbyHighscoreTask;
extern TaskHandle_t DrawGameTask;

/**
 * @brief Check function to easily raise an error if draw task wasn't
 * successful.
 *
 * @param status: status of the draw task to check
 * @param msg: message which should get error printed, if not successful
 */
void checkDraw(unsigned char status, const char *msg);

/**
 * @basic Computes and draws the number of frames per seconds in this this
 * function gets called.
 */
void vDrawFPS(void);

/**
 * @brief Draws a Rectangle with text inside. It gets called as intermediate
 * pop up page between game and lobby. On the time it fetches the value for
 * the next state in which the pop up page should change afterwards.
 *
 * @param pop_up_page: Reference to rectangle
 * @param next_state: Reference to next state
 * @param countdown: seconds left until next state occurs
 */
void vDrawPopUpField(my_square_t *pop_up_page, unsigned char *next_state,
		short countdown);

/**
 * @brief Draws a Rectangle shaped button with text on it.
 *
 * @param play_button: Reference to rectangle
 */
void vDrawPlayButton(my_square_t *play_button);

/**
 * @brief Clears the screen with a background image
 *
 * @param background_image: image handle of corresponding image
 */
void vDrawBackgroundImage(image_handle_t background_image);

/**
 * @brief Draws a Rectangle shaped button with text on it.
 *
 * @param cheat_button: Reference to rectangle
 */
void vDrawCheatButton(my_square_t *cheat_button);

/**
 * @brief Draws a Rectangle shaped button with text on it.
 *
 * @param highscore_button: Reference to rectangle
 */
void vDrawHighscoreButton(my_square_t *highscore_button);

/**
 * @brief Draws a Rectangle shaped button with text on it. The color changes
 * to green, if the TwoPlayerMode is active.
 *
 * @param two_player_mode_button: Reference to rectangle
 * @param mothership_AI_control: flag which indicates the difficulty of the
 * binary.
 */
void vDrawTwoPlayerButton(my_square_t *two_player_mode_button,
		unsigned char mothership_AI_control);

/**
 * @brief Draws a Rectangle shaped button with text on it.
 *
 * @param back_button: Reference to rectangle
 */
void vDrawBackButton(my_square_t *back_button);

/**
 * @brief Draws a Rectangle shaped button with text on it. The color changes
 * to green, if the level chosen is greater than one.
 *
 * @param setlevel_button: Reference to rectangle
 * @param game_wrapper_level: variable which indicates the current level
 */
void vDrawSetLevelButton(my_square_t *setlevel_button,
		short game_wrapper_level);

/**
 * @brief Draws a Rectangle shaped button with text on it. The color changes
 * to green, if the infinitive life option gets chosen.
 *
 * @param inflife_button: Reference to rectangle
 * @param game_wrapper_infinite_life_flag: variable which indicates the
 * state of the infinite life option (ON: 1, OFF: 0)
 */
void vDrawInfiniteLifeButton(my_square_t *inflife_button,
		unsigned char game_wrapper_infinite_life_flag);

/**
 * @brief Draws a Rectangle shaped button with text on it. The color of the text
 * changes to green, if one of the given select score options gets chosen.
 *
 * @param setscore_button: Reference to rectangle
 * @param game_wrapper_set_score_flag: variable which indicates the selected
 * score option. (100, 1000, 10000)
 */
void vDrawSetScoreButton(my_square_t *setscore_button,
		unsigned char game_wrapper_set_score_flag);

/**
 * @brief Draws the highscore as text.
 *
 * @param highscore: value
 */
void vDrawHighscore(short highscore);

/**
 * @brief Task which does not get resumed from the state machine. It draws a
 * temporary, intermediate pop up page that show a Rectangle with text in it.
 * In the text is a countdown implemented, which decrements every second,
 * to indicate th seconds left till the next action gets done.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vDrawPopUpPageTask(void *pvParameters);

/**
 * @brief Drawing task in state one. Draws the lobby main page. With the buttons
 * Play, Back, Cheat Mode, Highscore and TwoPlayerMode.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vDrawLobbyMainTask(void *pvParameters);

/**
 * @brief Drawing task in state two. Draws the lobby cheat mode page.
 * With the buttons Play, Back, SetScore, SetLevel and Set Infinite Life.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vDrawLobbyCheatTask(void *pvParameters);

/**
 * @brief Drawing task in state three. Draws the lobby highscore page.
 * With one back button.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vDrawLobbyHighscoreTask(void *pvParameters);

/**
 * @brief Draws in the game the current score to the middle, upper part
 * of the screen. The value is stored in the global struct game_wrapper.
 */
void vDrawScore();

/**
 * @brief Draws in the game the strings "Pause" and "Resume" to the right hand,
 * upper corner of the screen. According to the current state of the game (paused,
 * running) one or the other string is shown.
 */
void vDrawPauseResume();

/**
 * @brief Draws in the game the string "Reset" below "Pause" and "Resume".
 */
void vDrawReset();

/**
 * @brief Draws in the game the current level the game is in, in the left hand,
 * upper corner.
 */
void vDrawLevel();

/**
 * @brief Draws with little heart images the number of the remaining lifes.
 * If image cannot get loaded a simple rectangle replaces it.
 *
 * @param life_shape: Reference to a rectangle for the case the image can
 * not be loaded.
 * @param life_image: Image handler to loaded heart image
 */
void vDrawLifes(my_square_t *life_shape, image_handle_t life_image);

/**
 * @brief Checks according to incoming messages from the SwapInvadersQueue if
 * the Aliens should swap state, change their shapes.
 *
 * @param swap_state: reference to current state
 */
void vCheckSwapInvaders(unsigned char *swap_state);

/**
 * @brief Draws a single Alien on the given position, according to the given state.
 *
 * @param swap_state: reference to current state
 * @param alien_shape: reference to rectangle if image is not able to load
 * @param invaders_pos_x: invaders x position
 * @param invaders_pos_y: invaders y position
 * @param i: y number of Alien in invaders grid
 * @param j: x number of Alien in invaders grid
 * @param alien_1_1_image:
 * @param alien_1_2_image:
 * @param alien_2_1_image:
 * @param alien_2_2_image:
 * @param alien_3_1_image:
 * @param alien_3_2_image:
 */
void vDrawAlien(unsigned char *swap_state, my_square_t *alien_shape,
		short invaders_pos_x, short invaders_pos_y, unsigned char i,
		unsigned char j, image_handle_t alien_1_1_image,
		image_handle_t alien_1_2_image, image_handle_t alien_2_1_image,
		image_handle_t alien_2_2_image, image_handle_t alien_3_1_image,
		image_handle_t alien_3_2_image);

/**
 * @brief Draws invaders as a grid.
 *
 * @param swap_state: reference to current state
 * @param alien_shape: reference to rectangle if image is not able to load
 * @param alien_1_1_image:
 * @param alien_1_2_image:
 * @param alien_2_1_image:
 * @param alien_2_2_image:
 * @param alien_3_1_image:
 * @param alien_3_2_image:
 */
void vDrawInvaders(unsigned char *swap_state, my_square_t *alien_shape,
		image_handle_t alien_1_1_image, image_handle_t alien_1_2_image,
		image_handle_t alien_2_1_image, image_handle_t alien_2_2_image,
		image_handle_t alien_3_1_image, image_handle_t alien_3_2_image);

/**
 * @brief Draws a defined bullet from a image or alternatively a rectangle,
 * if it is alive. The rectangle also contains the position
 * in x and y.
 *
 * @param bullet_alive: indication if alive or not
 * @param bullet_shape: reference to rectangle if no image is provided
 * @param bullet_image: image handle to loaded bullet image
 */
void vDrawBullet(unsigned char bullet_alive, my_square_t *bullet_shape,
		image_handle_t bullet_image);

/**
 * @brief Draws the playership from a image or alternatively a rectangle,
 * if the image is not able to load. The rectangle also contains the position
 * in x and y.
 *
 * @param player_shape: reference to rectangle if no image is provided
 * @param player_image: image handle to loaded playership image
 */
void vDrawPlayer(my_square_t *player_shape, image_handle_t player_image);

/**
 * @brief Draws the mothership from a image or alternatively a rectangle,
 * if the image is not able to load. The rectangle also contains the position
 * in x and y.
 *
 * @param mothership_shape: reference to rectangle if no image is provided
 * @param mothership_image: image handle to loaded mothership image
 */
void vDrawMothership(unsigned char mothership_alive,
		my_square_t *mothership_shape, image_handle_t mothership_image);

/**
 * @brief Fetch Player bullets position and store it in the rectangle.
 *
 * @param player_shape: reference to rectangle of player
 * @param player_bullet_shape: reference to rectangle of player bullet
 * @param player_bullet_alive: indication if player is alive
 */
void vGetPlayerBulletPos(my_square_t *player_shape,
		my_square_t *player_bullet_shape, unsigned char *player_bullet_alive);

/**
 * @brief Fetch Aliens bullets position and store it in the rectangle.
 *
 * @param aliens_bullet_shape: reference to rectangle of alien bullet
 * @param aliens_bullet_alive: indication if player is alive
 */
void vGetAliensBulletPos(my_square_t *aliens_bullet_shape,
		unsigned char *aliens_bullet_alive);

/**
 * @brief Fetch motherships position and store it in the rectangle.
 *
 * @param mothership_shape: reference to rectangle of mothership
 * @param mothership_alive: indication if mothership is alive
 */
void vGetMothershipPos(my_square_t *mothership_shape,
		unsigned char *mothership_alive);

/**
 * @brief Draws for each bunker a fixed number of x and y blocks. Whether partly
 * destroyed can be seen in bunker struct, according to this destruction state,
 * different block_images get drawn. The rectangle also contains the position
 * in x and y.
 *
 * @param bunker_block_shape: reference to bunker block rectangle
 * @param bunker_block_worse_image:
 * @param bunker_block_bad_image:
 * @param bunker_block_good_image:
 */
void vDrawBunker(my_square_t *bunker_block_shape,
		image_handle_t bunker_block_worse_image,
		image_handle_t bunker_block_bad_image,
		image_handle_t bunker_block_good_image);

/**
 * @brief Task which draws the game continuously. Calls all the other
 * draw functions of the game. Update Period of 10 ticks. Corresponds to appr.
 * 50 fps.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vDrawGameTask(void *pvParameters);


/**
 * @brief initialize Tasks defined on this file.
 *
 * @returns 0 if successful and -1 if error occurred.
 */
int vInit_Draw();


#endif // __DRAW_H__

