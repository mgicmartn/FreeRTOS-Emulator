/**
 * @file main.h
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


#ifndef __MAIN_H__
#define __MAIN_H__

#include "semphr.h"
#include "queue.h"

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

extern QueueHandle_t StateQueue;
extern QueueHandle_t PlayerQueue;
extern QueueHandle_t AlienShootsQueue;
extern QueueHandle_t SwapInvadersQueue;


/**
 * @brief Receives current KeyStates from buttonInputQueue
 */
void xGetButtonInput(void);

/**
 * @brief Get current state, which is defined in global state struct
 *
 * @returns: current state
 */
unsigned char xGetState(void);

/**
 * @brief Set current state in global state struct
 *
 * @param currState: current state
 */
void vSetState(unsigned char currState);

/**
 * @brief State Machine, which receives states from the stateQueue and handles
 * according to this, suspending and resuming of state Tasks.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void basicSequentialStateMachine(void *pvParameters);

// see FreeRTOS for documentation
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
		StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
		StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize);


/**
 * @brief Checks Button P and if it is pressed, singnal to state machine is
 * sent to change to state go_to.
 *
 * @param keycodeP_last: defines last KeyPressed status
 * @param go_to: state signal of state you want to jump to.
 */
void vCheckButtonP(unsigned char *keycodeP_last, const unsigned char go_to);

/**
 * @brief Checks Button P while game is running. It triggers the Pause and
 * after pressing again the Resume Mode. Within the function it Suspends
 * and Resumes the game tasks.
 *
 * @param keycodeP_last: defines last KeyPressed status
 * @param paused: indicates whether current game is paused or not.
 */
void vCheckButtonGameP(unsigned char *keycodeP_last, unsigned char *paused);

/**
 * @brief Checks Button C and if it is pressed, singnal to state machine is
 * sent to change to state go_to.
 *
 * @param keycodeP_last: defines last KeyPressed status
 * @param go_to: state signal of state you want to jump to.
 */
void vCheckButtonC(unsigned char *keycodeC_last, const unsigned char go_to);

/**
 * @brief Checks Button A and depending on the number it was pressed, it switches
 * between TwoPlayerModus of different difficulties and SinglePlayerModus.
 *
 * @param keycodeA_last: defines last KeyPressed status
 */
void vCheckButtonA(unsigned char *keycodeA_last);

/**
 * @brief Checks Button H and if it is pressed, singnal to state machine is
 * sent to change to state go_to.
 *
 * @param keycodeH_last: defines last KeyPressed status
 * @param go_to: state signal of state you want to jump to.
 */
void vCheckButtonH(unsigned char *keycodeH_last, const unsigned char go_to);

/**
 * @brief Checks Button B while game is running. It resets the match by calling the endMAtch
 * function and jumps to the state defined in go_to.
 *
 * @param keycodeB_last: defines last KeyPressed status
 * @param go_to: state signal of state you want to jump to.
 * @param paused: indicates whether current game is paused or not.
 */
void vCheckButtonGameB(unsigned char *keycodeB_last, const unsigned char go_to,
		unsigned char *paused);

/**
 * @brief Checks Button B and if it is pressed, singnal to state machine is
 * sent to change to state go_to.
 *
 * @param keycodeB_last: defines last KeyPressed status
 * @param go_to: state signal of state you want to jump to.
 */
void vCheckButtonB(unsigned char *keycodeB_last, const unsigned char go_to);

/**
 * @brief Checks Button L and if pressed, it sets/resets infinitive.life.flag
 * in CheatMode.
 *
 * @param keycodeL_last: defines last KeyPressed status
 */
void vCheckButtonL(unsigned char *keycodeL_last);

/**
 * @brief Checks Button L and if pressed, it sets/resets setScore(100).flag
 * in CheatMode.
 *
 * @param keycodeT_last: defines last KeyPressed status
 */
void vCheckButtonT(unsigned char *keycodeT_last);

/**
 * @brief Checks Button L and if pressed, it sets/resets setScore(1000).flag
 * in CheatMode.
 *
 * @param keycodeK_last: defines last KeyPressed status
 */
void vCheckButtonK(unsigned char *keycodeK_last);

/**
 * @brief Checks Button L and if pressed, it sets/resets setScore(10000).flag
 * in CheatMode.
 *
 * @param keycodeU_last: defines last KeyPressed status
 */
void vCheckButtonU(unsigned char *keycodeU_last);

/**
 * @brief Checks left arrow Key. If pressed player is moving left. If released player
 * stops moving. Player gets the signal with an PlayerQueue. Basic Debounce-logic
 * implemented.
 *
 * @param keycodeLEFT_last: defines last KeyPressed status
 * @param debounce_LEFT: In between State of KeysPressed to check if still the same.
 * @param move: signal to send with queue
 * @param stop: signal to send with queue
 */
void vCheckButtonLEFT(unsigned char *keycodeLEFT_last, short *debounce_LEFT,
		const unsigned char move, const unsigned char stop);

/**
 * @brief Checks right arrow Key. If pressed player is moving richt. If released player
 * stops moving. Player gets the signal with an PlayerQueue. Basic Debounce-logic
 * implemented.
 *
 * @param keycodeRIGHT_last: defines last KeyPressed status
 * @param debounce_RIGHT: In between State of KeysPressed to check if still the same.
 * @param move: signal to send with queue
 * @param stop: signal to send with queue
 */
void vCheckButtonRIGHT(unsigned char *keycodeRIGHT_last, short *debounce_RIGHT,
		const unsigned char move, const unsigned char stop);

/**
 * @brief Checks Button UP and if pressed, it increments the manually set level
 * selection in CheatMode.
 *
 * @param keycodeUP_last: defines last KeyPressed status
 */
void vCheckButtonUP(unsigned char *keycodeUP_last);

/**
 * @brief Checks Button DOWN and if pressed, it increments the manually set level
 * selection in CheatMode.
 *
 * @param keycodeDOWN_last: defines last KeyPressed status
 */
void vCheckButtonDOWN(unsigned char *keycodeDOWN_last);

/**
 * @brief Checks Button SPACE and if pressed, the player shoots.
 * Player gets the signal with an PlayerQueue
 *
 * @param keycodeSPACE_last: defines last KeyPressed status
 * @param event: signal to send with queue
 */
void vCheckButtonSPACE(unsigned char *keycodeSPACE_last,
		const unsigned char event);

/**
 * @brief Task which continuously queries for KeyStateEvents, depending on
 * the current state. Basic delay of 50 ticks.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vButtonInputTask(void *pvParameters);

/**
 * @brief Runs with Frequency dependant on invaders speed. Every loop it sends a
 * swap signal, to let the Aliens change their state. When the speed of the aliens
 * increases, swapping speeds also up.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vSwapInvadersTask(void *pvParameters);

/**
 * @brief Runs with continuous frequency and sends every loop a shoot signal.
 * This leads to periodic shooting behaviour of the Aliens.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vAlienShootTask(void *pvParameters);

/**
 * @brief Handles as a central position all drawing events. It is the Task with the
 * correct GL context to do so.
 *
 * @param pvParameters: Parameters passed into the function upon execution.
 */
void vSwapBuffers(void *pvParameters);


int main(int argc, char *argv[]);



#endif // __MAIN_H__
