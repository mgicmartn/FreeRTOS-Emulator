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



void checkDraw(unsigned char status, const char *msg);

void vDrawFPS(void);

void vDrawPopUpField(my_square_t* pop_up_page, unsigned char* next_state, short countdown );

void vDrawPlayButton(my_square_t* play_button);

void vDrawBackgroundImage(image_handle_t background_image);

void vDrawCheatButton(my_square_t* cheat_button);

void vDrawHighscoreButton(my_square_t* highscore_button);

void vDrawTwoPlayerButton(my_square_t* two_player_mode_button, unsigned char mothership_AI_control);

void vDrawBackButton(my_square_t* back_button);

void vDrawSetLevelButton(my_square_t* setlevel_button, short game_wrapper_level);

void vDrawInfiniteLifeButton(my_square_t* inflife_button, unsigned char game_wrapper_infinite_life_flag);

void vDrawSetScoreButton(my_square_t* setscore_button, unsigned char game_wrapper_set_score_flag);

void vDrawHighscore(short highscore);

void vDrawPopUpPageTask(void *pvParameters);

void vDrawLobbyMainTask(void *pvParameters);

void vDrawLobbyCheatTask(void *pvParameters);

void vDrawLobbyHighscoreTask(void *pvParameters);

void vDrawScore();

void vDrawPauseResume();

void vDrawReset();

void vDrawLevel();

void vDrawLifes(my_square_t* life_shape, image_handle_t life_image);

void check_swap_invaders(unsigned char *swap_state);

void vDrawAlien(unsigned char * swap_state, my_square_t* alien_shape, short invaders_pos_x, short invaders_pos_y, unsigned char i, unsigned char j, image_handle_t alien_1_1_image, image_handle_t alien_1_2_image, image_handle_t alien_2_1_image, image_handle_t alien_2_2_image, image_handle_t alien_3_1_image, image_handle_t alien_3_2_image);

void vDrawInvaders(unsigned char *swap_state, my_square_t* alien_shape, image_handle_t alien_1_1_image, image_handle_t alien_1_2_image, image_handle_t alien_2_1_image, image_handle_t alien_2_2_image, image_handle_t alien_3_1_image, image_handle_t alien_3_2_image);

void vDrawBullet(unsigned char bullet_alive, my_square_t* bullet_shape, image_handle_t bullet_image);

void vDrawPlayer(my_square_t* player_shape, image_handle_t player_image );

void vDrawMothership(unsigned char mothership_alive, my_square_t* mothership_shape, image_handle_t mothership_image );

void vGetPlayerBulletPos(my_square_t* player_shape, my_square_t* player_bullet_shape, unsigned char *player_bullet_alive);

void vGetAliensBulletPos(my_square_t* aliens_bullet_shape, unsigned char *aliens_bullet_alive);

void vGetMothershipPos(my_square_t* mothership_shape, unsigned char *mothership_alive);

void vDrawBunker(my_square_t* bunker_block_shape, image_handle_t bunker_block_worse_image, image_handle_t bunker_block_bad_image, image_handle_t bunker_block_good_image );

void vDrawGameTask(void *pvParameters);

int vInit_Draw();





#endif // __DRAW_H__


