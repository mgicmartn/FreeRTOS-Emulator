#ifndef __MAIN_H__
#define __MAIN_H__

#include "semphr.h"
#include "queue.h"

//#define NEXT_TASK 0
//#define PREV_TASK 1
//
//#define PRINT_TASK_ERROR(task) PRINT_ERROR("Failed to print task ##task");
//
//extern const unsigned char next_state_signal;
//extern const unsigned char prev_state_signal;
//
//extern SemaphoreHandle_t ScreenLock;
//extern SemaphoreHandle_t DrawSignal;
//
//extern QueueHandle_t StateQueue;
//
//void vDrawFPS(void);

extern const unsigned char one_state_signal;
extern const unsigned char two_state_signal;
extern const unsigned char three_state_signal;
extern const unsigned char four_state_signal;
extern const unsigned char five_state_signal;


extern const unsigned char move_right_signal;
extern const unsigned char move_left_signal;
extern const unsigned char stop_signal;
extern const unsigned char shoot_signal;

extern SemaphoreHandle_t ScreenLock;
extern SemaphoreHandle_t DrawSignal;
//extern SemaphoreHandle_t invaders.lock;


extern QueueHandle_t StateQueue;
extern QueueHandle_t PlayerQueue;
extern QueueHandle_t AlienShootsQueue;

void vDrawPopUpPageTask(void *pvParameters);


#endif // __MAIN_H__
