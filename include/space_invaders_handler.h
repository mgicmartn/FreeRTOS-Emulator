#ifndef __SPACE_INVADERS_HANDLER_H__
#define __SPACE_INVADERS_HANDLER_H__

#include "constants.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "main.h"


extern TaskHandle_t Init_Game;
extern TaskHandle_t Game_Handler;
extern TaskHandle_t UDPControlTask;

//typedef enum { INIT = 0, PLAYER_WON = 1, INVADERS_WON = 2} game_messages_t;


//#ifndef __PONG_H__
//#define __PONG_H__
//
//#include "FreeRTOS.h"
//#include "semphr.h"
//#include "task.h"
//
//#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
//#define mainGENERIC_STACK_SIZE ((unsigned short)2560)
//
//extern TaskHandle_t LeftPaddleTask;
//extern TaskHandle_t RightPaddleTask;
//extern TaskHandle_t PongControlTask;
//extern TaskHandle_t PausedStateTask;
//
//int pongInit(void);
//
//#endif // __PONG_H__

extern invaders_t invaders;
extern bunker_t bunker;
extern game_wrapper_t game_wrapper;
extern mothership_t mothership;
extern player_t player;

void increment_level();
void decrement_level();

void vUDPControlTask(void *pvParameters);

void UDPHandler(size_t read_size, char *buffer, void *args);

unsigned char xCheckPongUDPInput(unsigned short *paddle_y);

void get_highscore(short * highscore);

void get_game_wrapper_flags(unsigned char* game_wrapper_infinite_life_flag, unsigned char* game_wrapper_set_score_flag);

void set_score_flag(unsigned char score_value_flag);

void init_invaders(double speed);

void init_player(void);

void init_mothership(void);

void init_bunker(void);

void init_game_wrapper(double* speed);

void vInit_Game(void *pvParameters);

void move_alien_bullet(bullet_t* bullet, short speed);

void Alien_shoots();

void move_player_bullet(bullet_t* bullet, short speed);

void handle_player_input(unsigned char* moving_left, unsigned char* moving_right);

void move_invaders(unsigned char* invaders_won, TickType_t * last_time);

void move_mothership(TickType_t* last_time_mothership);

void handle_mothership_appearance(TickType_t last_time_mothership);

void vkill_Alien(unsigned char y, unsigned char x, unsigned char * player_won);

void vkill_Mothership(TickType_t * last_time_mothership);

void destruct_bunker_block_player(short s, short player_front, short t);

void destruct_bunker_block_alien(short s, short aliens_front, short t);

void player_dies(unsigned char *player_dead);

void check_aliens_bullet_collision(unsigned char *player_dead);

void check_player_bullet_collision(unsigned char * player_won, TickType_t * last_time_mothership);

void set_new_last_time_resume(	unsigned char* invaders_resume, TickType_t* last_time, TickType_t* last_time_mothership);

void handle_player_death(unsigned char* invaders_won);

void handle_invaders_won();

void handle_player_won();

void check_for_extra_life();

void vGame_Handler(void *pvParameters);

int init_space_invaders_handler(void);

#endif
