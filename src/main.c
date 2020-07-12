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


const unsigned char one_state_signal = STATE_ONE;
const unsigned char two_state_signal = STATE_TWO;
const unsigned char three_state_signal = STATE_THREE;
const unsigned char four_state_signal = STATE_FOUR;
const unsigned char five_state_signal = STATE_FIVE;


const unsigned char move_right_signal = MOVE_RIGHT;
const unsigned char move_left_signal = MOVE_LEFT;
const unsigned char stop_signal = STOP;
const unsigned char shoot_signal = SHOOT;


// Core
static TaskHandle_t StateMachine = NULL;
static TaskHandle_t BufferSwap = NULL;
static TaskHandle_t buttonInput = NULL;

QueueHandle_t StateQueue = NULL;
QueueHandle_t PlayerQueue = NULL;
static QueueHandle_t SwapInvadersQueue = NULL;
QueueHandle_t AlienShootsQueue = NULL;
SemaphoreHandle_t DrawSignal = NULL;
SemaphoreHandle_t ScreenLock = NULL;



// State2
static TaskHandle_t Draw_Lobby_Main = NULL;
static TaskHandle_t Draw_Lobby_Cheat = NULL;
static TaskHandle_t Draw_Lobby_Highscore = NULL;
static TaskHandle_t Draw_Game = NULL;
static TaskHandle_t Swap_Invaders = NULL;
static TaskHandle_t Let_Alien_Shoot = NULL;


typedef struct state {
	unsigned char currState;
	SemaphoreHandle_t lock;
} state_t;

static state_t state = { 0 };


typedef struct buttons_buffer {
    unsigned char buttons[SDL_NUM_SCANCODES];
    SemaphoreHandle_t lock;
} buttons_buffer_t;

static buttons_buffer_t buttons = { 0 };




void xGetButtonInput(void)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE)
    {
        xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
        xSemaphoreGive(buttons.lock);
    }
}

unsigned char xGetState(void)
{
	unsigned char my_state = 0;

    if (xSemaphoreTake(state.lock, 0) == pdTRUE)
    {
        my_state = state.currState;
        xSemaphoreGive(state.lock);
    }

    return my_state;
}

void vSetState(unsigned char currState)
{
    if (xSemaphoreTake(state.lock, 0) == pdTRUE)
    {
        state.currState = currState;
        xSemaphoreGive(state.lock);
    }
}


void checkDraw(unsigned char status, const char *msg)
{
    if (status) {
        if (msg)
            fprints(stderr, "[ERROR] %s, %s\n", msg, tumGetErrorMessage());
        else {
            fprints(stderr, "[ERROR] %s\n", tumGetErrorMessage());
        }
    }
}

void give_all_sempaphores()
{
    if(xSemaphoreGive(invaders.lock) != pdTRUE)
    {
    	prints("1 invaders lock wasn't taken.\n");
		// fflush(stdout);
    }
    if(xSemaphoreGive(player.lock) != pdTRUE)
    {
    	prints("2 player lock wasn't taken.\n");
		// fflush(stdout);
    }
    if(xSemaphoreGive(bunker.lock) != pdTRUE)
    {
    	prints("3 bunker lock wasn't taken.\n");
		// fflush(stdout);
    }
    if(xSemaphoreGive(game_wrapper.lock) != pdTRUE)
    {
    	prints("4 game_wrapper lock wasn't taken.\n");
		// fflush(stdout);
    }
    if(xSemaphoreGive(mothership.lock) != pdTRUE)
    {
    	prints("5 mothership lock wasn't taken.\n");
		// fflush(stdout);
    }
}


unsigned char AI_control()
{
	unsigned char mothership_AI_control = 0;

	if (xSemaphoreTake(mothership.lock, portMAX_DELAY) == pdTRUE)
	{
		mothership_AI_control = mothership.AI_control;

		xSemaphoreGive(mothership.lock);
	}

	return mothership_AI_control;
}


/*
 * Example basic state machine with sequential states
 */
void basicSequentialStateMachine(void *pvParameters)
{
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
        if (StateQueue)
        {
            if (xQueueReceive(StateQueue, &input, portMAX_DELAY) == pdTRUE)
            {

                if (xTaskGetTickCount() - last_change > state_change_period) {
    				current_state = input;

    				// set current state
    				vSetState(current_state);

    				prints("new state in statemachine %d\n",current_state);
    				// fflush(stdout);

    				state_changed = 1;
    				last_change = xTaskGetTickCount();
                }
                else {
    				prints("new state in statemachine BUT DEBUUNCE %d\n",current_state);
    				// fflush(stdout);
                }


            }
        }



initial_state:
        // Handle current state
        if (state_changed) {
            switch (current_state) {
                case STATE_ONE:
                	give_all_sempaphores();
                    if (Draw_Lobby_Cheat) vTaskSuspend(Draw_Lobby_Cheat);
                    if (Draw_Lobby_Highscore) vTaskSuspend(Draw_Lobby_Highscore);
                    if (Draw_Game) vTaskSuspend(Draw_Game);
                    if (Game_Handler) vTaskSuspend(Game_Handler);
                    if (Swap_Invaders) vTaskSuspend(Swap_Invaders);
                    if (Let_Alien_Shoot) vTaskSuspend(Let_Alien_Shoot);
                    if (Init_Game) vTaskSuspend(Init_Game);
                    if (UDPControlTask) vTaskSuspend(UDPControlTask);
                    if (Draw_Lobby_Main) vTaskResume(Draw_Lobby_Main);
                    state_changed = 0;

                    break;
                case STATE_TWO:
                	give_all_sempaphores();
                    if (Draw_Lobby_Main) vTaskSuspend(Draw_Lobby_Main);
                    if (Draw_Lobby_Highscore) vTaskSuspend(Draw_Lobby_Highscore);
                    if (Draw_Game) vTaskSuspend(Draw_Game);
                    if (Game_Handler) vTaskSuspend(Game_Handler);
                    if (Swap_Invaders) vTaskSuspend(Swap_Invaders);
                    if (Let_Alien_Shoot) vTaskSuspend(Let_Alien_Shoot);
                    if (Init_Game) vTaskSuspend(Init_Game);
                    if (UDPControlTask) vTaskSuspend(UDPControlTask);
                    if (Draw_Lobby_Cheat) vTaskResume(Draw_Lobby_Cheat);
                    state_changed = 0;

                    break;
                case STATE_THREE:
                	give_all_sempaphores();
                    if (Draw_Lobby_Main) vTaskSuspend(Draw_Lobby_Main);
                    if (Draw_Lobby_Cheat) vTaskSuspend(Draw_Lobby_Cheat);
                    if (Draw_Game) vTaskSuspend(Draw_Game);
                    if (Game_Handler) vTaskSuspend(Game_Handler);
                    if (Swap_Invaders) vTaskSuspend(Swap_Invaders);
                    if (Let_Alien_Shoot) vTaskSuspend(Let_Alien_Shoot);
                    if (Init_Game) vTaskSuspend(Init_Game);
                    if (UDPControlTask) vTaskSuspend(UDPControlTask);
                    if (Draw_Lobby_Highscore) vTaskResume(Draw_Lobby_Highscore);
                    state_changed = 0;

                    break;

                case STATE_FOUR:
                	give_all_sempaphores();
                    if (Draw_Lobby_Main) vTaskSuspend(Draw_Lobby_Main);
                    if (Draw_Lobby_Cheat) vTaskSuspend(Draw_Lobby_Cheat);
                    if (Draw_Lobby_Highscore) vTaskSuspend(Draw_Lobby_Highscore);
                    if (Game_Handler) vTaskSuspend(Game_Handler);
                    if (Swap_Invaders) vTaskSuspend(Swap_Invaders);
                    if (Let_Alien_Shoot) vTaskSuspend(Let_Alien_Shoot);
                    if (Draw_Game) vTaskSuspend(Draw_Game);
                    if(AI_control()) if (UDPControlTask) vTaskResume(UDPControlTask);
                    if (Init_Game) vTaskResume(Init_Game);
                    state_changed = 0;

                    break;


                case STATE_FIVE:
                	give_all_sempaphores();
                	prints("hey i am in state 5 (gamestate)\n");
                	// fflush(stdout);
//                	if (Init_Game) vTaskSuspend(Init_Game);
                    if (Draw_Lobby_Main) vTaskSuspend(Draw_Lobby_Main);
                    if (Draw_Lobby_Cheat) vTaskSuspend(Draw_Lobby_Cheat);
                    if (Draw_Lobby_Highscore) vTaskSuspend(Draw_Lobby_Highscore);
                    if (Draw_Game) vTaskResume(Draw_Game);
                    if (Game_Handler) vTaskResume(Game_Handler);
                    if (Swap_Invaders) vTaskResume(Swap_Invaders);
                    if (Let_Alien_Shoot) vTaskResume(Let_Alien_Shoot);

                    if(AI_control()) if (UDPControlTask) vTaskResume(UDPControlTask);

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



void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static – otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

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

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static – otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

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


#define FPS_AVERAGE_COUNT 50
#define FPS_FONT "IBMPlexSans-Bold.ttf"

void vDrawFPS(void)
{
    static unsigned int periods[FPS_AVERAGE_COUNT] = { 0 };
    static unsigned int periods_total = 0;
    static unsigned int index = 0;
    static unsigned int average_count = 0;
    static TickType_t xLastWakeTime = 0, prevWakeTime = 0;
    static char str[10] = { 0 };
    static int text_width;
    int fps = 0;
    font_handle_t cur_font = tumFontGetCurFontHandle();

    xLastWakeTime = xTaskGetTickCount();

    if (prevWakeTime != xLastWakeTime) {
        periods[index] =
            configTICK_RATE_HZ / (xLastWakeTime - prevWakeTime);
        prevWakeTime = xLastWakeTime;
    }
    else {
        periods[index] = 0;
    }

    periods_total += periods[index];

    if (index == (FPS_AVERAGE_COUNT - 1)) {
        index = 0;
    }
    else {
        index++;
    }

    if (average_count < FPS_AVERAGE_COUNT) {
        average_count++;
    }
    else {
        periods_total -= periods[index];
    }

    fps = periods_total / average_count;

    tumFontSelectFontFromName(FPS_FONT);

    sprintf(str, "FPS: %2d", fps);

    if (!tumGetTextSize((char *)str, &text_width, NULL))
        checkDraw(tumDrawText(str, SCREEN_WIDTH - text_width - 10,
                              SCREEN_HEIGHT - DEFAULT_FONT_SIZE * 1.5,
                              Skyblue),
                  __FUNCTION__);

    tumFontSelectFontFromHandle(cur_font);
    tumFontPutFontHandle(cur_font);
}

void checkButton_P(unsigned char * keycodeP_last, const unsigned char go_to){
	if (*keycodeP_last != buttons.buttons[KEYCODE(P)])
	{
		if(buttons.buttons[KEYCODE(P)])	if (StateQueue) xQueueSend(StateQueue, &go_to, 0);
		*keycodeP_last = buttons.buttons[KEYCODE(P)];
	}
}

void checkButton_P_game(unsigned char * keycodeP_last, unsigned char* paused){
	if (*keycodeP_last != buttons.buttons[KEYCODE(P)])
	{
		if(buttons.buttons[KEYCODE(P)])
		{
			if(*paused == 0)
			{
		        if (Swap_Invaders) vTaskSuspend(Swap_Invaders);
		        if (Game_Handler) vTaskSuspend(Game_Handler);
		        if (UDPControlTask) vTaskSuspend(UDPControlTask);

		    	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
		    	{
		    		invaders.paused = 1;
		    		xSemaphoreGive(invaders.lock);
		    	}

		        *paused = 1;

			}
			else if(*paused == 1)
			{
                if (Game_Handler) vTaskResume(Game_Handler);
                if (Swap_Invaders) vTaskResume(Swap_Invaders);
                if (UDPControlTask) vTaskResume(UDPControlTask);

            	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
            	{
            		invaders.resume = 1;
            		invaders.paused = 0;
            		xSemaphoreGive(invaders.lock);
            	}
                *paused = 0;
			}
		}

		*keycodeP_last = buttons.buttons[KEYCODE(P)];
	}
}

void checkButton_C(unsigned char * keycodeC_last, const unsigned char go_to){
	if (*keycodeC_last != buttons.buttons[KEYCODE(C)])
	{
		if(buttons.buttons[KEYCODE(C)])	if (StateQueue) xQueueSend(StateQueue, &go_to, 0);
		*keycodeC_last = buttons.buttons[KEYCODE(C)];
	}
}

void checkButton_A(unsigned char * keycodeA_last){
	if (*keycodeA_last != buttons.buttons[KEYCODE(A)])
	{
		if(buttons.buttons[KEYCODE(A)])
		{
			if (xSemaphoreTake(mothership.lock, portMAX_DELAY) == pdTRUE)
			{
				mothership.AI_control = (mothership.AI_control + 1) % 2;
				xSemaphoreGive(mothership.lock);
			}
		}
		*keycodeA_last = buttons.buttons[KEYCODE(A)];
	}
}

void checkButton_H(unsigned char * keycodeH_last, const unsigned char go_to){
	if (*keycodeH_last != buttons.buttons[KEYCODE(H)])
	{
		if(buttons.buttons[KEYCODE(H)])	if (StateQueue) xQueueSend(StateQueue, &go_to, 0);
		*keycodeH_last = buttons.buttons[KEYCODE(H)];
	}
}

void checkButton_B(unsigned char * keycodeB_last, const unsigned char go_to){
	if (*keycodeB_last != buttons.buttons[KEYCODE(B)])
	{

		if(buttons.buttons[KEYCODE(B)])	if (StateQueue) xQueueSend(StateQueue, &go_to, 0);
		*keycodeB_last = buttons.buttons[KEYCODE(B)];
	}
}

void checkButton_L(unsigned char * keycodeL_last){
	if (*keycodeL_last != buttons.buttons[KEYCODE(L)])
	{

		if(buttons.buttons[KEYCODE(L)])
		{
			if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
			{
				game_wrapper.infinite_life_flag = (game_wrapper.infinite_life_flag + 1) % 2;
				xSemaphoreGive(game_wrapper.lock);
			}
		}
		*keycodeL_last = buttons.buttons[KEYCODE(L)];
	}
}

void checkButton_T(unsigned char * keycodeT_last){
	if (*keycodeT_last != buttons.buttons[KEYCODE(T)])
	{

		if(buttons.buttons[KEYCODE(T)])
		{
			set_score_flag(1);
		}
		*keycodeT_last = buttons.buttons[KEYCODE(T)];
	}
}

void checkButton_K(unsigned char * keycodeK_last){
	if (*keycodeK_last != buttons.buttons[KEYCODE(K)])
	{

		if(buttons.buttons[KEYCODE(K)])
		{
			set_score_flag(2);
		}
		*keycodeK_last = buttons.buttons[KEYCODE(K)];
	}
}

void checkButton_U(unsigned char * keycodeU_last){
	if (*keycodeU_last != buttons.buttons[KEYCODE(U)])
	{

		if(buttons.buttons[KEYCODE(U)])
		{
			set_score_flag(3);
		}
		*keycodeU_last = buttons.buttons[KEYCODE(U)];
	}
}



void checkButton_LEFT(unsigned char * keycodeLEFT_last, const unsigned char move, const unsigned char stop){
	if (*keycodeLEFT_last != buttons.buttons[KEYCODE(LEFT)])
	{
		if(buttons.buttons[KEYCODE(LEFT)] == 1)
		{
			if (PlayerQueue) xQueueSend(PlayerQueue, &move, 0);
			*keycodeLEFT_last = buttons.buttons[KEYCODE(LEFT)];
		}
		else if(buttons.buttons[KEYCODE(LEFT)] == 0)
		{
			if (PlayerQueue) xQueueSend(PlayerQueue, &stop, 0);
			*keycodeLEFT_last = buttons.buttons[KEYCODE(LEFT)];
		}
	}
}

void checkButton_RIGHT(unsigned char * keycodeRIGHT_last, const unsigned char move, const unsigned char stop){
	if (*keycodeRIGHT_last != buttons.buttons[KEYCODE(RIGHT)])
	{
		if(buttons.buttons[KEYCODE(RIGHT)] == 1)
		{
			if (PlayerQueue) xQueueSend(PlayerQueue, &move, 0);
			*keycodeRIGHT_last = buttons.buttons[KEYCODE(RIGHT)];
		}
		else if(buttons.buttons[KEYCODE(RIGHT)] == 0)
		{
			if (PlayerQueue) xQueueSend(PlayerQueue, &stop, 0);
			*keycodeRIGHT_last = buttons.buttons[KEYCODE(RIGHT)];
		}
	}
}

void checkButton_UP(unsigned char * keycodeUP_last){
	if (*keycodeUP_last != buttons.buttons[KEYCODE(UP)])
	{
		if(buttons.buttons[KEYCODE(UP)] == 1)
		{
			increment_level();
		}
		*keycodeUP_last = buttons.buttons[KEYCODE(UP)];
	}
}

void checkButton_DOWN(unsigned char * keycodeDOWN_last){
	if (*keycodeDOWN_last != buttons.buttons[KEYCODE(DOWN)])
	{
		if(buttons.buttons[KEYCODE(DOWN)] == 1)
		{
			decrement_level();
		}
		*keycodeDOWN_last = buttons.buttons[KEYCODE(DOWN)];

	}
}

void checkButton_SPACE(unsigned char * keycodeSPACE_last, const unsigned char event){
	if (*keycodeSPACE_last != buttons.buttons[KEYCODE(SPACE)])
	{
		if(buttons.buttons[KEYCODE(SPACE)])	if (PlayerQueue) xQueueSend(PlayerQueue, &event, 0);
		*keycodeSPACE_last = buttons.buttons[KEYCODE(SPACE)];
	}
}


#define eSTATE_RUNNING 2
#define eSTATE_SUSPENDED 4

void vbuttonInput(void *pvParameters){

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



	while(1)
	{
		xGetButtonInput();
		currentState = xGetState();

		if(currentState != lastState)
		{
			prints("Current State: %d\n", currentState + 1);
			// fflush(stdout);
			lastState = currentState;
		}


        switch (currentState) {
            case STATE_ONE:
        		if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE)
        		{
        			checkButton_P(&keycodeP_last, four_state_signal);
        			checkButton_C(&keycodeC_last, two_state_signal);
        			checkButton_H(&keycodeH_last, three_state_signal);
        			checkButton_A(&keycodeA_last);

        			xSemaphoreGive(buttons.lock);
        		}
                break;

            case STATE_TWO:
        		if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE)
        		{
        			checkButton_P(&keycodeP_last, four_state_signal);
        			checkButton_B(&keycodeB_last, one_state_signal);
        			checkButton_L(&keycodeL_last);
        			checkButton_T(&keycodeT_last);
        			checkButton_K(&keycodeK_last);
        			checkButton_U(&keycodeU_last);
        			checkButton_DOWN(&keycodeDOWN_last);
        			checkButton_UP(&keycodeUP_last);

        			xSemaphoreGive(buttons.lock);
        		}
        		break;

            case STATE_THREE:
        		if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE)
        		{
        			checkButton_B(&keycodeB_last, one_state_signal);

        			xSemaphoreGive(buttons.lock);
        		}
            	break;

            case STATE_FOUR:
            	break;

            case STATE_FIVE:
        		if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE)
        		{
        			checkButton_B(&keycodeB_last, one_state_signal);
        			checkButton_P_game(&keycodeP_last, &paused);

        			if(!paused)
        			{
            			checkButton_LEFT(&keycodeLEFT_last, move_left_signal, stop_signal);
            			checkButton_RIGHT(&keycodeRIGHT_last, move_right_signal, stop_signal);
            			checkButton_SPACE(&keycodeSPACE_last, shoot_signal);
        			}

        			xSemaphoreGive(buttons.lock);
        		}
            	break;

            default:
                break;

        }

		vTaskDelay(50);
	}

}




void vDraw_Lobby_Main(void *pvParameters){

	unsigned char mothership_AI_control = 0;

	static char spaceInvaders_string[100];
	static int spaceInvaders_string_width = 0;
	static char play_string[100];
	static int play_string_width = 0;
	static char cheat_string[100];
	static int cheat_string_width = 0;
	static char highscore_string[100];
	static int highscore_string_width = 0;
	static char two_player_mode_string[100];
	static int two_player_mode_string_width = 0;


	sprintf(spaceInvaders_string, "SPACE INVADERS");
	sprintf(play_string, "PLAY [P]");
	sprintf(cheat_string, "CHEAT [C]");
	sprintf(highscore_string, "HIGHSCORE [H]");
	sprintf(two_player_mode_string, "TWO PLAYER (AI-MODE) [A]");

	// create buttons
    my_square_t* play_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*3/7 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);
    my_square_t* cheat_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*4/7 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);
    my_square_t* highscore_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*5/7 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);
    my_square_t* two_player_mode_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*6/7 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);


	while(1){

		// draw
		if (DrawSignal)
			if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
			{
				xSemaphoreTake(ScreenLock, portMAX_DELAY);

				checkDraw(tumDrawClear(White), __FUNCTION__); 	// Clear screen

				if (xSemaphoreTake(mothership.lock, portMAX_DELAY) == pdTRUE)
				{

					mothership_AI_control = mothership.AI_control;

					xSemaphoreGive(mothership.lock);
				}

//				// draw static text and buttons
//				drawText_State1(play_button, cheat_button, highscore_button);
				// draw button fields
				if (!tumDrawFilledBox(play_button->x_pos, play_button->y_pos, play_button->width, play_button->height, play_button->color)){} //Draw Box.
				if (!tumDrawFilledBox(cheat_button->x_pos, cheat_button->y_pos, cheat_button->width, cheat_button->height, cheat_button->color)){} //Draw Box.
				if (!tumDrawFilledBox(highscore_button->x_pos, highscore_button->y_pos, highscore_button->width, highscore_button->height, highscore_button->color)){} //Draw Box.

				if(mothership_AI_control)
				{
					if (!tumDrawFilledBox(two_player_mode_button->x_pos, two_player_mode_button->y_pos, two_player_mode_button->width, two_player_mode_button->height, Green)){} //Draw Box.
				}
				else{
					if (!tumDrawFilledBox(two_player_mode_button->x_pos, two_player_mode_button->y_pos, two_player_mode_button->width, two_player_mode_button->height, Black)){} //Draw Box.

				}

				// draw button text
				if (!tumGetTextSize((char *)spaceInvaders_string,&spaceInvaders_string_width, NULL))
					tumDrawText(spaceInvaders_string,SCREEN_WIDTH/2-spaceInvaders_string_width/2,
								SCREEN_HEIGHT/4-DEFAULT_FONT_SIZE /2, Black);
				if (!tumGetTextSize((char *)play_string,&play_string_width, NULL))
					tumDrawText(play_string,play_button->x_pos + LOBBY_BUTTON_WIDTH/2-play_string_width/2,
								play_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2, White);
				if (!tumGetTextSize((char *)cheat_string,&cheat_string_width, NULL))
					tumDrawText(cheat_string,cheat_button->x_pos + LOBBY_BUTTON_WIDTH/2-cheat_string_width/2,
								cheat_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2, White);
				if (!tumGetTextSize((char *)highscore_string,&highscore_string_width, NULL))
					tumDrawText(highscore_string,highscore_button->x_pos + LOBBY_BUTTON_WIDTH/2-highscore_string_width/2,
								highscore_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE/2, White);
				if (!tumGetTextSize((char *)two_player_mode_string,&two_player_mode_string_width, NULL))
					tumDrawText(two_player_mode_string,two_player_mode_button->x_pos + LOBBY_BUTTON_WIDTH/2-two_player_mode_string_width/2,
								two_player_mode_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE/2, White);

				xSemaphoreGive(ScreenLock);
			}
		vTaskDelay((TickType_t)100); // Basic sleep of 100ms

	}

}



void vDraw_Lobby_Cheat(void *pvParameters){

	unsigned char game_wrapper_infinite_life_flag = 0;
	unsigned char game_wrapper_set_score_flag = 0;

	short game_wrapper_level = 0;

	static char cheatMode_string[100];
	static int cheatMode_string_width = 0;
	static char inflife_string[100];
	static int inflife_string_width = 0;
	static char setscore_string[100];
	static int setscore_string_width = 0;
	static char setlevel_string[100];
	static int setlevel_string_width = 0;
	static char one_hundred_string[100];
	static int one_hundred_string_width = 0;
	static char one_thousand_string[100];
	static int one_thousand_string_width = 0;
	static char ten_thousand_string[100];
	static int ten_thousand_string_width = 0;
	static char play_string[100];
	static int play_string_width = 0;
	static char back_string[100];
	static int back_string_width = 0;

	sprintf(one_hundred_string, "100 [T]");
	sprintf(one_thousand_string, "1000 [K]");
	sprintf(ten_thousand_string, "10000 [U]");
	sprintf(setscore_string, "SET SCORE:");
	sprintf(inflife_string, "INFINITE LIFES [L]");
	sprintf(cheatMode_string, "CHEAT MODE");
	sprintf(play_string, "PLAY [P]");
	sprintf(back_string, "BACK [B]");

	// create buttons
	my_square_t* setscore_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*2/9 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT *2 ,Grey);
	my_square_t* inflife_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*5/9 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Grey);
    my_square_t* play_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*7/9 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);
    my_square_t* back_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*8/9 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);
    my_square_t* setlevel_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*4/9 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT, Grey);



	while(1){

		if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
		{

			game_wrapper_level = game_wrapper.level;

			xSemaphoreGive(game_wrapper.lock);
		}

		sprintf(setlevel_string, "SET LEVEL: %d [up] [down]", game_wrapper_level + 1);

		//draw
		if (DrawSignal)
			if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
			{
				xSemaphoreTake(ScreenLock, portMAX_DELAY);

				checkDraw(tumDrawClear(White), __FUNCTION__); 	// Clear screen

				get_game_wrapper_flags(&game_wrapper_infinite_life_flag, &game_wrapper_set_score_flag);

				if (!tumDrawFilledBox(play_button->x_pos, play_button->y_pos, play_button->width, play_button->height, play_button->color)){} //Draw Box.
				if (!tumDrawFilledBox(back_button->x_pos, back_button->y_pos, back_button->width, back_button->height, back_button->color)){} //Draw Box.
				if (!tumDrawFilledBox(setscore_button->x_pos, setscore_button->y_pos, setscore_button->width, setscore_button->height, setscore_button->color)){} //Draw Box.

				if(game_wrapper_level == 0)
				{
					if (!tumDrawFilledBox(setlevel_button->x_pos, setlevel_button->y_pos, setlevel_button->width, setlevel_button->height, Grey)){} //Draw Box.
				}
				else
				{
					if (!tumDrawFilledBox(setlevel_button->x_pos, setlevel_button->y_pos, setlevel_button->width, setlevel_button->height, Green)){} //Draw Box.
				}


				if(game_wrapper_infinite_life_flag)
				{
					if (!tumDrawFilledBox(inflife_button->x_pos, inflife_button->y_pos, inflife_button->width, inflife_button->height, Green)){} //Draw Box.
				}
				else
				{
					if (!tumDrawFilledBox(inflife_button->x_pos, inflife_button->y_pos, inflife_button->width, inflife_button->height, inflife_button->color)){} //Draw Box.
				}


				if (!tumGetTextSize((char *)cheatMode_string,&cheatMode_string_width, NULL))
					tumDrawText(cheatMode_string,SCREEN_WIDTH/2-cheatMode_string_width/2,
								SCREEN_HEIGHT*1/9-DEFAULT_FONT_SIZE /2, Black);


				unsigned int color_100 = White;
				unsigned int color_1000 = White;
				unsigned int color_10000 = White;

				if(game_wrapper_set_score_flag == 1)
				{
					color_100 = Green;
				}
				if(game_wrapper_set_score_flag == 2)
				{
					color_1000 = Green;
				}
				if(game_wrapper_set_score_flag == 3)
				{
					color_10000 = Green;
				}

				if (!tumGetTextSize((char *)one_hundred_string,&one_hundred_string_width, NULL))
					tumDrawText(one_hundred_string,setscore_button->x_pos + LOBBY_BUTTON_WIDTH*1/4 - one_hundred_string_width/2,
								setscore_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2 + LOBBY_BUTTON_HEIGHT*2/3, color_100);

				if (!tumGetTextSize((char *)one_thousand_string,&one_thousand_string_width, NULL))
					tumDrawText(one_thousand_string,setscore_button->x_pos + LOBBY_BUTTON_WIDTH*2/4 - one_thousand_string_width/2,
								setscore_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2 + LOBBY_BUTTON_HEIGHT*2/3, color_1000);

				if (!tumGetTextSize((char *)ten_thousand_string,&ten_thousand_string_width, NULL))
					tumDrawText(ten_thousand_string,setscore_button->x_pos + LOBBY_BUTTON_WIDTH*3/4 - ten_thousand_string_width/2,
								setscore_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2 + LOBBY_BUTTON_HEIGHT*2/3, color_10000);


				if (!tumGetTextSize((char *)setscore_string,&setscore_string_width, NULL))
					tumDrawText(setscore_string,setscore_button->x_pos + LOBBY_BUTTON_WIDTH/2-setscore_string_width/2,
							setscore_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2, White);

				if (!tumGetTextSize((char *)setlevel_string,&setlevel_string_width, NULL))
					tumDrawText(setlevel_string,setlevel_button->x_pos + LOBBY_BUTTON_WIDTH/2-setlevel_string_width/2,
							setlevel_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2, White);

				if (!tumGetTextSize((char *)inflife_string,&inflife_string_width, NULL))
					tumDrawText(inflife_string,inflife_button->x_pos + LOBBY_BUTTON_WIDTH/2-inflife_string_width/2,
							inflife_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2, White);

				if (!tumGetTextSize((char *)play_string,&play_string_width, NULL))
					tumDrawText(play_string,play_button->x_pos + LOBBY_BUTTON_WIDTH/2-play_string_width/2,
								play_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2, White);

				if (!tumGetTextSize((char *)back_string,&back_string_width, NULL))
					tumDrawText(back_string,back_button->x_pos + LOBBY_BUTTON_WIDTH/2-back_string_width/2,
								back_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2, White);

				xSemaphoreGive(ScreenLock);
			}
		vTaskDelay((TickType_t)100); // Basic sleep of 100ms

	}

}


void vDraw_Lobby_Highscore(void *pvParameters){

	static char highscoreText_string[100];
	static int highscoreText_string_width = 0;
	static char back_string[100];
	static int back_string_width = 0;
	static char highscore_string[100];
	static int highscore_string_width = 0;

	short highscore = 0;

	sprintf(highscoreText_string, "HIGHSCORE");
	sprintf(back_string, "BACK [B]");

	// draw back button
    my_square_t* back_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2,  SCREEN_HEIGHT*5/6 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);


	while(1){

		get_highscore(&highscore);
		sprintf(highscore_string, "%d", highscore);

		if (DrawSignal)
			if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
			{
				xSemaphoreTake(ScreenLock, portMAX_DELAY);

				checkDraw(tumDrawClear(White), __FUNCTION__); 	// Clear screen

				if (!tumDrawFilledBox(back_button->x_pos, back_button->y_pos, back_button->width, back_button->height, back_button->color)){} //Draw Box.

				int font_size =  40;

				if (!tumGetTextSize((char *)highscoreText_string,&highscoreText_string_width, &font_size))
					tumDrawText(highscoreText_string,SCREEN_WIDTH/2-highscoreText_string_width/2,
								SCREEN_HEIGHT/4-DEFAULT_FONT_SIZE /2, Black);

				if (!tumGetTextSize((char *)highscore_string,&highscore_string_width, NULL))
					tumDrawText(highscore_string, SCREEN_WIDTH/2-highscore_string_width/2,
								SCREEN_HEIGHT*3/6- DEFAULT_FONT_SIZE/2, Black);

				if (!tumGetTextSize((char *)back_string,&back_string_width, NULL))
					tumDrawText(back_string,back_button->x_pos + LOBBY_BUTTON_WIDTH/2-back_string_width/2,
								back_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2, White);

				xSemaphoreGive(ScreenLock);
			}
		vTaskDelay((TickType_t)100); // Basic sleep of 100ms

	}

}

void draw_score()
{
	static char scoreText_string[100];
	static int scoreText_string_width = 0;

	// get score
	if (xSemaphoreTake(game_wrapper.lock, 0) == pdTRUE)
	{
		sprintf(scoreText_string, "Score: %d", game_wrapper.score);
		xSemaphoreGive(game_wrapper.lock);
	}

	if (!tumGetTextSize((char *)scoreText_string, &scoreText_string_width, NULL))
		tumDrawText(scoreText_string, SCREEN_WIDTH/2 - scoreText_string_width/2, SCREEN_HEIGHT/20 - DEFAULT_FONT_SIZE /2, White);
}

void draw_pause_resume()
{
	static char pause_resumeText_string[100];
	static int pause_resumeText_string_width = 0;

	// get score
	if (xSemaphoreTake(invaders.lock, 0) == pdTRUE)
	{
		if(invaders.paused)
		{
			sprintf(pause_resumeText_string, "RESUME [P]");
		}
		else
		{
			sprintf(pause_resumeText_string, "PAUSE [P]");
		}

		xSemaphoreGive(invaders.lock);
	}

	if (!tumGetTextSize((char *)pause_resumeText_string, &pause_resumeText_string_width, NULL))
		tumDrawText(pause_resumeText_string, SCREEN_WIDTH*9/10 - pause_resumeText_string_width/2, SCREEN_HEIGHT/20 - DEFAULT_FONT_SIZE /2, White);
}

void draw_quit()
{
	static char quitText_string[100];
	static int quitText_string_width = 0;


	sprintf(quitText_string, "RESET [B]");


	if (!tumGetTextSize((char *)quitText_string, &quitText_string_width, NULL))
		tumDrawText(quitText_string, SCREEN_WIDTH*9/10 - quitText_string_width/2, SCREEN_HEIGHT*2/20 - DEFAULT_FONT_SIZE /2, White);
}

void draw_level()
{
	static char levelText_string[100];
	static int levelText_string_width = 0;

	// get score
	if (xSemaphoreTake(game_wrapper.lock, 0) == pdTRUE)
	{
		sprintf(levelText_string, "Level: %d", game_wrapper.level + 1);
		xSemaphoreGive(game_wrapper.lock);
	}

	if (!tumGetTextSize((char *)levelText_string, &levelText_string_width, NULL))
		tumDrawText(levelText_string, SCREEN_WIDTH/10 - levelText_string_width/2, SCREEN_HEIGHT/20 - DEFAULT_FONT_SIZE /2, White);
}


void draw_lifes(my_square_t* life_shape)
{
	// get lifes
	if (xSemaphoreTake(game_wrapper.lock, 0) == pdTRUE)
	{
		for(unsigned char u = 0; u < game_wrapper.remaining_life; u++)
		{
			if (!tumDrawFilledBox( SCREEN_WIDTH/30 - LIFE_SIZE_X/2 + u * (LIFE_SIZE_X/2 + LIFE_SIZE_DISTANCE),  SCREEN_HEIGHT*29/30 - LIFE_SIZE_Y/2 , LIFE_SIZE_X, LIFE_SIZE_Y, Red)){}
		}

		xSemaphoreGive(game_wrapper.lock);
	}
}

void check_swap_invaders(unsigned char *swap_state)
{
	unsigned char input = 0;

	if (SwapInvadersQueue) if (xQueueReceive(SwapInvadersQueue, &input, 0) == pdTRUE)
	{
		if(input == 1)
		{
			*swap_state = (*swap_state + input) % 2;
			input = 0;
		}
	}
}

void draw_Alien(unsigned char * swap_state, my_square_t* alien_shape, short invaders_pos_x, short invaders_pos_y, unsigned char i, unsigned char j, image_handle_t alien_1_1_image, image_handle_t alien_1_2_image, image_handle_t alien_2_1_image, image_handle_t alien_2_2_image, image_handle_t alien_3_1_image, image_handle_t alien_3_2_image)
{
	unsigned int color_state_1[3] = {Red, Blue, Yellow};
	unsigned int color_state_2[3] = {Pink, Fuchsia, Lime};
	unsigned int color[3] = {0};

	image_handle_t alien_type_1[3] = {alien_1_1_image, alien_2_1_image, alien_3_1_image};
	image_handle_t alien_type_2[3] = {alien_1_2_image, alien_2_2_image, alien_3_2_image};
	image_handle_t alien_type[3] = {0};


	if(*swap_state)
	{
		for(unsigned char u = 0; u < 3; u++)
		{
			color[u] = color_state_1[u];
			alien_type[u] = alien_type_1[u];
		}

	}
	else
	{
		for(unsigned char u = 0; u < 3; u++)
		{
			color[u] = color_state_2[u];
			alien_type[u] = alien_type_2[u];
		}
	}

	if(i == 0)
	{
		if(tumDrawLoadedImage(alien_type[0], invaders_pos_x + ALIEN_DISTANCE*j, invaders_pos_y + ALIEN_DISTANCE*i))
		{
			if (!tumDrawFilledBox( invaders_pos_x + ALIEN_DISTANCE*j,  invaders_pos_y + ALIEN_DISTANCE*i , alien_shape->width, alien_shape->height, color[0])){}
		}

	}
	else if(i < 3 && i > 0)
	{
		if(tumDrawLoadedImage(alien_type[1], invaders_pos_x + ALIEN_DISTANCE*j, invaders_pos_y + ALIEN_DISTANCE*i ))
		{
			if (!tumDrawFilledBox( invaders_pos_x + ALIEN_DISTANCE*j,  invaders_pos_y + ALIEN_DISTANCE*i , alien_shape->width, alien_shape->height, color[1])){}
		}
	}
	else if(i < NUMBER_OF_ALIENS_Y && i > 2)
	{
		if(tumDrawLoadedImage(alien_type[2],invaders_pos_x + ALIEN_DISTANCE*j, invaders_pos_y + ALIEN_DISTANCE*i))
		{
			if (!tumDrawFilledBox( invaders_pos_x + ALIEN_DISTANCE*j,  invaders_pos_y + ALIEN_DISTANCE*i , alien_shape->width, alien_shape->height, color[2])){}
		}
	}

}

void draw_Invaders(unsigned char *swap_state, my_square_t* alien_shape, image_handle_t alien_1_1_image, image_handle_t alien_1_2_image, image_handle_t alien_2_1_image, image_handle_t alien_2_2_image, image_handle_t alien_3_1_image, image_handle_t alien_3_2_image)
{
	short invaders_pos_x = invaders.pos_x;
	short invaders_pos_y = invaders.pos_y;

	// get position of invaders
	if (xSemaphoreTake(invaders.lock, 0) == pdTRUE)
	{
		// draw Invaders
		for (unsigned char i = 0; i < NUMBER_OF_ALIENS_Y; i++)
		{
			for (unsigned char j = 0; j < NUMBER_OF_ALIENS_X; j++)
			{
				// if Alien is alive
				if (invaders.enemy[i][j].alive)
				{

					check_swap_invaders(swap_state);

					draw_Alien(swap_state, alien_shape, invaders_pos_x, invaders_pos_y, i, j, alien_1_1_image, alien_1_2_image, alien_2_1_image, alien_2_2_image, alien_3_1_image, alien_3_2_image);

				}

			}
		}
		xSemaphoreGive(invaders.lock);
	}
}

void draw_player_bullet(unsigned char player_bullet_alive, my_square_t* player_bullet_shape)
{
	// draw bullet
	if (player_bullet_alive)
	{
		if (!tumDrawFilledBox(player_bullet_shape->x_pos, player_bullet_shape->y_pos, player_bullet_shape->width, player_bullet_shape->height, player_bullet_shape->color)){}
	}
}

void draw_aliens_bullet(unsigned char aliens_bullet_alive, my_square_t* aliens_bullet_shape, image_handle_t alien_bullet_image)
{
	// draw bullet
	if (aliens_bullet_alive)
	{
		if(tumDrawLoadedImage(alien_bullet_image, aliens_bullet_shape->x_pos, aliens_bullet_shape->y_pos))
		{
			if (!tumDrawFilledBox(aliens_bullet_shape->x_pos, aliens_bullet_shape->y_pos, aliens_bullet_shape->width, aliens_bullet_shape->height, aliens_bullet_shape->color)){}
		}
	}
}



void draw_player(my_square_t* player_shape, image_handle_t player_image )
{
	// draw player
	if(tumDrawLoadedImage(player_image, player_shape->x_pos, player_shape->y_pos))
	{
		if (!tumDrawFilledBox(player_shape->x_pos, player_shape->y_pos, player_shape->width, player_shape->height, player_shape->color)){}
	}

}

void draw_mothership(unsigned char mothership_alive, my_square_t* mothership_shape, image_handle_t mothership_image )
{
	if(mothership_alive)
	{
		// draw player
		if(tumDrawLoadedImage(mothership_image, mothership_shape->x_pos, mothership_shape->y_pos))
		{
			if (!tumDrawFilledBox(mothership_shape->x_pos, mothership_shape->y_pos, mothership_shape->width, mothership_shape->height, mothership_shape->color)){}
		}
	}


}

void get_player_bullet_position(my_square_t* player_shape, my_square_t* player_bullet_shape, unsigned char *player_bullet_alive)
{
	// get position of player_x, bullet, bullet_alive
	if (xSemaphoreTake(player.lock, 0) == pdTRUE)
	{
	    	player_shape->x_pos = player.pos_x;

	    	if(player.bullet.alive == 1)
	    	{
				player_bullet_shape->x_pos = player.bullet.pos_x;
				player_bullet_shape->y_pos = player.bullet.pos_y;
	    	}
	    	*player_bullet_alive = player.bullet.alive;

		xSemaphoreGive(player.lock);
	}
}

void get_aliens_bullet_position(my_square_t* aliens_bullet_shape, unsigned char *aliens_bullet_alive)
{
	// get position of player_x, bullet, bullet_alive
	if (xSemaphoreTake(invaders.lock, 0) == pdTRUE)
	{
		if(invaders.bullet.alive == 1)
		{
			aliens_bullet_shape->x_pos = invaders.bullet.pos_x;
			aliens_bullet_shape->y_pos = invaders.bullet.pos_y;
		}
		*aliens_bullet_alive = invaders.bullet.alive;

		xSemaphoreGive(invaders.lock);
	}
}

void get_mothership_position(my_square_t* mothership_shape, unsigned char *mothership_alive)
{
	if (xSemaphoreTake(mothership.lock, 0) == pdTRUE)
	{
		mothership_shape->x_pos = mothership.pos_x;
		*mothership_alive = mothership.alive;
		xSemaphoreGive(mothership.lock);
	}
}

void draw_bunker(my_square_t* bunker_block_shape, image_handle_t bunker_block_worse_image, image_handle_t bunker_block_bad_image, image_handle_t bunker_block_good_image )
{
	if (xSemaphoreTake(bunker.lock, 0) == pdTRUE)
	{
		// for each bunker
		for(short s = 0; s < 4; s++)
		{
			for(short u = 0; u < NUM_BUNKER_BLOCK_Y; u++)
			{
				for(short r = 0; r < NUM_BUNKER_BLOCK_X; r++)
				{

					switch (bunker.bunkers[s].block_destruction_state[u][r])
					{
					case 0:
						break;
					case 1:

						if(tumDrawLoadedImage(bunker_block_worse_image, bunker.bunkers[s].pos_x + BUNKER_BLOCK_SIZE_X * r, bunker.pos_y + BUNKER_BLOCK_SIZE_Y * u))
						{
							if (!tumDrawFilledBox(bunker.bunkers[s].pos_x + BUNKER_BLOCK_SIZE_X * r, bunker.pos_y + BUNKER_BLOCK_SIZE_Y * u, BUNKER_BLOCK_SIZE_X, BUNKER_BLOCK_SIZE_Y, Silver)){}
						}
						break;
					case 2:

						if( tumDrawLoadedImage(bunker_block_bad_image, bunker.bunkers[s].pos_x + BUNKER_BLOCK_SIZE_X * r, bunker.pos_y + BUNKER_BLOCK_SIZE_Y * u))
						{
							if (!tumDrawFilledBox(bunker.bunkers[s].pos_x + BUNKER_BLOCK_SIZE_X * r, bunker.pos_y + BUNKER_BLOCK_SIZE_Y * u, BUNKER_BLOCK_SIZE_X, BUNKER_BLOCK_SIZE_Y, Gray)){}
						}
						break;
					case 3:

						if(tumDrawLoadedImage(bunker_block_good_image, bunker.bunkers[s].pos_x + BUNKER_BLOCK_SIZE_X * r, bunker.pos_y + BUNKER_BLOCK_SIZE_Y * u))
						{
							if (!tumDrawFilledBox(bunker.bunkers[s].pos_x + BUNKER_BLOCK_SIZE_X * r, bunker.pos_y + BUNKER_BLOCK_SIZE_Y * u, BUNKER_BLOCK_SIZE_X, BUNKER_BLOCK_SIZE_Y, White)){}
						}
						break;
					default:
						break;
					}

				}

			}
		}
		xSemaphoreGive(bunker.lock);
	}
}


void vDraw_Game(void *pvParameters){

	unsigned char player_bullet_alive = 0;
	unsigned char aliens_bullet_alive = 0;
	unsigned char mothership_alive = 0;
	unsigned char swap_state = 0;


	// create player
    my_square_t* player_shape=create_rect(SCREEN_WIDTH/2 - PLAYER_SIZE_X/2,  SCREEN_HEIGHT*5/6 - PLAYER_SIZE_Y/2, PLAYER_SIZE_X, PLAYER_SIZE_Y, Green);
	// create mothership
    my_square_t* mothership_shape=create_rect(SCREEN_WIDTH/2 - MOTHERSHIP_SIZE_X/2,  MOTHERSHIP_POS_Y, MOTHERSHIP_SIZE_X, MOTHERSHIP_SIZE_Y, Red);
    // create player_bullet
    my_square_t* player_bullet_shape=create_rect(SCREEN_WIDTH/2 - BULLET_SIZE_X/2,  SCREEN_HEIGHT*5/6 - BULLET_SIZE_Y/2, BULLET_SIZE_X, BULLET_SIZE_Y, Green);
    // create aliens_bullet
    my_square_t* aliens_bullet_shape=create_rect(SCREEN_WIDTH/2 - BULLET_SIZE_X/2,  SCREEN_HEIGHT*5/6 - BULLET_SIZE_Y/2, BULLET_SIZE_X, BULLET_SIZE_Y, Red);
    // create Alien
    my_square_t* alien_shape=create_rect(SCREEN_WIDTH/2 - ALIEN_SIZE_X/2,  SCREEN_HEIGHT*5/6 - ALIEN_SIZE_Y/2, ALIEN_SIZE_X, ALIEN_SIZE_Y, Red);
    // create Bunker Part
    my_square_t* bunker_block_shape=create_rect(SCREEN_WIDTH/2 - BUNKER_BLOCK_SIZE_X/2,  SCREEN_HEIGHT*5/6 - BUNKER_BLOCK_SIZE_Y/2, BUNKER_BLOCK_SIZE_X, BUNKER_BLOCK_SIZE_Y, Red);
    // create life
    my_square_t* life_shape = create_rect(SCREEN_WIDTH/30 - LIFE_SIZE_X/2,  SCREEN_HEIGHT*29/30 - LIFE_SIZE_Y/2, LIFE_SIZE_X, LIFE_SIZE_Y, Red);


    image_handle_t player_image = tumDrawLoadImage("../resources/images/player.png");
    image_handle_t bunker_block_good_image = tumDrawLoadImage("../resources/images/bunker_block_good.png");
    image_handle_t bunker_block_bad_image = tumDrawLoadImage("../resources/images/bunker_block_bad.png");
    image_handle_t bunker_block_worse_image = tumDrawLoadImage("../resources/images/bunker_block_worse.png");

    image_handle_t alien_1_1_image = tumDrawLoadImage("../resources/images/alien_1_1.png");
    image_handle_t alien_1_2_image = tumDrawLoadImage("../resources/images/alien_1_2.png");
    image_handle_t alien_2_1_image = tumDrawLoadImage("../resources/images/alien_2_1.png");
    image_handle_t alien_2_2_image = tumDrawLoadImage("../resources/images/alien_2_2.png");
    image_handle_t alien_3_1_image = tumDrawLoadImage("../resources/images/alien_3_1.png");
    image_handle_t alien_3_2_image = tumDrawLoadImage("../resources/images/alien_3_2.png");
    image_handle_t alien_bullet_image = tumDrawLoadImage("../resources/images/alien_bullet.png");
    image_handle_t mothership_image = tumDrawLoadImage("../resources/images/mothership.png");

	while(1){

		if (DrawSignal)
			if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
			{
				get_player_bullet_position(player_shape, player_bullet_shape, &player_bullet_alive);
				get_aliens_bullet_position(aliens_bullet_shape, &aliens_bullet_alive);
				get_mothership_position(mothership_shape, &mothership_alive);

				xSemaphoreTake(ScreenLock, portMAX_DELAY);

				checkDraw(tumDrawClear(Black), __FUNCTION__); 	// Clear screen

				draw_bunker(bunker_block_shape, bunker_block_worse_image, bunker_block_bad_image, bunker_block_good_image);
				draw_Invaders(&swap_state, alien_shape, alien_1_1_image, alien_1_2_image, alien_2_1_image, alien_2_2_image, alien_3_1_image, alien_3_2_image);
           	    draw_player(player_shape, player_image);
           	    draw_mothership(mothership_alive, mothership_shape, mothership_image);
				draw_player_bullet(player_bullet_alive, player_bullet_shape);
				draw_aliens_bullet(aliens_bullet_alive, aliens_bullet_shape, alien_bullet_image);
				draw_score();
				draw_level();
				draw_pause_resume();
				draw_quit();
				draw_lifes(life_shape);

				xSemaphoreGive(ScreenLock);
			}

		vTaskDelay((TickType_t)50); // Basic sleep of 100ms
	}

}




void vSwap_Invaders(void *pvParameters)
{
	TickType_t xWaitingTime = 1000;

	unsigned char swap = 1;


	while(1)
	{
		if (xSemaphoreTake(invaders.lock, 0) == pdTRUE)
		{
			xWaitingTime = abs(round((1/invaders.speed) * 50 ));
			prints("Swarp Invaders frequency: %d, invaders speed: %f\n", xWaitingTime, invaders.speed);
			// fflush(stdout);
			xSemaphoreGive(invaders.lock);
		}

		if (SwapInvadersQueue) xQueueSend(SwapInvadersQueue, &swap, 0);


		vTaskDelay(pdMS_TO_TICKS(xWaitingTime));
	}
}


void vLet_Alien_Shoot(void *pvParameters)
{
	unsigned char shoot = 1;
	TickType_t xWaitingTime = 500;

	while(1)
	{
		if (AlienShootsQueue) xQueueSend(AlienShootsQueue, &shoot, 0);
		vTaskDelay(pdMS_TO_TICKS(xWaitingTime));
	}
}


void vSwapBuffers(void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    const TickType_t frameratePeriod = 19;

    tumDrawBindThread(); // Setup Rendering handle with correct GL context

    while (1) {

        if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE) {
        	vDrawFPS();
            tumDrawUpdateScreen();
            tumEventFetchEvents(FETCH_EVENT_BLOCK);
            xSemaphoreGive(ScreenLock);
            xSemaphoreGive(DrawSignal);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(frameratePeriod));
        }
    }
}



int main(int argc, char *argv[])
{
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

    // locks and semaphores
    buttons.lock = xSemaphoreCreateMutex(); // Button Locking mechanism
    if (!buttons.lock) {
        PRINT_ERROR("Failed to create buttons lock");
        goto err_buttons_lock;
    }
    state.lock = xSemaphoreCreateMutex(); 	//State Locking meachanism
    if(!state.lock) {
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
    if (!PlayerQueue) {			// State Queue
        PRINT_ERROR("Could not open Player Queue");
        goto err_PlayerQueue;
    }

    SwapInvadersQueue = xQueueCreate(SWAP_INVADERS_QUEUE_LENGTH, sizeof(unsigned char));
    if (!SwapInvadersQueue) {			// State Queue
        PRINT_ERROR("Could not open SwapInvadersQueue");
        goto err_SwapInvadersQueue;
    }
    AlienShootsQueue = xQueueCreate(ALIENS_SHOOTS_QUEUE_LENGTH, sizeof(unsigned char));
    if (!AlienShootsQueue) {			// State Queue
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
    if (xTaskCreate(vbuttonInput, "buttonInput",
                    mainGENERIC_STACK_SIZE * 2, NULL, configMAX_PRIORITIES-1,
					buttonInput) != pdPASS) {
        PRINT_TASK_ERROR("buttonInput Task");
        goto err_buttonInput;
    }





    if (xTaskCreate(vDraw_Lobby_Main, "Draw_Lobby_Main",
    						mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 3,
    						&Draw_Lobby_Main) != pdPASS) {
        PRINT_TASK_ERROR("Draw_Lobby_Main");
        goto err_Draw_Lobby_Main;
    }

    if (xTaskCreate(vDraw_Lobby_Cheat, "Draw_Lobby_Cheat",
    						mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 3,
    						&Draw_Lobby_Cheat) != pdPASS) {
        PRINT_TASK_ERROR("Draw_Lobby_Cheat");
        goto err_Draw_Lobby_Cheat;
    }

    if (xTaskCreate(vDraw_Lobby_Highscore, "Draw_Lobby_Highscore",
    						mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 3,
    						&Draw_Lobby_Highscore) != pdPASS) {
        PRINT_TASK_ERROR("Draw_Lobby_Highscore");
        goto err_Draw_Lobby_Highscore;
    }

    if (xTaskCreate(vDraw_Game, "Draw_Game",
    						mainGENERIC_STACK_SIZE * 3, NULL, mainGENERIC_PRIORITY + 3,
    						&Draw_Game) != pdPASS) {
        PRINT_TASK_ERROR("Draw_Game");
        goto err_Draw_Game;
    }



    if (xTaskCreate(vSwap_Invaders, "Swap_Invaders",
    						mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 1,
    						&Swap_Invaders) != pdPASS) {
        PRINT_TASK_ERROR("Swap_Invaders");
        goto err_Swap_Invaders;
    }

    if (xTaskCreate(vLet_Alien_Shoot, "Let_Alien_Shoot",
    						mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 1,
    						&Let_Alien_Shoot) != pdPASS) {
        PRINT_TASK_ERROR("Let_Alien_Shoot");
        goto err_Let_Alien_Shoot;
    }


    init_space_invaders_handler();


    vTaskSuspend(Draw_Lobby_Main);
    vTaskSuspend(Draw_Lobby_Cheat);
    vTaskSuspend(Draw_Lobby_Highscore);
    vTaskSuspend(Draw_Game);
    vTaskSuspend(Swap_Invaders);
    vTaskSuspend(Let_Alien_Shoot);


    vTaskStartScheduler();

    return EXIT_SUCCESS;



	vTaskDelete(Draw_Lobby_Main);
err_Draw_Lobby_Main:
	vTaskDelete(Draw_Lobby_Cheat);
err_Draw_Lobby_Cheat:
	vTaskDelete(Draw_Lobby_Highscore);
err_Draw_Lobby_Highscore:
	vTaskDelete(Draw_Game);
err_Draw_Game:
	vTaskDelete(Swap_Invaders);
err_Swap_Invaders:
	vTaskDelete(Let_Alien_Shoot);
err_Let_Alien_Shoot:
	vTaskDelete(BufferSwap);
err_bufferswap:
	vTaskDelete(buttonInput);
err_buttonInput:
	vTaskDelete(StateMachine);
err_statemachine:
	vQueueDelete(StateQueue);
err_state_queue:
	vQueueDelete(PlayerQueue);
err_PlayerQueue:
	vQueueDelete(SwapInvadersQueue);
err_SwapInvadersQueue:
	vQueueDelete(AlienShootsQueue);
err_AlienShootsQueue:
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
    tumEventExit();
err_init_events:
    tumDrawExit();
err_init_drawing:

    return EXIT_FAILURE;
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vMainQueueSendPassed(void)
{
    /* This is just an example implementation of the "queue send" trace hook. */
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vApplicationIdleHook(void)
{
#ifdef __GCC_POSIX__
    struct timespec xTimeToSleep, xTimeSlept;
    /* Makes the process more agreeable when using the Posix simulator. */
    xTimeToSleep.tv_sec = 1;
    xTimeToSleep.tv_nsec = 0;
    nanosleep(&xTimeToSleep, &xTimeSlept);
#endif
}
