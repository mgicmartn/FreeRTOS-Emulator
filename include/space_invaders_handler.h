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


void UDPHandler(size_t read_size, char *buffer, void *args);

void vUDPControlTask(void *pvParameters);

unsigned char xCheckMothershipUDPInput();

void set_score_flag(unsigned char score_value_flag);

void increment_level();

void decrement_level();

void vInitInvaders(double speed);

void vInitPlayer(void);

void vInitMothership(void);

void vInitBunker(void);

void vInitGameWrapper(double* speed);

void vInitGameTaks(void *pvParameters);

void vMoveAlienBullet(bullet_t* bullet, short speed);

void vAlienShoot();

void move_player_bullet(bullet_t* bullet, short speed);

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

void handle_end_match(end_game_reason_t reason);

void vCheckForExtraLife();

void vGameHandlerTask(void *pvParameters);

int init_space_invaders_handler(void);



#endif
