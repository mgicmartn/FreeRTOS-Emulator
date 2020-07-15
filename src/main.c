#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#include <SDL2/SDL_scancode.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "TUM_Ball.h"
#include "TUM_Draw.h"
#include "TUM_Event.h"
#include "TUM_Sound.h"
#include "TUM_Utils.h"
#include "TUM_Font.h"
#include "TUM_Print.h"

#include "AsyncIO.h"

#include "shapes.h"
#include "constants.h"
#include "space_invaders_handler.h"
#include "main.h"
#include "draw.h"

// state machine
const unsigned char one_state_signal = STATE_ONE;
const unsigned char two_state_signal = STATE_TWO;
const unsigned char three_state_signal = STATE_THREE;
const unsigned char four_state_signal = STATE_FOUR;
const unsigned char five_state_signal = STATE_FIVE;

// playership control
const unsigned char move_right_signal = MOVE_RIGHT;
const unsigned char move_left_signal = MOVE_LEFT;
const unsigned char stop_signal = STOP;
const unsigned char shoot_signal = SHOOT;

static TaskHandle_t StateMachine = NULL;
static TaskHandle_t BufferSwap = NULL;
static TaskHandle_t ButtonInputTask = NULL;
static TaskHandle_t SwapInvadersTask = NULL;
static TaskHandle_t AlienShootTask = NULL;

QueueHandle_t StateQueue = NULL;
QueueHandle_t PlayerQueue = NULL;
QueueHandle_t SwapInvadersQueue = NULL;
QueueHandle_t AlienShootsQueue = NULL;
SemaphoreHandle_t DrawSignal = NULL;
SemaphoreHandle_t ScreenLock = NULL;

// initialization of structs
static state_t state = { 0 };
static buttons_buffer_t buttons = { 0 };

void xGetButtonInput(void) {
	if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
		xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
		xSemaphoreGive(buttons.lock);
	}
}

unsigned char xGetState(void) {
	unsigned char my_state = 0;

	if (xSemaphoreTake(state.lock, portMAX_DELAY) == pdTRUE) {
		my_state = state.currState;
	}
	xSemaphoreGive(state.lock);

	return my_state;
}

void vSetState(unsigned char currState) {
	if (xSemaphoreTake(state.lock, portMAX_DELAY) == pdTRUE) {
		state.currState = currState;
	}
	xSemaphoreGive(state.lock);
}

/*
 * Example basic state machine with sequential states
 */
void basicSequentialStateMachine(void *pvParameters) {
	unsigned char current_state = STARTING_STATE; // Default state
	unsigned char state_changed = 1; // Only re-evaluate state if it has changed
	unsigned char input = 0;

	const int state_change_period = STATE_DEBOUNCE_DELAY;

	TickType_t last_change = xTaskGetTickCount();

	while (1) {
		if (state_changed) {
			goto initial_state;
		}

		// Handle state machine input
		if (StateQueue) {
			if (xQueueReceive(StateQueue, &input, portMAX_DELAY) == pdTRUE) {

				if (xTaskGetTickCount() - last_change > state_change_period) {

					current_state = input;
					vSetState(current_state); // set current state
					state_changed = 1;
					last_change = xTaskGetTickCount();

					prints("New state: %d\n", current_state);

				} else {
					prints("State Debounce conflict to state %d.\n",
							current_state);
				}
			}
		}

		initial_state:
		// Handle current state
		if (state_changed) {
			switch (current_state) {
			case STATE_ONE:
				prints("Now in state main lobby\n");
				if (DrawLobbyCheatTask)
					vTaskSuspend(DrawLobbyCheatTask);
				if (DrawLobbyHighscoreTask)
					vTaskSuspend(DrawLobbyHighscoreTask);
				if (DrawGameTask)
					vTaskSuspend(DrawGameTask);
				if (GameHandlerTask)
					vTaskSuspend(GameHandlerTask);
				if (SwapInvadersTask)
					vTaskSuspend(SwapInvadersTask);
				if (AlienShootTask)
					vTaskSuspend(AlienShootTask);
				if (InitGameTaks)
					vTaskSuspend(InitGameTaks);
				if (UDPControlTask)
					vTaskSuspend(UDPControlTask);
				if (DrawLobbyMainTask)
					vTaskResume(DrawLobbyMainTask);
				state_changed = 0;

				break;
			case STATE_TWO:
				prints("Now in state cheat mode\n");
				if (DrawLobbyMainTask)
					vTaskSuspend(DrawLobbyMainTask);
				if (DrawLobbyHighscoreTask)
					vTaskSuspend(DrawLobbyHighscoreTask);
				if (DrawGameTask)
					vTaskSuspend(DrawGameTask);
				if (GameHandlerTask)
					vTaskSuspend(GameHandlerTask);
				if (SwapInvadersTask)
					vTaskSuspend(SwapInvadersTask);
				if (AlienShootTask)
					vTaskSuspend(AlienShootTask);
				if (InitGameTaks)
					vTaskSuspend(InitGameTaks);
				if (UDPControlTask)
					vTaskSuspend(UDPControlTask);
				if (DrawLobbyCheatTask)
					vTaskResume(DrawLobbyCheatTask);
				state_changed = 0;

				break;
			case STATE_THREE:
				prints("Now in state highscore\n");
				if (DrawLobbyMainTask)
					vTaskSuspend(DrawLobbyMainTask);
				if (DrawLobbyCheatTask)
					vTaskSuspend(DrawLobbyCheatTask);
				if (DrawGameTask)
					vTaskSuspend(DrawGameTask);
				if (GameHandlerTask)
					vTaskSuspend(GameHandlerTask);
				if (SwapInvadersTask)
					vTaskSuspend(SwapInvadersTask);
				if (AlienShootTask)
					vTaskSuspend(AlienShootTask);
				if (InitGameTaks)
					vTaskSuspend(InitGameTaks);
				if (UDPControlTask)
					vTaskSuspend(UDPControlTask);
				if (DrawLobbyHighscoreTask)
					vTaskResume(DrawLobbyHighscoreTask);
				state_changed = 0;

				break;

			case STATE_FOUR:
				prints("Now in state init state\n");
				if (DrawLobbyMainTask)
					vTaskSuspend(DrawLobbyMainTask);
				if (DrawLobbyCheatTask)
					vTaskSuspend(DrawLobbyCheatTask);
				if (DrawLobbyHighscoreTask)
					vTaskSuspend(DrawLobbyHighscoreTask);
				if (GameHandlerTask)
					vTaskSuspend(GameHandlerTask);
				if (SwapInvadersTask)
					vTaskSuspend(SwapInvadersTask);
				if (AlienShootTask)
					vTaskSuspend(AlienShootTask);
				if (DrawGameTask)
					vTaskSuspend(DrawGameTask);
				if (InitGameTaks)
					vTaskResume(InitGameTaks);
				// check if Two Player Mode got selected
				if (xGetAIControl())
					if (UDPControlTask)
						vTaskResume(UDPControlTask);
				state_changed = 0;

				break;

			case STATE_FIVE:
				prints("Now in state gamestate\n");
//                	if (InitGameTaks) vTaskSuspend(InitGameTaks);
				if (DrawLobbyMainTask)
					vTaskSuspend(DrawLobbyMainTask);
				if (DrawLobbyCheatTask)
					vTaskSuspend(DrawLobbyCheatTask);
				if (DrawLobbyHighscoreTask)
					vTaskSuspend(DrawLobbyHighscoreTask);
				if (DrawGameTask)
					vTaskResume(DrawGameTask);
				if (GameHandlerTask)
					vTaskResume(GameHandlerTask);
				if (SwapInvadersTask)
					vTaskResume(SwapInvadersTask);
				if (AlienShootTask)
					vTaskResume(AlienShootTask);
				// check if Two Player Mode got selected
				if (xGetAIControl())
					if (UDPControlTask)
						vTaskResume(UDPControlTask);
				state_changed = 0;

				break;
			default:
				state_changed = 0;
				break;
			}
			state_changed = 0;
		}
	}
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
		StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
	/* If the buffers to be provided to the Idle task are declared inside this
	 function then they must be declared static – otherwise they will be allocated on
	 the stack and so not exists after this function exits. */
	static StaticTask_t xIdleTaskTCB;
	static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

	/* Pass out a pointer to the StaticTask_t structure in which the Idle task’s
	 state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task’s stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	 Note that, as the array is necessarily of type StackType_t,
	 configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
		StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
	/* If the buffers to be provided to the Timer task are declared inside this
	 function then they must be declared static – otherwise they will be allocated on
	 the stack and so not exists after this function exits. */
	static StaticTask_t xTimerTaskTCB;
	static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	 task’s state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task’s stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	 Note that, as the array is necessarily of type StackType_t,
	 configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void vCheckButtonP(unsigned char *keycodeP_last, const unsigned char go_to) {
	if (*keycodeP_last != buttons.buttons[KEYCODE(P)]) {
		if (buttons.buttons[KEYCODE(P)])
			if (StateQueue)
				xQueueSend(StateQueue, &go_to, 0);
		*keycodeP_last = buttons.buttons[KEYCODE(P)];
	}
}

void vCheckButtonGameP(unsigned char *keycodeP_last, unsigned char *paused) {
	if (*keycodeP_last != buttons.buttons[KEYCODE(P)]) {
		if (buttons.buttons[KEYCODE(P)]) {

			if (*paused == 0) { // game is running and gets paused

				// send PAUSE to binary
				static char buf[50];
				sprintf(buf, "PAUSE");
				aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, buf, strlen(buf));

				// suspend game tasks
				if (SwapInvadersTask)
					vTaskSuspend(SwapInvadersTask);
				if (GameHandlerTask)
					vTaskSuspend(GameHandlerTask);
				if (DrawGameTask)
					vTaskSuspend(DrawGameTask);
				if (xGetAIControl())
					if (UDPControlTask)
						vTaskSuspend(UDPControlTask);


				// set paused state for other tasks to check if needed
				if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE) {
					invaders.paused = 1;
				}
				xSemaphoreGive(invaders.lock);
				*paused = 1;

			} else if (*paused == 1) { // game is paused and gets resumed

				// resume game tasks
				if (GameHandlerTask)
					vTaskResume(GameHandlerTask);
				if (SwapInvadersTask)
					vTaskResume(SwapInvadersTask);
				if (DrawGameTask)
					vTaskResume(DrawGameTask);
				if (xGetAIControl())
					if (UDPControlTask)
						vTaskResume(UDPControlTask);

				// send RESUME to binary
				static char buf[50];
				sprintf(buf, "RESUME");
				aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, buf, strlen(buf));

				// reset paused state and set resume for last_time reset
				if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE) {
					invaders.resume = 1;
					invaders.paused = 0;
				}
				xSemaphoreGive(invaders.lock);
				*paused = 0;
			}
		}

		*keycodeP_last = buttons.buttons[KEYCODE(P)];
	}
}

void vCheckButtonC(unsigned char *keycodeC_last, const unsigned char go_to) {
	if (*keycodeC_last != buttons.buttons[KEYCODE(C)]) {
		if (buttons.buttons[KEYCODE(C)])
			if (StateQueue)
				xQueueSend(StateQueue, &go_to, 0);
		*keycodeC_last = buttons.buttons[KEYCODE(C)];
	}
}

void vCheckButtonA(unsigned char *keycodeA_last) {
	if (*keycodeA_last != buttons.buttons[KEYCODE(A)]) {
		if (buttons.buttons[KEYCODE(A)]) {
			if (xSemaphoreTake(mothership.lock, portMAX_DELAY) == pdTRUE) {
				// select TwoPlayerMode, difficulty
				mothership.AI_control = (mothership.AI_control + 1) % 4;
				xQueueOverwrite(DifficultyQueue,
						(void* )&mothership.AI_control);
			}
			xSemaphoreGive(mothership.lock);
		}
		*keycodeA_last = buttons.buttons[KEYCODE(A)];
	}
}

void vCheckButtonH(unsigned char *keycodeH_last, const unsigned char go_to) {
	if (*keycodeH_last != buttons.buttons[KEYCODE(H)]) {
		if (buttons.buttons[KEYCODE(H)])
			if (StateQueue)
				xQueueSend(StateQueue, &go_to, 0);
		*keycodeH_last = buttons.buttons[KEYCODE(H)];
	}
}

void vCheckButtonGameB(unsigned char *keycodeB_last, const unsigned char go_to,
		unsigned char *paused) {
	if (*keycodeB_last != buttons.buttons[KEYCODE(B)]) {
		if (buttons.buttons[KEYCODE(B)]) {
			// handle reset from game
			if (*paused)
				*paused = 0;
			vEndMatch(RESET_PRESSED);
			if (StateQueue)
				xQueueSend(StateQueue, &go_to, 0);
		}
		*keycodeB_last = buttons.buttons[KEYCODE(B)];
	}
}

void vCheckButtonB(unsigned char *keycodeB_last, const unsigned char go_to) {
	if (*keycodeB_last != buttons.buttons[KEYCODE(B)]) {
		if (buttons.buttons[KEYCODE(B)]) {
			if (StateQueue)
				xQueueSend(StateQueue, &go_to, 0);
		}
		*keycodeB_last = buttons.buttons[KEYCODE(B)];
	}
}

void vCheckButtonL(unsigned char *keycodeL_last) {
	if (*keycodeL_last != buttons.buttons[KEYCODE(L)]) {
		if (buttons.buttons[KEYCODE(L)]) {
			if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE) {
				game_wrapper.infinite_life_flag =
						(game_wrapper.infinite_life_flag + 1) % 2;
			}
			xSemaphoreGive(game_wrapper.lock);
		}
		*keycodeL_last = buttons.buttons[KEYCODE(L)];
	}
}

void vCheckButtonT(unsigned char *keycodeT_last) {
	if (*keycodeT_last != buttons.buttons[KEYCODE(T)]) {
		if (buttons.buttons[KEYCODE(T)]) {
			vSetScoreFlag(SET_SCORE_100);
		}
		*keycodeT_last = buttons.buttons[KEYCODE(T)];
	}
}

void vCheckButtonK(unsigned char *keycodeK_last) {
	if (*keycodeK_last != buttons.buttons[KEYCODE(K)]) {
		if (buttons.buttons[KEYCODE(K)]) {
			vSetScoreFlag(SET_SCORE_1000);
		}
		*keycodeK_last = buttons.buttons[KEYCODE(K)];
	}
}

void vCheckButtonU(unsigned char *keycodeU_last) {
	if (*keycodeU_last != buttons.buttons[KEYCODE(U)]) {
		if (buttons.buttons[KEYCODE(U)]) {
			vSetScoreFlag(SET_SCORE_10000);
		}
		*keycodeU_last = buttons.buttons[KEYCODE(U)];
	}
}

void vCheckButtonLEFT(unsigned char *keycodeLEFT_last, short *debounce_LEFT,
		const unsigned char move, const unsigned char stop) {

	if (*keycodeLEFT_last != buttons.buttons[KEYCODE(LEFT)])
		(*debounce_LEFT)++;
	else
		(*debounce_LEFT) = 0;

	if (*debounce_LEFT >= 1) {// this function gets again called after 50 ticks (Debounce Delay)
		if (*keycodeLEFT_last != buttons.buttons[KEYCODE(LEFT)]) {
			if (buttons.buttons[KEYCODE(LEFT)] == 1) {
				// move player left
				if (PlayerQueue)
					xQueueSend(PlayerQueue, &move, 0);
				*keycodeLEFT_last = buttons.buttons[KEYCODE(LEFT)];

			} else if (buttons.buttons[KEYCODE(LEFT)] == 0) {
				// stop player
				if (PlayerQueue)
					xQueueSend(PlayerQueue, &stop, 0);
				*keycodeLEFT_last = buttons.buttons[KEYCODE(LEFT)];
			}
		}
		*debounce_LEFT = 0;
	}

}

void vCheckButtonRIGHT(unsigned char *keycodeRIGHT_last, short *debounce_RIGHT,
		const unsigned char move, const unsigned char stop) {

	if (*keycodeRIGHT_last != buttons.buttons[KEYCODE(RIGHT)])
		(*debounce_RIGHT)++;
	else
		(*debounce_RIGHT) = 0;

	if (*debounce_RIGHT >= 1) { // this function gets again called after 50 ticks (Debounce Delay)
		if (*keycodeRIGHT_last != buttons.buttons[KEYCODE(RIGHT)]) {
			if (buttons.buttons[KEYCODE(RIGHT)] == 1) {
				// move player right
				if (PlayerQueue)
					xQueueSend(PlayerQueue, &move, 0);
				*keycodeRIGHT_last = buttons.buttons[KEYCODE(RIGHT)];

			} else if (buttons.buttons[KEYCODE(RIGHT)] == 0) {
				// stop player
				if (PlayerQueue)
					xQueueSend(PlayerQueue, &stop, 0);
				*keycodeRIGHT_last = buttons.buttons[KEYCODE(RIGHT)];
			}
		}
		*debounce_RIGHT = 0;
	}
}

void vCheckButtonUP(unsigned char *keycodeUP_last) {
	if (*keycodeUP_last != buttons.buttons[KEYCODE(UP)]) {
		if (buttons.buttons[KEYCODE(UP)] == 1) {
			// cheat mode level selection
			increment_level();
		}
		*keycodeUP_last = buttons.buttons[KEYCODE(UP)];
	}
}

void vCheckButtonDOWN(unsigned char *keycodeDOWN_last) {
	if (*keycodeDOWN_last != buttons.buttons[KEYCODE(DOWN)]) {
		if (buttons.buttons[KEYCODE(DOWN)] == 1) {
			// cheat mode level selection
			decrement_level();
		}
		*keycodeDOWN_last = buttons.buttons[KEYCODE(DOWN)];

	}
}

void vCheckButtonSPACE(unsigned char *keycodeSPACE_last,
		const unsigned char event) {
	if (*keycodeSPACE_last != buttons.buttons[KEYCODE(SPACE)]) {
		if (buttons.buttons[KEYCODE(SPACE)])
			if (PlayerQueue)
				xQueueSend(PlayerQueue, &event, 0);
		*keycodeSPACE_last = buttons.buttons[KEYCODE(SPACE)];
	}
}

#define eSTATE_RUNNING 2
#define eSTATE_SUSPENDED 4

void vButtonInputTask(void *pvParameters) {

	unsigned char currentState = 0;
	unsigned char lastState = 0;
	unsigned char paused = 0;

	unsigned char keycodeA_last = 0;
	unsigned char keycodeC_last = 0;
	unsigned char keycodeH_last = 0;
	unsigned char keycodeP_last = 0;
	unsigned char keycodeB_last = 0;
	unsigned char keycodeL_last = 0;
	unsigned char keycodeT_last = 0;
	unsigned char keycodeK_last = 0;
	unsigned char keycodeU_last = 0;
	unsigned char keycodeRIGHT_last = 0;
	unsigned char keycodeLEFT_last = 0;
	unsigned char keycodeSPACE_last = 0;
	unsigned char keycodeUP_last = 0;
	unsigned char keycodeDOWN_last = 0;

	short debounceLEFT = 0;
	short debounceRIGHT = 0;

	while (1) {
		// fetch buttons and state
		xGetButtonInput();
		currentState = xGetState();

		if (currentState != lastState) {
			prints("Current State: %d\n", currentState + 1);
			lastState = currentState;
		}

		// KeyPressedEvents dependent on current state
		switch (currentState) {

		case STATE_ONE:	// lobby main
			if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE) {

				vCheckButtonP(&keycodeP_last, four_state_signal);
				vCheckButtonC(&keycodeC_last, two_state_signal);
				vCheckButtonH(&keycodeH_last, three_state_signal);
				vCheckButtonA(&keycodeA_last);
			}
			xSemaphoreGive(buttons.lock);
			break;

		case STATE_TWO:	// lobby cheat mode
			if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE) {

				vCheckButtonP(&keycodeP_last, four_state_signal);
				vCheckButtonB(&keycodeB_last, one_state_signal);
				vCheckButtonL(&keycodeL_last);
				vCheckButtonT(&keycodeT_last);
				vCheckButtonK(&keycodeK_last);
				vCheckButtonU(&keycodeU_last);
				vCheckButtonDOWN(&keycodeDOWN_last);
				vCheckButtonUP(&keycodeUP_last);
			}
			xSemaphoreGive(buttons.lock);
			break;

		case STATE_THREE: // lobby highscore
			if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE) {
				vCheckButtonB(&keycodeB_last, one_state_signal);
			}
			xSemaphoreGive(buttons.lock);
			break;

		case STATE_FOUR: // Init state
			break;

		case STATE_FIVE: // Game state
			if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE) {

				vCheckButtonGameB(&keycodeB_last, one_state_signal, &paused);
				vCheckButtonGameP(&keycodeP_last, &paused);

				if (!paused) {
					vCheckButtonLEFT(&keycodeLEFT_last, &debounceLEFT,
							move_left_signal, stop_signal);
					vCheckButtonRIGHT(&keycodeRIGHT_last, &debounceRIGHT,
							move_right_signal, stop_signal);
					vCheckButtonSPACE(&keycodeSPACE_last, shoot_signal);
				}
			}
			xSemaphoreGive(buttons.lock);
			break;

		default:
			break;
		}
		vTaskDelay(50); // basic delay of 50ticks
	}
}

void vSwapInvadersTask(void *pvParameters) {
	TickType_t xWaitingTime = 1000;

	unsigned char swap = 1;

	while (1) {
		if (xSemaphoreTake(invaders.lock, 0) == pdTRUE) {
			// dependent on current invader speed, set frequency of this task
			xWaitingTime = abs(round((1 / invaders.speed) * 40));
		}
		xSemaphoreGive(invaders.lock);

		// send swap signal
		if (SwapInvadersQueue)
			xQueueSend(SwapInvadersQueue, &swap, 0);

		vTaskDelay(pdMS_TO_TICKS(xWaitingTime));
	}
}

void vAlienShootTask(void *pvParameters) {
	unsigned char shoot = 1;
	TickType_t xWaitingTime = ALIEN_SHOOT_DELAY;

	while (1) {
		// send shoot signal
		if (AlienShootsQueue)
			xQueueSend(AlienShootsQueue, &shoot, 0);
		vTaskDelay(pdMS_TO_TICKS(xWaitingTime));// basic delay of ALIEN_SHOOT_DELAY
	}
}

void vSwapBuffers(void *pvParameters) {
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	const TickType_t frameratePeriod = 19;

	tumDrawBindThread(); // Setup Rendering handle with correct GL context

	while (1) {

		if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE) {
			tumDrawUpdateScreen();
			tumEventFetchEvents(FETCH_EVENT_BLOCK);
			xSemaphoreGive(ScreenLock);
			xSemaphoreGive(DrawSignal);
			vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(frameratePeriod));
		}
	}
}

int main(int argc, char *argv[]) {
	char *bin_folder_path = tumUtilGetBinFolderPath(argv[0]);

	prints("Initializing: ");

	if (tumDrawInit(bin_folder_path)) {
		PRINT_ERROR("Failed to initialize drawing");
		goto err_init_drawing;
	}

	if (tumEventInit()) {
		PRINT_ERROR("Failed to initialize events");
		goto err_init_events;
	}

	if (tumSoundInit(bin_folder_path)) {
		PRINT_ERROR("Failed to initialize audio");
		goto err_init_audio;
	}

	atexit(aIODeinit);

	// locks and semaphores
	buttons.lock = xSemaphoreCreateMutex(); // Button Locking mechanism
	if (!buttons.lock) {
		PRINT_ERROR("Failed to create buttons lock");
		goto err_buttons_lock;
	}
	state.lock = xSemaphoreCreateMutex(); 	//State Locking meachanism
	if (!state.lock) {
		PRINT_ERROR("Failed to create state lock");
		goto err_state_lock;
	}

	ScreenLock = xSemaphoreCreateMutex();	// Screen Locking
	if (!ScreenLock) {
		PRINT_ERROR("Failed to create screen lock");
		goto err_screen_lock;
	}

	DrawSignal = xSemaphoreCreateBinary(); 	// Screen buffer locking
	if (!DrawSignal) {
		PRINT_ERROR("Failed to create draw signal");
		goto err_draw_signal;
	}

	StateQueue = xQueueCreate(STATE_QUEUE_LENGTH, sizeof(unsigned char));
	if (!StateQueue) {			// State Queue
		PRINT_ERROR("Could not open state queue");
		goto err_state_queue;
	}

	PlayerQueue = xQueueCreate(PLAYER_QUEUE_LENGTH, sizeof(unsigned char));
	if (!PlayerQueue) {			// Player Queue
		PRINT_ERROR("Could not open Player Queue");
		goto err_PlayerQueue;
	}

	SwapInvadersQueue = xQueueCreate(SWAP_INVADERS_QUEUE_LENGTH,
			sizeof(unsigned char));
	if (!SwapInvadersQueue) {			// Swap Invaders Queue
		PRINT_ERROR("Could not open SwapInvadersQueue");
		goto err_SwapInvadersQueue;
	}
	AlienShootsQueue = xQueueCreate(ALIENS_SHOOTS_QUEUE_LENGTH,
			sizeof(unsigned char));
	if (!AlienShootsQueue) {			// Alien Shoots Queue
		PRINT_ERROR("Could not open AlienShootsQueue");
		goto err_AlienShootsQueue;
	}

	// Tasks
	// Running Always
	if (xTaskCreate(basicSequentialStateMachine, "StateMachine",
	mainGENERIC_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 1,
			StateMachine) != pdPASS) {
		PRINT_TASK_ERROR("StateMachine");
		goto err_statemachine;
	}
	if (xTaskCreate(vSwapBuffers, "BufferSwapTask",
	mainGENERIC_STACK_SIZE * 2, NULL, configMAX_PRIORITIES,
			BufferSwap) != pdPASS) {
		PRINT_TASK_ERROR("BufferSwapTask");
		goto err_bufferswap;
	}
	if (xTaskCreate(vButtonInputTask, "ButtonInputTask",
	mainGENERIC_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 1,
			ButtonInputTask) != pdPASS) {
		PRINT_TASK_ERROR("ButtonInputTask");
		goto err_ButtonInputTask;
	}

	// Tasks
	// controlled by state machine
	if (xTaskCreate(vSwapInvadersTask, "SwapInvadersTask",
	mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 1,
			&SwapInvadersTask) != pdPASS) {
		PRINT_TASK_ERROR("SwapInvadersTask");
		goto err_SwapInvadersTask;
	}

	if (xTaskCreate(vAlienShootTask, "AlienShootTask",
	mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 1,
			&AlienShootTask) != pdPASS) {
		PRINT_TASK_ERROR("AlienShootTask");
		goto err_AlienShootTask;
	}

	// initialize from other files
	init_space_invaders_handler();
	vInit_Draw();

	vTaskSuspend(SwapInvadersTask);
	vTaskSuspend(AlienShootTask);

	vTaskStartScheduler();

	return EXIT_SUCCESS;

err_AlienShootTask:
	vTaskDelete(SwapInvadersTask);
err_SwapInvadersTask:
	vTaskDelete(ButtonInputTask);
err_ButtonInputTask:
	vTaskDelete(BufferSwap);
err_bufferswap:
	vTaskDelete(StateMachine);
err_statemachine:
	vQueueDelete(AlienShootsQueue);
err_AlienShootsQueue:
	vQueueDelete(SwapInvadersQueue);
err_SwapInvadersQueue:
	vQueueDelete(PlayerQueue);
err_PlayerQueue:
	 vQueueDelete(StateQueue);
err_state_queue:
	vSemaphoreDelete(DrawSignal);
err_draw_signal:
	vSemaphoreDelete(ScreenLock);
err_screen_lock:
	vSemaphoreDelete(state.lock);
err_state_lock:
	vSemaphoreDelete(buttons.lock);
err_buttons_lock:
	tumSoundExit();
 err_init_audio:
 	 tumDrawExit();
err_init_drawing:
	tumEventExit();
err_init_events:


	return EXIT_FAILURE;
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vMainQueueSendPassed(void) {
	/* This is just an example implementation of the "queue send" trace hook. */
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vApplicationIdleHook(void) {
#ifdef __GCC_POSIX__
    struct timespec xTimeToSleep, xTimeSlept;
    /* Makes the process more agreeable when using the Posix simulator. */
    xTimeToSleep.tv_sec = 1;
    xTimeToSleep.tv_nsec = 0;
    nanosleep(&xTimeToSleep, &xTimeSlept);
#endif
}
