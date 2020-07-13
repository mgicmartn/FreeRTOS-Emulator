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

#include "queue.h"


#include "shapes.h"
#include "space_invaders_handler.h"
#include "constants.h"

static QueueHandle_t PlayerXQueue = NULL;
static QueueHandle_t MothershipXQueue = NULL;
static QueueHandle_t PlayerBulletModeXQueue = NULL;
static QueueHandle_t NextKeyQueue = NULL;
//static QueueHandle_t GameMessageQueue = NULL;

static SemaphoreHandle_t HandleUDP = NULL;

// task handles
TaskHandle_t Init_Game = NULL;
TaskHandle_t Game_Handler = NULL;
TaskHandle_t UDPControlTask = NULL;
TaskHandle_t Draw_pop_up_page = NULL;

aIO_handle_t udp_soc_receive = NULL, udp_soc_transmit = NULL;

typedef enum { NONE = 0, INC = 1, DEC = -1 } opponent_cmd_t;
typedef enum { PASSIVE = 0, ATTACKING = 1} player_bullet_status_t;



invaders_t invaders = {0};
bunker_t bunker = {0};
game_wrapper_t game_wrapper = {0};
mothership_t mothership = {0};
player_t player = {0};


void UDPHandler(size_t read_size, char *buffer, void *args)
{
    opponent_cmd_t next_key = NONE;
    BaseType_t xHigherPriorityTaskWoken1 = pdFALSE;
    BaseType_t xHigherPriorityTaskWoken2 = pdFALSE;
    BaseType_t xHigherPriorityTaskWoken3 = pdFALSE;

    if (xSemaphoreTakeFromISR(HandleUDP, &xHigherPriorityTaskWoken1) ==
        pdTRUE) {

        char send_command = 0;
        if (strncmp(buffer, "INC", (read_size < 3) ? read_size : 3) ==
            0) {
            next_key = INC;
            send_command = 1;
        }
        else if (strncmp(buffer, "DEC",
                         (read_size < 3) ? read_size : 3) == 0) {
            next_key = DEC;
            send_command = 1;
        }
        else if (strncmp(buffer, "NONE",
                         (read_size < 4) ? read_size : 4) == 0) {
            next_key = NONE;
            send_command = 1;
        }

        if (NextKeyQueue && send_command) {
            xQueueSendFromISR(NextKeyQueue, (void *)&next_key,
                              &xHigherPriorityTaskWoken2);
        }
        xSemaphoreGiveFromISR(HandleUDP, &xHigherPriorityTaskWoken3);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken1 |
                           xHigherPriorityTaskWoken2 |
                           xHigherPriorityTaskWoken3);
    }
    else {
        fprintf(stderr, "[ERROR] Overlapping UDPHandler call\n");
    }
}



void vUDPControlTask(void *pvParameters)
{
    static char buf[50];
    char *addr = NULL; // Loopback
    in_port_t port = UDP_RECEIVE_PORT;
    unsigned int player_x = 0;
    unsigned int mothership_x = 0;
    player_bullet_status_t player_bullet_status = 0;
    player_bullet_status_t last_player_bullet_status = -1;
//    char last_difficulty = -1;
//    char difficulty = 1;

    short last_diff = 0;

    udp_soc_receive =
        aIOOpenUDPSocket(addr, port, UDP_BUFFER_SIZE, UDPHandler, NULL);

    prints("UDP socket opened on port %d\n", port);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(15));
        while (xQueueReceive(PlayerXQueue, &player_x, 0) == pdTRUE) {
        }
        while (xQueueReceive(MothershipXQueue, &mothership_x, 0) == pdTRUE) {
        }
        while (xQueueReceive(PlayerBulletModeXQueue, &player_bullet_status, 0) == pdTRUE) {
        }
//        while (xQueueReceive(DifficultyQueue, &difficulty, 0) == pdTRUE) {
//        }
//        prints("px %d, mx %d\n", player_x, mothership_x);
        short diff = player_x - mothership_x;
        if (diff > 0) {
            sprintf(buf, "+%d", diff);
        }
        else {
            sprintf(buf, "-%d", -diff);
        }

        if(last_diff != diff)
		{
        	aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, buf, strlen(buf));
        	last_diff = diff;
		}

//        prints("send buf: %s\n",buf);

        // send player bullet status
        if(player_bullet_status != last_player_bullet_status)
        {
        	if(player_bullet_status == ATTACKING)
        	{
        		sprintf(buf, "ATTACKING");
        	}
        	else if(player_bullet_status == PASSIVE)
        	{
        		sprintf(buf, "PASSIVE");
        	}

            aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, buf,
                         strlen(buf));
            last_player_bullet_status = player_bullet_status;
        }

//        if (last_difficulty != difficulty) {
//            sprintf(buf, "D%d", difficulty + 1);
//            aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, buf,
//                         strlen(buf));
//            last_difficulty = difficulty;
//        }
    }
}


unsigned char xCheckPongUDPInput(unsigned short *mothership_pox_x)
{
    static opponent_cmd_t current_key = NONE;

    if (NextKeyQueue) {
        xQueueReceive(NextKeyQueue, &current_key, 0);
    }

    if (current_key == INC) {

    		mothership.inc = 1;
    		mothership.dec = 0;

    }
    else if (current_key == DEC) {

    		mothership.inc = 0;
    		mothership.dec = 1;

    }
    else if (current_key == NONE) {

    		mothership.inc = 0;
    		mothership.dec = 0;

    }
    return 0;
}




void set_score_flag(unsigned char score_value_flag)
{
	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{

		if(game_wrapper.set_score_flag == score_value_flag)
		{
			game_wrapper.set_score_flag = 0;
		}
		else
		{
			game_wrapper.set_score_flag = score_value_flag;	//100
		}

		xSemaphoreGive(game_wrapper.lock);
	}
}


void increment_level()
{
	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{

		// fflush(stdout);
		game_wrapper.level ++;

		if(game_wrapper.level > 0)
		{
			game_wrapper.set_level_flag = 1;
		}
		else
		{
			game_wrapper.set_level_flag = 0;
		}
		prints("increment to levelraw %d\n", game_wrapper.level);
		xSemaphoreGive(game_wrapper.lock);
	}
}

void decrement_level()
{
	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{
		prints("decrement level\n");
		// fflush(stdout);
		if(game_wrapper.level > 0) game_wrapper.level --;

		if(game_wrapper.level > 0)
		{
			game_wrapper.set_level_flag = 1;
		}
		else
		{
			game_wrapper.set_level_flag = 0;
		}

		xSemaphoreGive(game_wrapper.lock);
	}
}




void init_invaders(double speed)
{

	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
	{
		prints("in init invaders\n");
		// fflush(stdout);

		invaders.pos_x = INVADERS_INIT_POS_X;
		invaders.float_pos_x = INVADERS_INIT_POS_X;
		invaders.pos_y = INVADERS_INIT_POS_Y;
		invaders.direction = right;
		invaders.speed = speed;
		invaders.state = 0;
		invaders.bullet.alive = 0;
		invaders.last_column_right = NUMBER_OF_ALIENS_X - 1;
		invaders.last_column_left = 0;
		invaders.maxFront = NUMBER_OF_ALIENS_Y - 1;
		invaders.killed = 0;
		invaders.bullet.alive = 0;
		invaders.resume = 1;	// reset last time in game handler
		invaders.paused = 0;

		for (unsigned char i = 0; i < NUMBER_OF_ALIENS_Y; i++)
		{
			for (unsigned char j = 0; j < NUMBER_OF_ALIENS_X; j++)
			{
				// Set every Alien alive
				invaders.enemy[i][j].alive = 1;
				invaders.front[j] = NUMBER_OF_ALIENS_Y - 1;
			}
		}


		xSemaphoreGive(invaders.lock);
	}
}

void init_player(void)
{

	if (xSemaphoreTake(player.lock, portMAX_DELAY) == pdTRUE)
	{
		prints("in init player\n");
		// fflush(stdout);

		player.lives = 3;
		player.pos_x = PLAYER_INIT_X;
		player.pos_y = PLAYER_INIT_Y - PLAYER_SIZE_Y/2;
		player.bullet.alive = 0;
		player.bullet.pos_x = SCREEN_HEIGHT;
		player.bullet.pos_y = 0;

		xSemaphoreGive(player.lock);
	}

}

void init_mothership(void)
{
	if (xSemaphoreTake(mothership.lock, portMAX_DELAY) == pdTRUE)
	{
		prints("in init mothership\n");
		// fflush(stdout);

		mothership.pos_x = -MOTHERSHIP_SIZE_X + SCREEN_WIDTH/2;
		mothership.stop = 0;
		mothership.alive = 0;
		mothership.inc = 0;
		mothership.dec = 0;


		xSemaphoreGive(mothership.lock);
	}
}


void init_bunker(void)
{
	if (xSemaphoreTake(bunker.lock, portMAX_DELAY) == pdTRUE)
	{

		prints("in init bunker\n");
		// fflush(stdout);

		for(short s = 0; s < 4; s++)
		{
			for(short r = 0; r < NUM_BUNKER_BLOCK_X; r++)
			{
				for(short u = 0; u < NUM_BUNKER_BLOCK_Y; u++)
				{
					bunker.bunkers[s].block_destruction_state[u][r] = 3;
					bunker.bunkers[s].pos_x = SCREEN_WIDTH/5 *(s+1) - (BUNKER_BLOCK_SIZE_X * NUM_BUNKER_BLOCK_X) / 2;
				}

				bunker.bunkers[s].player_front[r] = NUM_BUNKER_BLOCK_Y-1;
				bunker.bunkers[s].aliens_front[r] = 0;
			}

		}
		bunker.pos_y = BUNKER_POS_Y;

		xSemaphoreGive(bunker.lock);
	}
}

void init_game_wrapper(double* speed)
{
	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{
		prints("in init game wrapper\n");
		// fflush(stdout);


		if(game_wrapper.next_level_flag)
		{
			game_wrapper.speed += 0.005;
			game_wrapper.level++;
			game_wrapper.get_extra_life_scores += 10;
			check_for_extra_life();
			game_wrapper.next_level_flag = 0;
//			game_wrapper.game_message = "ALIENS WON. BACK TO MENUE IN";
			sprintf(game_wrapper.game_message, 	"YOU WON. NEXT LEVEL IN");
			game_wrapper.next_state = five_state_signal;

		}
		else
		{
			switch (game_wrapper.set_score_flag)
			{
			case 0:
				game_wrapper.score = 0;
				break;
			case 1:
				game_wrapper.score = 100;
				break;
			case 2:
				game_wrapper.score = 1000;
				break;
			case 3:
				game_wrapper.score = 10000;
				break;
			default:
				break;
			}

			if(game_wrapper.set_level_flag)
			{
				prints("set level flag was there\n");
				// fflush(stdout);
				game_wrapper.set_level_flag = 0;

			}
			else
			{
				game_wrapper.level = 0;
			}

//			game_wrapper.game_message = "GAME STARTS IN";
			sprintf(game_wrapper.game_message, "GAME STARTS IN");
			game_wrapper.next_state = five_state_signal;

			game_wrapper.speed = 0.005 * (game_wrapper.level + 1);
			game_wrapper.remaining_life = 3;
		}

		*speed = game_wrapper.speed;

//		unsigned char infinite_life_flag;
//		unsigned char set_score_flag;


		xSemaphoreGive(game_wrapper.lock);
	}
}


void vInit_Game(void *pvParameters)
{
	prints("init init game\n");
	// fflush(stdout);

	double speed = 0;

	while(1)
	{
		prints("init game was running\n");
		// fflush(stdout);

		init_game_wrapper(&speed);
		init_invaders(speed);
		init_player();
		init_bunker();
		init_mothership();

		vTaskDelay(1000);

//        if (GameMessageQueue)
//        {
//        	game_messages_t msg = INIT;
//            xQueueSend(GameMessageQueue, &msg, 0);
//        }

		vTaskResume(Draw_pop_up_page);

//		unsigned char fivestatesignal = 4;
		prints("change state sent #######################\n");
		// fflush(stdout);

		vTaskSuspend(NULL);
	}
}




void move_alien_bullet(bullet_t* bullet, short speed)
{
	if(bullet->alive)
	{

		if (bullet->pos_y < SCREEN_HEIGHT)
		{
			bullet->pos_y += speed;
		}
		else
		{
			// delete bullet if out of screen
			bullet->alive = 0;
			bullet->pos_y = 0;
			bullet->pos_x = 0;
		}

	}
}



void Alien_shoots()
{
	unsigned char input = 0;

	if (AlienShootsQueue) if (xQueueReceive(AlienShootsQueue, &input, 0) == pdTRUE)
	{
		if(input == 1)
		{
			if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
			{
				if(invaders.bullet.alive == 0)
				{
					unsigned char random_alien = rand() % NUMBER_OF_ALIENS_X;
					if(invaders.enemy[0][random_alien].alive)
					{
						invaders.bullet.pos_x = invaders.pos_x + ALIEN_SIZE_X/2 + random_alien * ALIEN_DISTANCE - BULLET_SIZE_X/2;
						invaders.bullet.pos_y = invaders.pos_y + ALIEN_SIZE_Y/2;
						invaders.bullet.alive = 1;
					}

				}

				xSemaphoreGive(invaders.lock);
			}

			input = 0;
		}
	}

}



void move_player_bullet(bullet_t* bullet, short speed)
{
	// move bulletq
	if (bullet->alive == 1)
	{
		if (bullet->pos_y > 0)
		{
			bullet->pos_y -= speed;
		}
		else
		{
			// delete bullet if out of screen
			bullet->alive = 0;
			bullet->pos_x = 0;
			bullet->pos_y = SCREEN_HEIGHT;
		}

	}
}

void handle_player_input(unsigned char* moving_left, unsigned char* moving_right)
{
	unsigned char input = 0;

	if (xSemaphoreTake(player.lock, portMAX_DELAY) == pdTRUE)
	{

		// Handle Player queue input
		if (PlayerQueue)
			if (xQueueReceive(PlayerQueue, &input, 0) == pdTRUE)
			{
				switch (input)
				{
					case MOVE_RIGHT:
						prints("player moves right\n");
						*moving_right = 1;
						*moving_left = 0;
						break;

					case MOVE_LEFT:
						prints("player moves left\n");
						*moving_left = 1;
						*moving_right = 0;
						break;

					case STOP:
						prints("player stoped\n");
						*moving_left = 0;
						*moving_right = 0;
						break;

					case SHOOT:
						prints("player shoots\n");
						if (player.bullet.alive == 0)
						{
							player.bullet.pos_x = player.pos_x + PLAYER_SIZE_X/2 - BULLET_SIZE_X/2;
							player.bullet.pos_y = player.pos_y;
							player.bullet.alive = 1;
						}
						break;

					default:
						break;
				}
			}

		if(player.pos_x > SCREEN_WIDTH - PLAYER_SIZE_X)
		{
			*moving_right = 0;
		}
		if(player.pos_x <= 0)
		{
			*moving_left = 0;
		}

		if(*moving_left) player.pos_x -= PLAYER_SPEED ;
		if(*moving_right) player.pos_x += PLAYER_SPEED;


		player_bullet_status_t bullet_status = 0;

		if(player.bullet.alive)
		{
			bullet_status = ATTACKING;
		}
		else
		{
			bullet_status = PASSIVE;
		}
		xQueueSend(PlayerBulletModeXQueue, &bullet_status, 0);

		move_player_bullet(&player.bullet, BULLET_SPEED);

		// send player pos x to udp handler


		short player_pos_x = player.pos_x;
//		prints("handle player send pos x %d\n", player_pos_x);

		xQueueSend(PlayerXQueue, (void *)&player_pos_x, 0);

		xSemaphoreGive(player.lock);
	}
}


void move_invaders(unsigned char* invaders_won, TickType_t * last_time)
{

//	unsigned char maxFront = 0;

	// move invaders
	if (xSemaphoreTake(invaders.lock, 0) == pdTRUE)
	{

		TickType_t current_time = xTaskGetTickCount();
		invaders.float_pos_x += invaders.speed * (current_time - *last_time);
		invaders.pos_x = round(invaders.float_pos_x);
		*last_time = current_time;

//		prints("pos_x: %d \n", invaders.pos_x);
//		// fflush(stdout);



		if (invaders.direction == right && invaders.pos_x + ALIEN_SIZE_X + invaders.last_column_right * ALIEN_DISTANCE > RIGHT_BORDER)
		{
			invaders.pos_y += INVADERS_STEP_IN_Y;
			invaders.speed *= -1;
			invaders.direction = left;
		}
		else if (invaders.direction == left && invaders.pos_x + invaders.last_column_left * (ALIEN_DISTANCE) < LEFT_BORDER)
		{
			invaders.pos_y += INVADERS_STEP_IN_Y;
			invaders.speed *= -1;
			invaders.direction = right;
		}


		if (invaders.pos_y + ALIEN_SIZE_Y + ALIEN_DISTANCE * invaders.maxFront > BUNKER_POS_Y )
		{
//	    		prints("you lost !!!\n");
			*invaders_won = 1;
		}


		move_alien_bullet(&invaders.bullet, ALIEN_BULLET_SPEED);

		xSemaphoreGive(invaders.lock);
	}

}



void move_mothership(TickType_t* last_time_mothership)
{
	if (xSemaphoreTake(mothership.lock, 0) == pdTRUE)
	{

		unsigned short mothership_pos_x = mothership.pos_x;

		if(mothership.AI_control) xCheckPongUDPInput(&mothership_pos_x);



		if(mothership.alive)
		{
			if(!mothership.AI_control)
			{
				mothership.inc = 1;
				mothership.dec = 0;
			}

			if(mothership.inc)
			{
				mothership.pos_x += 3;
			}
			else if(mothership.dec)
			{
				mothership.pos_x -= 3;
			}

			if(mothership.pos_x > SCREEN_WIDTH )
			{
				if(mothership.AI_control)
				{
					mothership.pos_x = -MOTHERSHIP_SIZE_X;
				}
				else
				{
					mothership.alive = 0;
					*last_time_mothership = xTaskGetTickCount();
				}
			}
			if(mothership.pos_x < -MOTHERSHIP_SIZE_X)
			{
				if(mothership.AI_control)
				{
					mothership.pos_x = SCREEN_WIDTH;
				}
				else
				{
					mothership.alive = 0;
					*last_time_mothership = xTaskGetTickCount();
				}
			}
		}

		// send mothership pos x to udp handler
		xQueueSend(MothershipXQueue, (void *)&mothership.pos_x, 0);

		xSemaphoreGive(mothership.lock);
	}

}



void handle_mothership_appearance(TickType_t last_time_mothership)
{

	if (xSemaphoreTake(mothership.lock, 0) == pdTRUE)
	{
//		prints("handle mothership appearance %d, %d, %d\n", score, *last_time_mothership, invaders_pos_y);
//		// fflush(stdout);

		if(mothership.alive == 0)
		{
			if(xTaskGetTickCount() - last_time_mothership > pdMS_TO_TICKS(WAITING_TIME_FOR_MOTHERSHIP))
			{
				prints("mothership got released!!\n");
				mothership.alive = 1;
				mothership.pos_x = -MOTHERSHIP_SIZE_X;
			}
		}


		xSemaphoreGive(mothership.lock);
	}
}


void vkill_Alien(unsigned char y, unsigned char x, unsigned char * player_won)
{
	unsigned char killed = 0;

	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
	{
		if(invaders.enemy[y][x].alive)
		{
			invaders.enemy[y][x].alive = 0;

			// decreasing front
			if(invaders.front[x] >= 0) invaders.front[x] --;

			// adjust right and left border of invaders block
			if(invaders.front[x] < 0)
			{
				for( unsigned char r = 0; r < NUMBER_OF_ALIENS_X; r++)
				{
					if(invaders.front[r] < 0) invaders.last_column_left = r;
					else break;
				}
				for( unsigned char t = NUMBER_OF_ALIENS_X - 1; t >= 0; t--)
				{
					if(invaders.front[t] < 0)
					{
						invaders.last_column_right = t;
					}
					else
					{
						prints("t == %d\n", t);
						break;
					}
				}
			}


			// check where the invaders block ends down side
			invaders.maxFront = 0;
			for(unsigned char c = 0; c < NUMBER_OF_ALIENS_X; c++)
			{
				if(invaders.maxFront < invaders.front[c])
				{
					invaders.maxFront = invaders.front[c];
				}
			}

			invaders.killed ++;

			// increase speed
			if(invaders.direction == right) invaders.speed += INVADERS_MAX_SPEED / (NUMBER_OF_ALIENS_X * NUMBER_OF_ALIENS_Y);
			else if(invaders.direction == left) invaders.speed -= INVADERS_MAX_SPEED / (NUMBER_OF_ALIENS_X * NUMBER_OF_ALIENS_Y);

			killed = 1;

			// check if all aliens killed
			if(invaders.killed >= NUMBER_OF_ALIENS_X * NUMBER_OF_ALIENS_Y)
			{
				*player_won = 1;
				prints("player won\n");
				// fflush(stdout);
			}

			prints("max front %d\n", invaders.maxFront);
			prints("invaders front %d\n", invaders.front[x]);
			prints("last column left %d\n", invaders.last_column_left);
			prints("last column right %d\n", invaders.last_column_right);
			prints("invader speed: %f\n", invaders.speed);
			prints("killed: %d,%d front: %d total: %d\n", y,x,invaders.front[x],invaders.killed);
			// fflush(stdout);
		}

		xSemaphoreGive(invaders.lock);
	}

	if (killed)
	{
		// handle scoring
		if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
		{
			if(y == 0)
			{
				game_wrapper.score += 30;
				game_wrapper.get_extra_life_scores += 3;
			}
			else if(y < 3 && y > 0)
			{
				game_wrapper.score += 20;
				game_wrapper.get_extra_life_scores += 2;
			}
			else if(y < NUMBER_OF_ALIENS_Y && y > 2)
			{
				game_wrapper.score += 10;
				game_wrapper.get_extra_life_scores += 1;
			}

			prints("score: %d\n", game_wrapper.score);
			// fflush(stdout);

			check_for_extra_life();

			xSemaphoreGive(game_wrapper.lock);
		}

		if (xSemaphoreTake(player.lock, portMAX_DELAY) == pdTRUE)
		{
			player.bullet.alive = 0;
			xSemaphoreGive(player.lock);
		}
	}



}


void vkill_Mothership(TickType_t * last_time_mothership)
{
	if(mothership.alive)
	{
		if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
		{
			game_wrapper.score += 50;
			game_wrapper.get_extra_life_scores += 5;

			check_for_extra_life();

			xSemaphoreGive(game_wrapper.lock);
		}

		*last_time_mothership = xTaskGetTickCount();
		mothership.alive = 0;
	}

}




void destruct_bunker_block_player(short s, short player_front, short t)
{

	if(bunker.bunkers[s].block_destruction_state[ player_front ][ t ] > 0)
	{
		bunker.bunkers[s].block_destruction_state[ player_front ][ t ]--;
	}

	prints("player_front: %d\n",player_front);

	if(bunker.bunkers[s].block_destruction_state[player_front][t] == 0)
	{
		if(player_front > 0)
		{
			bunker.bunkers[s].player_front[t]--;
		}

	}


	if (xSemaphoreTake(player.lock, portMAX_DELAY) == pdTRUE)
	{
		player.bullet.alive = 0;
		xSemaphoreGive(player.lock);
	}

}


void destruct_bunker_block_alien(short s, short aliens_front, short t)
{

	if(bunker.bunkers[s].block_destruction_state[ aliens_front ][ t ] > 0)
	{
		// maybe invaders should destrct a block by one shot
		bunker.bunkers[s].block_destruction_state[ aliens_front ][ t ] = 0;
	}

	prints("aliens_front: %d\n",aliens_front);

	if(bunker.bunkers[s].block_destruction_state[aliens_front][t] == 0)
	{
		if(aliens_front < NUM_BUNKER_BLOCK_Y - 1)
		{
			bunker.bunkers[s].aliens_front[t]++;
		}

	}


	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
	{
		invaders.bullet.alive = 0;
		xSemaphoreGive(invaders.lock);
	}

}



void player_dies(unsigned char *player_dead)
{

	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
	{
		invaders.bullet.alive = 0;

		xSemaphoreGive(invaders.lock);
	}

	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{
		if(game_wrapper.infinite_life_flag == 0)
		{
			*player_dead = 1;
		}
		xSemaphoreGive(game_wrapper.lock);
	}


	prints("player got hit\n");
	// fflush(stdout);
}



void check_aliens_bullet_collision(unsigned char *player_dead)
{
	short player_pos_x = 0;
	short player_pos_y = 0;


	short invaders_bullet_alive = 0;
	short invaders_bullet_pos_y = SCREEN_HEIGHT;
	short invaders_bullet_pos_x = 0;

	unsigned char player_bullet_alive = 0;
	short player_bullet_pos_x = 0;
	short player_bullet_pos_y = 0;


	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
	{
		invaders_bullet_alive = invaders.bullet.alive;
		invaders_bullet_pos_y = invaders.bullet.pos_y;
		invaders_bullet_pos_x = invaders.bullet.pos_x;
		xSemaphoreGive(invaders.lock);
	}


	if(invaders_bullet_alive)
	{
		if (xSemaphoreTake(player.lock, portMAX_DELAY) == pdTRUE)
		{
			player_pos_x = player.pos_x;
			player_pos_y = player.pos_y;
			player_bullet_alive = player.bullet.alive;
			player_bullet_pos_x = player.bullet.pos_x;
			player_bullet_pos_y = player.bullet.pos_y;
			xSemaphoreGive(player.lock);
		}


		// check if player bullet crashes invaders bullet
		if(player_bullet_alive)
		{
			if(invaders_bullet_pos_x > player_bullet_pos_x - BULLET_SIZE_X && invaders_bullet_pos_x < player_bullet_pos_x + BULLET_SIZE_X * 2)
			{
				prints("bullet crash xxxxxxxxx!!\n");
				// fflush(stdout);

				if(invaders_bullet_pos_y + BULLET_SIZE_Y * 3 > player_bullet_pos_y)
				{
					prints("bullet crash yyyyyyyyy!!\n");
					// fflush(stdout);

					if (xSemaphoreTake(player.lock, portMAX_DELAY) == pdTRUE)
					{
						player.bullet.alive = 0;
						xSemaphoreGive(player.lock);
					}

					if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
					{
						invaders.bullet.alive = 0;
						xSemaphoreGive(invaders.lock);
					}

					prints("bullet crash!!\n");
					// fflush(stdout);
				}
			}
		}


		// check if bullet reached player y
		if (invaders_bullet_pos_y > player_pos_y)
		{
			prints("player bullet pos x: %d, player pos x: %d\n", invaders_bullet_pos_x, player_pos_x);
			if (invaders_bullet_pos_x - BULLET_SIZE_X >= player_pos_x && invaders_bullet_pos_x <= player_pos_x + PLAYER_SIZE_X)
			{
				player_dies(player_dead);
			}
		}

		if (xSemaphoreTake(bunker.lock, portMAX_DELAY) == pdTRUE)
		{
			// check if bunker reached and not already passed
			if(invaders_bullet_pos_y > bunker.pos_y - BULLET_SIZE_Y && invaders_bullet_pos_y < bunker.pos_y + BUNKER_BLOCK_SIZE_Y * NUM_BUNKER_BLOCK_Y  )
			{

				for(short s = 0; s < 4; s++)
				{
					// check at which bunker
					if(invaders_bullet_pos_x > bunker.bunkers[s].pos_x && invaders_bullet_pos_x < bunker.bunkers[s].pos_x + BUNKER_BLOCK_SIZE_X*NUM_BUNKER_BLOCK_X)
					{
						for( short t = 0; t < NUM_BUNKER_BLOCK_X; t++)
						{
							// check column
							if((invaders_bullet_pos_x > bunker.bunkers[s].pos_x + t * BUNKER_BLOCK_SIZE_X - BULLET_SIZE_X) && (invaders_bullet_pos_x < bunker.bunkers[s].pos_x + BUNKER_BLOCK_SIZE_X + t * BUNKER_BLOCK_SIZE_X ))
							{
								if(bunker.bunkers[s].block_destruction_state[bunker.bunkers[s].player_front[t]][t] > 0)
								{
									destruct_bunker_block_alien(s, bunker.bunkers[s].aliens_front[t], t);
								}
							}

//							prints("x column: %d\n",t);
						}
//						prints("bunker number: %d\n",s);
					}
				}
			}

			xSemaphoreGive(bunker.lock);
		}


	}

}


void check_player_bullet_collision(unsigned char * player_won, TickType_t * last_time_mothership)
{
	short invaders_pos_y = 0;
	short invaders_pos_x = 0;
	short invaders_front[NUMBER_OF_ALIENS_X];

	short player_bullet_alive = 0;
	short player_bullet_pos_y = SCREEN_HEIGHT;
	short player_bullet_pos_x = 0;


	if (xSemaphoreTake(player.lock, portMAX_DELAY) == pdTRUE)
	{
		player_bullet_alive = player.bullet.alive;
		player_bullet_pos_y = player.bullet.pos_y;
		player_bullet_pos_x = player.bullet.pos_x;
		xSemaphoreGive(player.lock);
	}

	if(player_bullet_alive)
	{
		if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
		{
			invaders_pos_y = invaders.pos_y;
			invaders_pos_x = invaders.pos_x;
			for (unsigned char u = 0; u < NUMBER_OF_ALIENS_X; u++)
			{
				invaders_front[u] = invaders.front[u];
			}
			xSemaphoreGive(invaders.lock);
		}


		// check if invaders block reached and not already passed
		if(player_bullet_pos_y < invaders_pos_y + ALIEN_DISTANCE * NUMBER_OF_ALIENS_Y * 1  + ALIEN_SIZE_Y && player_bullet_pos_y > invaders_pos_y - INVADERS_SIZE_Y / 2)
		{
			for(unsigned char i = 0; i < NUMBER_OF_ALIENS_X; i++)
			{
				// for every alien check x
				if( (player_bullet_pos_x > invaders_pos_x  + i * ALIEN_DISTANCE - BULLET_SIZE_X) && (player_bullet_pos_x < invaders_pos_x + i * ALIEN_DISTANCE + ALIEN_SIZE_X ))
				{
					// for every alien whose x the bullet is, check the front alien && check if column still exists
					if (player_bullet_pos_y < invaders_pos_y + ALIEN_DISTANCE * invaders_front[i] + ALIEN_SIZE_Y &&
//							player_bullet_pos_y > invaders_pos_y + ALIEN_SIZE_Y + ALIEN_DISTANCE * invaders_front[i - 1] &&
							invaders_front[i] >= 0)
					{
						vkill_Alien(invaders_front[i],i, player_won);
					}

//					for(short u = NUMBER_OF_ALIENS_Y - 1; u >= 0; u--)
//					{
//						if(player_bullet_pos_y < invaders_pos_y + ALIEN_SIZE_Y + ALIEN_DISTANCE * u && player_bullet_pos_y > invaders_pos_y + ALIEN_SIZE_Y + ALIEN_DISTANCE * u - ALIEN_SIZE_Y - ALIEN_DISTANCE / 2)
//						{
//							vkill_Alien(u,i, player_won);
//						}
//					}
				}
			}
		}



		if (xSemaphoreTake(bunker.lock, portMAX_DELAY) == pdTRUE)
		{
			// check if bunker reached and not already passed
			if(player_bullet_pos_y < bunker.pos_y + BUNKER_BLOCK_SIZE_Y*NUM_BUNKER_BLOCK_Y && player_bullet_pos_y > bunker.pos_y - BULLET_SIZE_Y)
			{

				for(short s = 0; s < 4; s++)
				{
					// check at which bunker
					if(player_bullet_pos_x > bunker.bunkers[s].pos_x && player_bullet_pos_x < bunker.bunkers[s].pos_x + BUNKER_BLOCK_SIZE_X*NUM_BUNKER_BLOCK_X)
					{
						for( short t = 0; t < NUM_BUNKER_BLOCK_X; t++)
						{


							// check column
							if((player_bullet_pos_x > bunker.bunkers[s].pos_x + t * BUNKER_BLOCK_SIZE_X - BULLET_SIZE_X) && (player_bullet_pos_x < bunker.bunkers[s].pos_x + BUNKER_BLOCK_SIZE_X + t * BUNKER_BLOCK_SIZE_X ))
							{
								if(bunker.bunkers[s].block_destruction_state[bunker.bunkers[s].player_front[t]][t] > 0)
								{
									destruct_bunker_block_player(s, bunker.bunkers[s].player_front[t], t);
								}

							}

							prints("x column: %d\n",t);
						}
						prints("bunker number: %d\n",s);
					}
				}
			}

			xSemaphoreGive(bunker.lock);
		}


		if (xSemaphoreTake(mothership.lock, portMAX_DELAY) == pdTRUE)
		{
			// check if mothership reached and not already passed
			if(player_bullet_pos_y < MOTHERSHIP_POS_Y + MOTHERSHIP_SIZE_Y && player_bullet_pos_y > MOTHERSHIP_POS_Y - BULLET_SIZE_Y*2)
			{
				if(player_bullet_pos_x > mothership.pos_x && player_bullet_pos_x < mothership.pos_x + MOTHERSHIP_SIZE_X)
				{
					vkill_Mothership(last_time_mothership);
				}

			}

			xSemaphoreGive(mothership.lock);
		}

	}

}


void set_new_last_time_resume(	unsigned char* invaders_resume, TickType_t* last_time, TickType_t* last_time_mothership)
{


	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
	{
		*invaders_resume = invaders.resume;

		if(invaders.resume)
		{
			invaders.resume = 0;
		}

		xSemaphoreGive(invaders.lock);
	}

	if(*invaders_resume)
	{
		prints("start with new last time\n");
		// fflush(stdout);
		*last_time = xTaskGetTickCount();
		*last_time_mothership = xTaskGetTickCount();
		*invaders_resume = 0;
	}
}


void handle_player_death(unsigned char* invaders_won)
{

		if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
		{
			game_wrapper.remaining_life--;
			prints("remaining life: %d\n", game_wrapper.remaining_life);
			// fflush(stdout);

			if(game_wrapper.remaining_life == 0) *invaders_won = 1;

			xSemaphoreGive(game_wrapper.lock);
		}
}

void handle_end_match(end_game_reason_t reason)
{

	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{

		if(reason == INVADERS_WON)	// invaders won
		{
			if(game_wrapper.highscore < game_wrapper.score) game_wrapper.highscore = game_wrapper.score;
			game_wrapper.next_level_flag = 0;

			sprintf(game_wrapper.game_message, "ALIENS WON. BACK TO MENUE IN");
			game_wrapper.next_state = one_state_signal;

		}
		else if(reason == PLAYER_WON)	// player won
		{
			if(game_wrapper.highscore < game_wrapper.score) game_wrapper.highscore = game_wrapper.score;
			game_wrapper.next_level_flag = 1;
		}
		else if(reason == RESET_PRESSED)	// reset pressed
		{
			if(game_wrapper.highscore < game_wrapper.score) game_wrapper.highscore = game_wrapper.score;
			game_wrapper.next_level_flag = 0;
		}

		xSemaphoreGive(game_wrapper.lock);
	}
}



void check_for_extra_life()
{
	if(game_wrapper.get_extra_life_scores > 150)
	{
		if(game_wrapper.remaining_life < 3)
		{
			game_wrapper.remaining_life++;
		}

		game_wrapper.get_extra_life_scores = 0;
	}
}


void vGame_Handler(void *pvParameters)
{

	unsigned char invaders_won = 0;
	unsigned char player_won = 0;
	unsigned char player_dead = 0;
	unsigned char moving_left = 0;
	unsigned char moving_right = 0;
	unsigned char invaders_resume = 0;
	TickType_t last_time = 0;
	TickType_t last_time_mothership = 0;
//	unsigned char mothership_AI_control = 0;


	while(1){


		set_new_last_time_resume(&invaders_resume, &last_time, &last_time_mothership);
		move_invaders(&invaders_won, &last_time);

		// mothership
		handle_mothership_appearance(last_time_mothership);
		move_mothership(&last_time_mothership);

		handle_player_input(&moving_left, &moving_right);
		check_player_bullet_collision(&player_won, &last_time_mothership);
		check_aliens_bullet_collision(&player_dead);
		Alien_shoots();

		if(player_dead)
		{
			handle_player_death(&invaders_won);
			player_dead = 0;
		}

		if(invaders_won)
		{
			handle_end_match(INVADERS_WON);
			invaders_won = 0;

			prints("invaders won.\n");
			// fflush(stdout);

			vTaskResume(Draw_pop_up_page);

//			if (StateQueue) xQueueSend(StateQueue, &one_state_signal, 0);
		}

		if(player_won)
		{
			handle_end_match(PLAYER_WON);
			player_won = 0;

			prints("player won.\n");

//			vTaskResume(Draw_pop_up_page);
			if (StateQueue) xQueueSend(StateQueue, &four_state_signal, 0);
		}

		vTaskDelay((TickType_t)50); // Basic sleep of 100ms
	}

}



int init_space_invaders_handler(void)
{
    player.lock = xSemaphoreCreateMutex(); 	//State Locking meachanism
    if(!player.lock) {
    	PRINT_ERROR("Failed to create player lock");
		goto err_player_lock;
    }
    invaders.lock = xSemaphoreCreateMutex(); 	//State Locking meachanism
    if(!invaders.lock) {
    	PRINT_ERROR("Failed to create invaders lock");
		goto err_invaders_lock;
    }
    bunker.lock = xSemaphoreCreateMutex(); 	//State Locking meachanism
    if(!bunker.lock) {
    	PRINT_ERROR("Failed to create bunker lock");
		goto err_bunker_lock;
    }
    game_wrapper.lock = xSemaphoreCreateMutex(); 	//State Locking meachanism
    if(!game_wrapper.lock) {
    	PRINT_ERROR("Failed to create game_wrapper lock");
		goto err_game_wrapper_lock;
    }
    mothership.lock = xSemaphoreCreateMutex(); 	//State Locking meachanism
    if(!mothership.lock) {
    	PRINT_ERROR("Failed to create mothership lock");
		goto err_mothership_lock;
    }

    HandleUDP = xSemaphoreCreateMutex();
    if (!HandleUDP) {
        exit(EXIT_FAILURE);
    }


    PlayerXQueue = xQueueCreate(5, sizeof(short));
    if (!PlayerXQueue) {
        exit(EXIT_FAILURE);
    }
    MothershipXQueue = xQueueCreate(5, sizeof(short));
    if (!MothershipXQueue) {
        exit(EXIT_FAILURE);
    }
    PlayerBulletModeXQueue = xQueueCreate(5, sizeof(player_bullet_status_t));
    if (!PlayerBulletModeXQueue) {
        exit(EXIT_FAILURE);
    }
//    GameMessageQueue = xQueueCreate(5, sizeof(game_messages_t));
//    if (!GameMessageQueue) {
//        exit(EXIT_FAILURE);
//    }
    NextKeyQueue = xQueueCreate(1, sizeof(opponent_cmd_t));
    if (!NextKeyQueue) {
        exit(EXIT_FAILURE);
    }


    if (xTaskCreate(vInit_Game, "Init_Game",
    						mainGENERIC_STACK_SIZE * 3, NULL, mainGENERIC_PRIORITY + 3,
    						&Init_Game) != pdPASS) {
        PRINT_TASK_ERROR("Init_Game");
        goto err_Init_Game;
    }

    if (xTaskCreate(vGame_Handler, "Game_Handler",
    						mainGENERIC_STACK_SIZE * 3, NULL, mainGENERIC_PRIORITY + 3,
    						&Game_Handler) != pdPASS) {
        PRINT_TASK_ERROR("Game_Handler");
        goto err_Game_Handler;
    }

    if (xTaskCreate(vDraw_pop_up_page, "Draw_pop_up_page",
    						mainGENERIC_STACK_SIZE * 3, NULL, mainGENERIC_PRIORITY + 3,
    						&Draw_pop_up_page) != pdPASS) {
        PRINT_TASK_ERROR("Draw_pop_up_page");
        goto err_Draw_pop_up_page;
    }

    if (xTaskCreate(vUDPControlTask, "UDPControlTask",
                    mainGENERIC_STACK_SIZE, NULL, mainGENERIC_PRIORITY,
                    &UDPControlTask) != pdPASS) {
        PRINT_TASK_ERROR("UDPControlTask");
        goto err_udpcontrol;
    }


    vTaskSuspend(Game_Handler);
    vTaskSuspend(Init_Game);
    vTaskSuspend(UDPControlTask);
    vTaskSuspend(Draw_pop_up_page);


    return 0;


    vTaskDelete(UDPControlTask);
err_udpcontrol:
	vTaskDelete(Draw_pop_up_page);
err_Draw_pop_up_page:
	vSemaphoreDelete(player.lock);
err_player_lock:
	vSemaphoreDelete(invaders.lock);
err_invaders_lock:
	vSemaphoreDelete(bunker.lock);
err_bunker_lock:
	vSemaphoreDelete(game_wrapper.lock);
err_game_wrapper_lock:
	vSemaphoreDelete(mothership.lock);
err_mothership_lock:
	vTaskDelete(Game_Handler);
err_Game_Handler:
	vTaskDelete(Init_Game);
err_Init_Game:


	return -1;
}
