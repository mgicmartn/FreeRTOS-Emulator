

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



TaskHandle_t DrawLobbyMainTask = NULL;
TaskHandle_t DrawLobbyCheatTask = NULL;
TaskHandle_t DrawLobbyHighscoreTask = NULL;
TaskHandle_t DrawGameTask = NULL;


void checkDraw(unsigned char status, const char *msg)
{
    if (status) {
        if (msg)
            fprints(stderr, "[ERROR] %s, %s\n", msg,
                    tumGetErrorMessage());
        else {
            fprints(stderr, "[ERROR] %s\n", tumGetErrorMessage());
        }
    }
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

void vDrawPopUpField(my_square_t* pop_up_page, unsigned char* next_state, short countdown )
{
	static char pop_up_page_string[100];
	static int pop_up_page_string_width = 0;

	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{
		sprintf(pop_up_page_string, "%s %d s !", game_wrapper.game_message, countdown);
		*next_state = game_wrapper.next_state;
	}
	xSemaphoreGive(game_wrapper.lock);

	tumDrawFilledBox(pop_up_page->x_pos, pop_up_page->y_pos, pop_up_page->width, pop_up_page->height, pop_up_page->color);

	if (!tumGetTextSize((char *)pop_up_page_string,&pop_up_page_string_width, NULL))
		tumDrawText(pop_up_page_string,pop_up_page->x_pos + POP_UP_PAGE_WIDTH/2-pop_up_page_string_width/2,
					pop_up_page->y_pos + POP_UP_PAGE_HEIGHT / 2 - DEFAULT_FONT_SIZE/2, White);
}



void vDrawPlayButton(my_square_t* play_button)
{
	static char play_string[100];
	static int play_string_width = 0;

	sprintf(play_string, "PLAY [P]");

	tumDrawFilledBox(play_button->x_pos, play_button->y_pos, play_button->width, play_button->height, play_button->color);

	if (!tumGetTextSize((char *)play_string,&play_string_width, NULL))
		tumDrawText(play_string,play_button->x_pos + LOBBY_BUTTON_WIDTH/2-play_string_width/2,
					play_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2, White);
}

void vDrawBackgroundImage(image_handle_t background_image)
{
	if(tumDrawLoadedImage(background_image, 0, 0))
	{
		static char background_string[100];
		static int background_string_width = 0;

		sprintf(background_string, "*** background image missing ***");

		checkDraw(tumDrawClear(White), __FUNCTION__); 	// Clear screen

		if (!tumGetTextSize((char *)background_string,&background_string_width, NULL))
			tumDrawText(background_string,SCREEN_WIDTH/2-background_string_width/2,
						SCREEN_HEIGHT*1/9-DEFAULT_FONT_SIZE /2, Black);
	}
}

void vDrawCheatButton(my_square_t* cheat_button)
{
	static char cheat_string[100];
	static int cheat_string_width = 0;

	sprintf(cheat_string, "CHEAT [C]");

	tumDrawFilledBox(cheat_button->x_pos, cheat_button->y_pos, cheat_button->width, cheat_button->height, cheat_button->color);

	if (!tumGetTextSize((char *)cheat_string,&cheat_string_width, NULL))
		tumDrawText(cheat_string,cheat_button->x_pos + LOBBY_BUTTON_WIDTH/2-cheat_string_width/2,
					cheat_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2, White);
}

void vDrawHighscoreButton(my_square_t* highscore_button)
{
	static char highscore_string[100];
	static int highscore_string_width = 0;

	sprintf(highscore_string, "HIGHSCORE [H]");

	tumDrawFilledBox(highscore_button->x_pos, highscore_button->y_pos, highscore_button->width, highscore_button->height, highscore_button->color);

	if (!tumGetTextSize((char *)highscore_string,&highscore_string_width, NULL))
		tumDrawText(highscore_string,highscore_button->x_pos + LOBBY_BUTTON_WIDTH/2-highscore_string_width/2,
					highscore_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE/2, White);
}

void vDrawTwoPlayerButton(my_square_t* two_player_mode_button, unsigned char mothership_AI_control)
{
	static char two_player_mode_string[100];
	static int two_player_mode_string_width = 0;

	sprintf(two_player_mode_string, "TWO PLAYER (AI-MODE) [A]");

	if(mothership_AI_control)
		tumDrawFilledBox(two_player_mode_button->x_pos, two_player_mode_button->y_pos, two_player_mode_button->width, two_player_mode_button->height, Green);
	else
		tumDrawFilledBox(two_player_mode_button->x_pos, two_player_mode_button->y_pos, two_player_mode_button->width, two_player_mode_button->height, Black);

	if (!tumGetTextSize((char *)two_player_mode_string,&two_player_mode_string_width, NULL))
		tumDrawText(two_player_mode_string,two_player_mode_button->x_pos + LOBBY_BUTTON_WIDTH/2-two_player_mode_string_width/2,
					two_player_mode_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE/2, White);

}

void vDrawBackButton(my_square_t* back_button)
{
	static char back_string[100];
	static int back_string_width = 0;

	sprintf(back_string, "BACK [B]");

	if (!tumDrawFilledBox(back_button->x_pos, back_button->y_pos, back_button->width, back_button->height, back_button->color)){} //Draw Box.

	if (!tumGetTextSize((char *)back_string, &back_string_width, NULL))
		tumDrawText(back_string, back_button->x_pos + LOBBY_BUTTON_WIDTH / 2 - back_string_width / 2,
					back_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE / 2, White);

}



void vDrawSetLevelButton(my_square_t* setlevel_button, short game_wrapper_level)
{
	static char setlevel_string[100];
	static int setlevel_string_width = 0;

	sprintf(setlevel_string, "SET LEVEL: %d [UP] [DOWN]", game_wrapper_level + 1);

	if(game_wrapper_level == 0)
		tumDrawFilledBox(setlevel_button->x_pos, setlevel_button->y_pos, setlevel_button->width, setlevel_button->height, Grey);
	else
		tumDrawFilledBox(setlevel_button->x_pos, setlevel_button->y_pos, setlevel_button->width, setlevel_button->height, Green);

	if (!tumGetTextSize((char *)setlevel_string,&setlevel_string_width, NULL))
		tumDrawText(setlevel_string,setlevel_button->x_pos + LOBBY_BUTTON_WIDTH/2-setlevel_string_width/2,
				setlevel_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2, White);
}

void vDrawInfiniteLifeButton(my_square_t* inflife_button, unsigned char game_wrapper_infinite_life_flag)
{
	static char inflife_string[100];
	static int inflife_string_width = 0;

	sprintf(inflife_string, "INFINITE LIFES [L]");

	if(game_wrapper_infinite_life_flag)
		tumDrawFilledBox(inflife_button->x_pos, inflife_button->y_pos, inflife_button->width, inflife_button->height, Green);
	else
		tumDrawFilledBox(inflife_button->x_pos, inflife_button->y_pos, inflife_button->width, inflife_button->height, inflife_button->color);

	if (!tumGetTextSize((char *)inflife_string,&inflife_string_width, NULL))
		tumDrawText(inflife_string,inflife_button->x_pos + LOBBY_BUTTON_WIDTH/2-inflife_string_width/2,
				inflife_button->y_pos + LOBBY_BUTTON_HEIGHT / 2 - DEFAULT_FONT_SIZE /2, White);
}

void vDrawSetScoreButton(my_square_t* setscore_button, unsigned char game_wrapper_set_score_flag)
{
	static char setscore_string[100];
	static int setscore_string_width = 0;
	static char one_hundred_string[100];
	static int one_hundred_string_width = 0;
	static char one_thousand_string[100];
	static int one_thousand_string_width = 0;
	static char ten_thousand_string[100];
	static int ten_thousand_string_width = 0;

	sprintf(one_hundred_string, "100 [T]");
	sprintf(one_thousand_string, "1000 [K]");
	sprintf(ten_thousand_string, "10000 [U]");
	sprintf(setscore_string, "SET SCORE:");

	unsigned int color_100 = White;
	unsigned int color_1000 = White;
	unsigned int color_10000 = White;

	if(game_wrapper_set_score_flag == 1)
		color_100 = Green;
	else if(game_wrapper_set_score_flag == 2)
		color_1000 = Green;
	else if(game_wrapper_set_score_flag == 3)
		color_10000 = Green;

	tumDrawFilledBox(setscore_button->x_pos, setscore_button->y_pos, setscore_button->width, setscore_button->height, Grey);

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

}


void vDrawHighscore(short highscore)
{
	static char highscore_string[100];
	static int highscore_string_width = 0;

	sprintf(highscore_string, "%d", highscore);

	if (!tumGetTextSize((char *)highscore_string,&highscore_string_width, NULL))
		tumDrawText(highscore_string, SCREEN_WIDTH/2-highscore_string_width/2,
					SCREEN_HEIGHT*3/6- DEFAULT_FONT_SIZE/2, Green);
}



void vDrawPopUpPageTask(void *pvParameters)
{
	unsigned char next_state = 0;
	short countdown = SECS_TO_WAIT;

	my_square_t* pop_up_page = create_rect(SCREEN_WIDTH/2 - POP_UP_PAGE_WIDTH/2, SCREEN_HEIGHT/2 - POP_UP_PAGE_HEIGHT/2, POP_UP_PAGE_WIDTH, POP_UP_PAGE_HEIGHT,Black);



	while(1)
	{
		if (DrawSignal)
		{
			if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
			{
				if (DrawGameTask) vTaskSuspend(DrawGameTask);

				taskENTER_CRITICAL();

				if(xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE)
				{
//					vDrawBackgroundImage( background_image);
					checkDraw(tumDrawClear(Grey), __FUNCTION__); 	// Clear screen
					vDrawPopUpField(pop_up_page, &next_state, countdown );
				}

				xSemaphoreGive(ScreenLock);

				taskEXIT_CRITICAL();

				vTaskDelay((TickType_t)1000); // Basic sleep of 100ms

				countdown--;

				if(countdown <= 0)
				{
					if (StateQueue)
					{
						if(xQueueOverwrite(StateQueue, &next_state) != pdPASS)
						{
							prints("failed to send\n");

						}
					}
					countdown = SECS_TO_WAIT;

					vTaskSuspend(NULL);
				}
			}
		}
	}
}



void vDrawLobbyMainTask(void *pvParameters){

	unsigned char mothership_AI_control = 0;

	// create buttons
    my_square_t* play_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*3/7 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);
    my_square_t* cheat_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*4/7 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);
    my_square_t* highscore_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*5/7 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);
    my_square_t* two_player_mode_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*6/7 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);

    image_handle_t background_image = tumDrawLoadImage("../resources/images/lobby_main.png");

	while(1){

		if (DrawSignal)
		{
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
            {
    			if (xSemaphoreTake(mothership.lock, portMAX_DELAY) == pdTRUE)
    			{
    				mothership_AI_control = mothership.AI_control;
    			}
				xSemaphoreGive(mothership.lock);

    			taskENTER_CRITICAL();

				if(xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE)
				{
					vDrawBackgroundImage(background_image);
					vDrawTwoPlayerButton(two_player_mode_button, mothership_AI_control);
					vDrawHighscoreButton(highscore_button);
					vDrawCheatButton(cheat_button);
					vDrawPlayButton(play_button);
				}
				xSemaphoreGive(ScreenLock);

				taskEXIT_CRITICAL();

				vTaskDelay((TickType_t)100); // Basic sleep of 100ms
            }
		}
	}
}



void vDrawLobbyCheatTask(void *pvParameters){

	short game_wrapper_level = 0;
	unsigned char game_wrapper_infinite_life_flag = 0;
	unsigned char game_wrapper_set_score_flag = 0;

	// create button fields
	my_square_t* setscore_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*2/9 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT *2 ,Grey);
	my_square_t* inflife_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*5/9 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Grey);
    my_square_t* play_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*7/9 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);
    my_square_t* back_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*8/9 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);
    my_square_t* setlevel_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2, SCREEN_HEIGHT*4/9 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT, Grey);

    image_handle_t background_image = tumDrawLoadImage("../resources/images/lobby_cheat.png");

	while(1){

		if (DrawSignal)
		{
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
            {
        		if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
        		{
        			game_wrapper_level = game_wrapper.level;
        			game_wrapper_infinite_life_flag = game_wrapper.infinite_life_flag;
        			game_wrapper_set_score_flag = game_wrapper.set_score_flag;
        		}
    			xSemaphoreGive(game_wrapper.lock);

        		taskENTER_CRITICAL();

        		if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE) {

    				vDrawBackgroundImage(background_image);
    				vDrawSetScoreButton(setscore_button, game_wrapper_set_score_flag);
    				vDrawInfiniteLifeButton(inflife_button, game_wrapper_infinite_life_flag);
    				vDrawSetLevelButton(setlevel_button, game_wrapper_level);
    				vDrawPlayButton(play_button);
    				vDrawBackButton(back_button);
        		}
				xSemaphoreGive(ScreenLock);

				taskEXIT_CRITICAL();

				vTaskDelay((TickType_t)100); // Basic sleep of 100ms

            }
		}
	}
}



void vDrawLobbyHighscoreTask(void *pvParameters){

	short highscore = 0;

	// draw back button
    my_square_t* back_button=create_rect(SCREEN_WIDTH/2 - LOBBY_BUTTON_WIDTH/2,  SCREEN_HEIGHT*5/6 - LOBBY_BUTTON_HEIGHT/2, LOBBY_BUTTON_WIDTH, LOBBY_BUTTON_HEIGHT,Black);

    image_handle_t background_image = tumDrawLoadImage("../resources/images/lobby_highscore.png");

	while(1){

		if (DrawSignal)
		{
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
            {
        		if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
        		{
        			highscore = game_wrapper.highscore;
        		}
    			xSemaphoreGive(game_wrapper.lock);

        		taskENTER_CRITICAL();

        		if (xSemaphoreTake(ScreenLock, 0) == pdTRUE) {

    				vDrawBackgroundImage(background_image);
    				vDrawHighscore(highscore);
    				vDrawBackButton(back_button);
        		}
				xSemaphoreGive(ScreenLock);

        		taskEXIT_CRITICAL();

        		vTaskDelay((TickType_t)100); // Basic sleep of 100ms
            }
		}
	}
}



void vDrawScore()
{
	static char scoreText_string[100];
	static int scoreText_string_width = 0;

	// get score
	if (xSemaphoreTake(game_wrapper.lock, 0) == pdTRUE)
	{
		sprintf(scoreText_string, "Score: %d", game_wrapper.score);
	}
	xSemaphoreGive(game_wrapper.lock);

	if (!tumGetTextSize((char *)scoreText_string, &scoreText_string_width, NULL))
		tumDrawText(scoreText_string, SCREEN_WIDTH/2 - scoreText_string_width/2, SCREEN_HEIGHT/20 - DEFAULT_FONT_SIZE /2, White);
}

void vDrawPauseResume()
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
	}
	xSemaphoreGive(invaders.lock);

	if (!tumGetTextSize((char *)pause_resumeText_string, &pause_resumeText_string_width, NULL))
	{
		tumDrawText(pause_resumeText_string, SCREEN_WIDTH*19/20 - pause_resumeText_string_width, SCREEN_HEIGHT/20 - DEFAULT_FONT_SIZE /2, White);
	}

}

void vDrawReset()
{
	static char quitText_string[100];
	static int quitText_string_width = 0;

	sprintf(quitText_string, "RESET [B]");

	if (!tumGetTextSize((char *)quitText_string, &quitText_string_width, NULL))
		tumDrawText(quitText_string, SCREEN_WIDTH*19/20 - quitText_string_width, SCREEN_HEIGHT*2/20 - DEFAULT_FONT_SIZE /2, White);
}

void vDrawLevel()
{
	static char levelText_string[100];
	static int levelText_string_width = 0;

	// get score
	if (xSemaphoreTake(game_wrapper.lock, 0) == pdTRUE)
	{
		sprintf(levelText_string, "Level: %d", game_wrapper.level + 1);
	}
	xSemaphoreGive(game_wrapper.lock);

	if (!tumGetTextSize((char *)levelText_string, &levelText_string_width, NULL))
		tumDrawText(levelText_string, SCREEN_WIDTH/10 - levelText_string_width/2, SCREEN_HEIGHT/20 - DEFAULT_FONT_SIZE /2, White);
}


void vDrawLifes(my_square_t* life_shape, image_handle_t life_image)
{
	// get lifes
	if (xSemaphoreTake(game_wrapper.lock, 0) == pdTRUE)
	{
		for(unsigned char u = 0; u < game_wrapper.remaining_life; u++)
		{
			if(tumDrawLoadedImage(life_image, SCREEN_WIDTH/30 - LIFE_SIZE_X/2 + u * (LIFE_SIZE_X/2 + LIFE_SIZE_DISTANCE), SCREEN_HEIGHT*29/30 - LIFE_SIZE_Y/2))
			{
				if (!tumDrawFilledBox( SCREEN_WIDTH/30 - LIFE_SIZE_X/2 + u * (LIFE_SIZE_X/2 + LIFE_SIZE_DISTANCE),  SCREEN_HEIGHT*29/30 - LIFE_SIZE_Y/2 , LIFE_SIZE_X, LIFE_SIZE_Y, Red)){}

			}
		}
	}
	xSemaphoreGive(game_wrapper.lock);
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

void vDrawAlien(unsigned char * swap_state, my_square_t* alien_shape, short invaders_pos_x, short invaders_pos_y, unsigned char i, unsigned char j, image_handle_t alien_1_1_image, image_handle_t alien_1_2_image, image_handle_t alien_2_1_image, image_handle_t alien_2_2_image, image_handle_t alien_3_1_image, image_handle_t alien_3_2_image)
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

void vDrawInvaders(unsigned char *swap_state, my_square_t* alien_shape, image_handle_t alien_1_1_image, image_handle_t alien_1_2_image, image_handle_t alien_2_1_image, image_handle_t alien_2_2_image, image_handle_t alien_3_1_image, image_handle_t alien_3_2_image)
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
					vDrawAlien(swap_state, alien_shape, invaders_pos_x, invaders_pos_y, i, j, alien_1_1_image, alien_1_2_image, alien_2_1_image, alien_2_2_image, alien_3_1_image, alien_3_2_image);
				}

			}
		}
	}
	xSemaphoreGive(invaders.lock);
}

void vDrawBullet(unsigned char bullet_alive, my_square_t* bullet_shape, image_handle_t bullet_image)
{
	if (bullet_alive)
	{
		if(tumDrawLoadedImage(bullet_image, bullet_shape->x_pos, bullet_shape->y_pos))
		{
			tumDrawFilledBox(bullet_shape->x_pos, bullet_shape->y_pos, bullet_shape->width, bullet_shape->height, bullet_shape->color);
		}
	}
}


void vDrawPlayer(my_square_t* player_shape, image_handle_t player_image )
{
	if(tumDrawLoadedImage(player_image, player_shape->x_pos, player_shape->y_pos))
	{
		if (!tumDrawFilledBox(player_shape->x_pos, player_shape->y_pos, player_shape->width, player_shape->height, player_shape->color)){}
	}

}

void vDrawMothership(unsigned char mothership_alive, my_square_t* mothership_shape, image_handle_t mothership_image )
{
	if(mothership_alive)
	{
		if(tumDrawLoadedImage(mothership_image, mothership_shape->x_pos, mothership_shape->y_pos))
		{
			if (!tumDrawFilledBox(mothership_shape->x_pos, mothership_shape->y_pos, mothership_shape->width, mothership_shape->height, mothership_shape->color)){}
		}
	}
}

void vGetPlayerBulletPos(my_square_t* player_shape, my_square_t* player_bullet_shape, unsigned char *player_bullet_alive)
{
	if (xSemaphoreTake(player.lock, 0) == pdTRUE)
	{
	    	player_shape->x_pos = player.pos_x;

	    	if(player.bullet.alive == 1)
	    	{
				player_bullet_shape->x_pos = player.bullet.pos_x;
				player_bullet_shape->y_pos = player.bullet.pos_y;
	    	}
	    	*player_bullet_alive = player.bullet.alive;
	}
	xSemaphoreGive(player.lock);
}

void vGetAliensBulletPos(my_square_t* aliens_bullet_shape, unsigned char *aliens_bullet_alive)
{
	if (xSemaphoreTake(invaders.lock, 0) == pdTRUE)
	{
		if(invaders.bullet.alive == 1)
		{
			aliens_bullet_shape->x_pos = invaders.bullet.pos_x;
			aliens_bullet_shape->y_pos = invaders.bullet.pos_y;
		}
		*aliens_bullet_alive = invaders.bullet.alive;
	}
	xSemaphoreGive(invaders.lock);
}

void vGetMothershipPos(my_square_t* mothership_shape, unsigned char *mothership_alive)
{
	if (xSemaphoreTake(mothership.lock, 0) == pdTRUE)
	{
		mothership_shape->x_pos = mothership.pos_x;
		*mothership_alive = mothership.alive;
	}
	xSemaphoreGive(mothership.lock);
}

void vDrawBunker(my_square_t* bunker_block_shape, image_handle_t bunker_block_worse_image, image_handle_t bunker_block_bad_image, image_handle_t bunker_block_good_image )
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
							if (!tumDrawFilledBox(bunker.bunkers[s].pos_x + BUNKER_BLOCK_SIZE_X * r, bunker.pos_y + BUNKER_BLOCK_SIZE_Y * u, BUNKER_BLOCK_SIZE_X, BUNKER_BLOCK_SIZE_Y, Green)){}
						}
						break;
					default:
						break;
					}
				}
			}
		}
	}
	xSemaphoreGive(bunker.lock);
}


void vDrawGameTask(void *pvParameters){

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
    image_handle_t life_image = tumDrawLoadImage("../resources/images/heart.png");
    image_handle_t background_image = tumDrawLoadImage("../resources/images/background.png");

	while(1){

		if (DrawSignal)
			if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
			{
				vGetPlayerBulletPos(player_shape, player_bullet_shape, &player_bullet_alive);
				vGetAliensBulletPos(aliens_bullet_shape, &aliens_bullet_alive);
				vGetMothershipPos(mothership_shape, &mothership_alive);

				taskENTER_CRITICAL();

				if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE) {

					vDrawBackgroundImage(background_image);
					vDrawBunker(bunker_block_shape, bunker_block_worse_image, bunker_block_bad_image, bunker_block_good_image);
					vDrawInvaders(&swap_state, alien_shape, alien_1_1_image, alien_1_2_image, alien_2_1_image, alien_2_2_image, alien_3_1_image, alien_3_2_image);
	           	    vDrawPlayer(player_shape, player_image);
	           	    vDrawMothership(mothership_alive, mothership_shape, mothership_image);
	           	    vDrawBullet(player_bullet_alive, player_bullet_shape, NULL);
					vDrawBullet(aliens_bullet_alive, aliens_bullet_shape, alien_bullet_image);
					vDrawScore();
					vDrawLevel();
					vDrawPauseResume();
					vDrawReset();
					vDrawLifes(life_shape,life_image);
				}

				xSemaphoreGive(ScreenLock);

				taskEXIT_CRITICAL();

				vTaskDelay((TickType_t)50); // Basic sleep of 100ms
			}
	}
}


int vInit_Draw()
{
    if (xTaskCreate(vDrawLobbyMainTask, "DrawLobbyMainTask",
    						mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 3,
    						&DrawLobbyMainTask) != pdPASS) {
        PRINT_TASK_ERROR("DrawLobbyMainTask");
        goto err_DrawLobbyMainTask;
    }

    if (xTaskCreate(vDrawLobbyCheatTask, "DrawLobbyCheatTask",
    						mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 3,
    						&DrawLobbyCheatTask) != pdPASS) {
        PRINT_TASK_ERROR("DrawLobbyCheatTask");
        goto err_DrawLobbyCheatTask;
    }

    if (xTaskCreate(vDrawLobbyHighscoreTask, "DrawLobbyHighscoreTask",
    						mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 3,
    						&DrawLobbyHighscoreTask) != pdPASS) {
        PRINT_TASK_ERROR("DrawLobbyHighscoreTask");
        goto err_DrawLobbyHighscoreTask;
    }

    if (xTaskCreate(vDrawGameTask, "DrawGameTask",
    						mainGENERIC_STACK_SIZE * 3, NULL, mainGENERIC_PRIORITY + 3,
    						&DrawGameTask) != pdPASS) {
        PRINT_TASK_ERROR("DrawGameTask");
        goto err_DrawGameTask;
    }

    vTaskSuspend(DrawLobbyMainTask);
    vTaskSuspend(DrawLobbyCheatTask);
    vTaskSuspend(DrawLobbyHighscoreTask);
    vTaskSuspend(DrawGameTask);

    return 0;


	vTaskDelete(DrawLobbyMainTask);
err_DrawLobbyMainTask:
	vTaskDelete(DrawLobbyCheatTask);
err_DrawLobbyCheatTask:
	vTaskDelete(DrawLobbyHighscoreTask);
err_DrawLobbyHighscoreTask:
	vTaskDelete(DrawGameTask);
err_DrawGameTask:

	return -1;
}
