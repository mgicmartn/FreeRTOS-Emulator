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
QueueHandle_t DifficultyQueue = NULL;
static QueueHandle_t NextKeyQueue = NULL;
//static QueueHandle_t GameMessageQueue = NULL;

static SemaphoreHandle_t HandleUDP = NULL;

// task handles
TaskHandle_t InitGameTaks = NULL;
TaskHandle_t GameHandlerTask = NULL;
TaskHandle_t UDPControlTask = NULL;
TaskHandle_t DrawPopUpPageTask = NULL;

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
        fprints(stderr, "[ERROR] Overlapping UDPHandler call\n");
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
    char last_difficulty = -1;
    char difficulty = 1;

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
        while (xQueueReceive(DifficultyQueue, &difficulty, 0) == pdTRUE) {
        }
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

        if (last_difficulty != difficulty && difficulty != 0) {
            sprintf(buf, "D%d", difficulty);
            aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, buf,
                         strlen(buf));
            last_difficulty = difficulty;
        }
    }
}


unsigned char xCheckMothershipUDPInput()
{
    static opponent_cmd_t current_key = NONE;

    if (NextKeyQueue) {
        while (xQueueReceive(NextKeyQueue, &current_key, 0) == pdTRUE) {
        }
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




void vSetScoreFlag(unsigned char score_value_flag)
{
	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{
		if(game_wrapper.set_score_flag == score_value_flag)
			game_wrapper.set_score_flag = 0;
		else
			game_wrapper.set_score_flag = score_value_flag;
	}
	xSemaphoreGive(game_wrapper.lock);
}


void increment_level()
{
	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{
		game_wrapper.level ++;

		// indicate with set_level_flag that level got manually set
		if(game_wrapper.level > 0)
			game_wrapper.set_level_flag = 1;
		else
			game_wrapper.set_level_flag = 0;
	}
	xSemaphoreGive(game_wrapper.lock);
}

void decrement_level()
{
	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{
		if(game_wrapper.level > 0)
			game_wrapper.level --;

		// indicate with set_level_flag that level got manually set
		if(game_wrapper.level > 0)
			game_wrapper.set_level_flag = 1;
		else
			game_wrapper.set_level_flag = 0;
	}
	xSemaphoreGive(game_wrapper.lock);
}


void vInitInvaders(double speed)
{
	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
	{
		prints("in init invaders\n");

		for (unsigned char x = 0; x < NUMBER_OF_ALIENS_X; x++)
		{
			for (unsigned char y = 0; y < NUMBER_OF_ALIENS_Y; y++)
			{
				// Set every Alien alive
				invaders.enemy[y][x].alive = 1;
			}
			// initialize front
			invaders.front[x] = NUMBER_OF_ALIENS_Y - 1;
		}

		invaders.last_column_right = NUMBER_OF_ALIENS_X - 1;
		invaders.last_column_left = 0;
		invaders.maxFront = NUMBER_OF_ALIENS_Y - 1;
		invaders.pos_x = INVADERS_INIT_POS_X;
		invaders.float_pos_x = INVADERS_INIT_POS_X;
		invaders.pos_y = INVADERS_INIT_POS_Y;
		invaders.direction = right;
		invaders.resume = 1;	// reset last time in game handler
		invaders.paused = 0;
		invaders.killed = 0;
		invaders.speed = speed;
		invaders.bullet.alive = 0;

	}
	xSemaphoreGive(invaders.lock);
}

void vInitPlayer(void)
{
	if (xSemaphoreTake(player.lock, portMAX_DELAY) == pdTRUE)
	{
		prints("in init player\n");

		player.pos_x = PLAYER_INIT_X - PLAYER_SIZE_X/2;
		player.pos_y = PLAYER_INIT_Y - PLAYER_SIZE_Y/2;
		player.bullet.alive = 0;
		player.bullet.pos_x = SCREEN_HEIGHT;
		player.bullet.pos_y = 0;
	}
	xSemaphoreGive(player.lock);
}

void vInitMothership(void)
{
	if (xSemaphoreTake(mothership.lock, portMAX_DELAY) == pdTRUE)
	{
		prints("in init mothership\n");

		mothership.alive = 0;
		mothership.pos_x = -MOTHERSHIP_SIZE_X;
		mothership.inc = 0;
		mothership.dec = 0;
		mothership.stop = 0;
	}
	xSemaphoreGive(mothership.lock);
}


void vInitBunker(void)
{
	if (xSemaphoreTake(bunker.lock, portMAX_DELAY) == pdTRUE)
	{
		prints("in init bunker\n");

		for(short bunkerNumber = 0; bunkerNumber < 4; bunkerNumber++)
		{
			for(short x = 0; x < NUM_BUNKER_BLOCK_X; x++)
			{
				for(short y = 0; y < NUM_BUNKER_BLOCK_Y; y++)
				{
					// init destruction state and position
					bunker.bunkers[bunkerNumber].block_destruction_state[y][x] = 3;
				}

				bunker.bunkers[bunkerNumber].player_front[x] = NUM_BUNKER_BLOCK_Y-1;
				bunker.bunkers[bunkerNumber].aliens_front[x] = 0;
			}
			bunker.bunkers[bunkerNumber].pos_x = SCREEN_WIDTH/5 *(bunkerNumber+1) - (BUNKER_BLOCK_SIZE_X * NUM_BUNKER_BLOCK_X) / 2;
		}
		bunker.pos_y = BUNKER_POS_Y;
	}
	xSemaphoreGive(bunker.lock);
}

void vInitGameWrapper(double* speed)
{
	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{
		prints("in init game wrapper\n");

		// entering game state right after previous level won
		if(game_wrapper.next_level_flag)
		{
			/* do not change:
			 * score
			 * remaining_life
			 * infinite_life_flag
			 */

			game_wrapper.set_level_flag = 0;
			game_wrapper.level++;				// changing into higher level
			game_wrapper.get_extra_life_scores += NEW_LEVEL_GET_EXTRA_LIFE_SCORE_REWARD;

			// initialize pop up page
			sprintf(game_wrapper.game_message, 	"YOU WON. NEXT LEVEL IN");
			game_wrapper.next_state = five_state_signal;

			game_wrapper.next_level_flag = 0;

			vCheckForExtraLife();

		}
		// entering game state from lobby
		else
		{
			// cheat mode score was set
			switch (game_wrapper.set_score_flag)
			{
			case 0:
				game_wrapper.score = 0;
				break;
			case SET_SCORE_100:
				game_wrapper.score = 100;
				break;
			case SET_SCORE_1000:
				game_wrapper.score = 1000;
				break;
			case SET_SCORE_10000:
				game_wrapper.score = 10000;
				break;
			default:
				break;
			}

			// cheat mode level was set
			if(game_wrapper.set_level_flag)
				game_wrapper.set_level_flag = 0;
			else
				game_wrapper.level = 0;

			game_wrapper.remaining_life = 3;

			// initialize pop up page
			sprintf(game_wrapper.game_message, "GAME STARTS IN");
			game_wrapper.next_state = five_state_signal;
		}

		game_wrapper.speed = NEXT_LEVEL_SPEED_CLIMAX * (game_wrapper.level + 1);
		*speed = game_wrapper.speed;
	}
	xSemaphoreGive(game_wrapper.lock);
}


void vInitGameTaks(void *pvParameters)
{
	prints("init init game\n");

	double speed = 0;

	while(1) {

		prints("init game was running\n");

		vInitGameWrapper(&speed);
		vInitInvaders(speed);
		vInitPlayer();
		vInitBunker();
		vInitMothership();

		vTaskDelay(1000);	// wait to avoid state Debounce time

		// unleash Pop Up Page
		vTaskResume(DrawPopUpPageTask);

		prints("change state sent #######################\n");

		vTaskSuspend(NULL);
	}
}




void vMoveAlienBullet(bullet_t* bullet, short speed)
{
	if(bullet->alive)
	{
		//move bullet
		if (bullet->pos_y < SCREEN_HEIGHT) {
			bullet->pos_y += speed;
		}
		else {
			// delete bullet if out of screen
			bullet->alive = 0;
			bullet->pos_y = 0;
			bullet->pos_x = 0;
		}
	}
}



void vAlienShoot()
{
	unsigned char input = 0;

	// Receive from Alien Shooting Task
	if (AlienShootsQueue)
		while(xQueueReceive(AlienShootsQueue, &input, 0) == pdTRUE) {
		}

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
		}
		xSemaphoreGive(invaders.lock);

		input = 0;
	}
}



void vMovePlayerBullet(bullet_t* bullet, short speed)
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

void vMovePlayer(unsigned char* moving_left, unsigned char* moving_right, unsigned char AI_control_ON)
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

		// limit Player Movement to Screen Size
		if(player.pos_x > SCREEN_WIDTH - PLAYER_SIZE_X)
			*moving_right = 0;

		if(player.pos_x <= 0)
			*moving_left = 0;

		// Move player
		if(*moving_left)
			player.pos_x -= PLAYER_SPEED ;
		if(*moving_right)
			player.pos_x += PLAYER_SPEED;


		vMovePlayerBullet(&player.bullet, BULLET_SPEED);

		// send bullet status and player position to binary
		if(AI_control_ON)
		{
			player_bullet_status_t bullet_status = 0;

			if(player.bullet.alive)
				bullet_status = ATTACKING;
			else
				bullet_status = PASSIVE;

			short player_pos_x = player.pos_x;

			xQueueSend(PlayerBulletModeXQueue, (void*)&bullet_status, 0);
			xQueueSend(PlayerXQueue, (void *)&player_pos_x, 0);
		}
	}
	xSemaphoreGive(player.lock);
}


void vMoveInvaders(unsigned char* invaders_won, TickType_t * last_time)
{

	if (xSemaphoreTake(invaders.lock, 0) == pdTRUE)
	{
		// move invaders
		TickType_t current_time = xTaskGetTickCount();
		invaders.float_pos_x += invaders.speed * (current_time - *last_time);
		invaders.pos_x = round(invaders.float_pos_x);
		*last_time = current_time;

		// check left and right border
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

		// check if invaders reached bottom line
		if (invaders.pos_y + ALIEN_SIZE_Y + ALIEN_DISTANCE * invaders.maxFront > BUNKER_POS_Y )
		{
			prints("aliens reached bottonscreen\n");
			if(*invaders_won == 0)
				*invaders_won = 1;
		}

		vMoveAlienBullet(&invaders.bullet, ALIEN_BULLET_SPEED);

	}
	xSemaphoreGive(invaders.lock);

}



void vMoveMothership(TickType_t* last_time_mothership, unsigned char* AI_control_ON)
{
	if (xSemaphoreTake(mothership.lock, 0) == pdTRUE)
	{
		// check AI commands from mothership
		if(mothership.AI_control && mothership.alive)
		{
			*AI_control_ON = 1;
			xCheckMothershipUDPInput();
		}
		else {
			*AI_control_ON = 0;
		}

		if(mothership.alive)
		{
			// Single Player Mode
			if(!mothership.AI_control){
				mothership.inc = 1;
				mothership.dec = 0;
			}

			// move mothership
			if(mothership.inc) {
				mothership.pos_x += MOTHERSHIP_SPEED;
			}
			else if(mothership.dec) {
				mothership.pos_x -= MOTHERSHIP_SPEED;
			}

			// check if mothership reaches end of screen right hand side
			if(mothership.pos_x > SCREEN_WIDTH)
			{
				if(mothership.AI_control) {
					mothership.pos_x = -MOTHERSHIP_SIZE_X;
				}
				else {
					mothership.alive = 0;
					*last_time_mothership = xTaskGetTickCount();
				}
			}

			// check if mothership reaches end of screen left hand side
			if(mothership.pos_x < -MOTHERSHIP_SIZE_X)
			{
				if(mothership.AI_control) {
					mothership.pos_x = SCREEN_WIDTH;
				}
				else {
					mothership.alive = 0;
					*last_time_mothership = xTaskGetTickCount();
				}
			}
		}
		else {
			// revive mothership / handle mothership appearance
			if(xTaskGetTickCount() - *last_time_mothership > pdMS_TO_TICKS(WAITING_TIME_FOR_MOTHERSHIP))
			{
				mothership.alive = 1;
				mothership.pos_x = -MOTHERSHIP_SIZE_X;
			}
		}

		// send mothership pos x to udp handler
		xQueueSend(MothershipXQueue, (void *)&mothership.pos_x, 0);
	}
	xSemaphoreGive(mothership.lock);

}


void vKillPlayerBullet(void)
{
	if (xSemaphoreTake(player.lock, portMAX_DELAY) == pdTRUE)
	{
		player.bullet.alive = 0;
	}
	xSemaphoreGive(player.lock);
}

void vKillInvadersBullet(void)
{
	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
	{
		invaders.bullet.alive = 0;
	}
	xSemaphoreGive(invaders.lock);
}


void vKillAlien(short y, short x, unsigned char * player_won)
{
	unsigned char killed = 0;

	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
	{
		if(invaders.enemy[y][x].alive)
		{
			invaders.enemy[y][x].alive = 0;
			killed = 1;
			invaders.killed ++;

			// adjust front
			for(short u = NUMBER_OF_ALIENS_Y - 1; u >= -1; u--)
			{
				if(invaders.enemy[u][x].alive)
				{
					invaders.front[x] = u;
					break;
				}

				// column empty
				if(u < 0)
				{
					invaders.front[x] = -1;
					break;
				}
			}

			// adjust right and left border of invaders block
			if(invaders.front[x] < 0)
			{
				for( short r = 0; r < NUMBER_OF_ALIENS_X; r++)
				{
					// check if far left column still exists
					if(invaders.front[r] < 0)
						invaders.last_column_left = r;
					else
						break;
				}
				for( short t = NUMBER_OF_ALIENS_X - 1; t >= 0; t--)
				{
					// check if far right column still exists
					if(invaders.front[t] < 0)
						invaders.last_column_right = t;
					else
						break;
				}
			}

			// check where the invaders block ends down side
			invaders.maxFront = 0;
			for(short c = 0; c < NUMBER_OF_ALIENS_X; c++)
			{
				if(invaders.maxFront < invaders.front[c])
					invaders.maxFront = invaders.front[c];
			}

			// increase speed
			if(invaders.direction == right)
				invaders.speed += INVADERS_MAX_SPEED / (NUMBER_OF_ALIENS_X * NUMBER_OF_ALIENS_Y);
			else if(invaders.direction == left)
				invaders.speed -= INVADERS_MAX_SPEED / (NUMBER_OF_ALIENS_X * NUMBER_OF_ALIENS_Y);

			// check if all aliens killed
			if(invaders.killed >= NUMBER_OF_ALIENS_X * NUMBER_OF_ALIENS_Y)
			{
				*player_won = 1;
			}

//			prints("max front %d\n", invaders.maxFront);
//			prints("invaders front %d\n", invaders.front[x]);
//			prints("last column left %d\n", invaders.last_column_left);
//			prints("last column right %d\n", invaders.last_column_right);
//			prints("invader speed: %f\n", invaders.speed);
//			prints("killed: %d,%d front: %d total: %d\n", y,x,invaders.front[x],invaders.killed);
		}
	}
	xSemaphoreGive(invaders.lock);

	if (killed)
	{
		// handle scoring
		if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
		{
			if(y == 0) 	// Alien Type 1
			{
				game_wrapper.score += ALIEN_TYPE_1_SCORE_REWARD;
				game_wrapper.get_extra_life_scores += ALIEN_TYPE_1_SCORE_REWARD / 10;
			}
			else if(y < 3 && y > 0)		// Alien Type 2
			{
				game_wrapper.score += ALIEN_TYPE_2_SCORE_REWARD;
				game_wrapper.get_extra_life_scores += ALIEN_TYPE_2_SCORE_REWARD / 10;
			}
			else if(y < NUMBER_OF_ALIENS_Y && y > 2)	// Alien Type 3
			{
				game_wrapper.score += ALIEN_TYPE_3_SCORE_REWARD;
				game_wrapper.get_extra_life_scores += ALIEN_TYPE_3_SCORE_REWARD / 10;
			}

			vCheckForExtraLife();
		}
		xSemaphoreGive(game_wrapper.lock);

		vKillPlayerBullet();
	}
}


void vKillMothership(TickType_t * last_time_mothership, unsigned char* AI_control_ON)
{
	if(mothership.alive)
	{
		if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
		{
			game_wrapper.score += MOTHERSHIP_SCORE_REWARD;
			game_wrapper.get_extra_life_scores += MOTHERSHIP_SCORE_REWARD / 10;

			vCheckForExtraLife();
		}
		xSemaphoreGive(game_wrapper.lock);

		vKillPlayerBullet();

		if(*AI_control_ON)
		{
			player_bullet_status_t bullet_status = PASSIVE;
			xQueueSend(PlayerBulletModeXQueue, (void*)&bullet_status, 0);
		}

		mothership.alive = 0;
		*last_time_mothership = xTaskGetTickCount();
	}
}




void vDestructBunkerBlockPlayerSide(short bunkerNumber, short player_front, short xBlock)
{

	// check bunker block state
	if(bunker.bunkers[bunkerNumber].block_destruction_state[ player_front ][ xBlock ] > 0)
	{
		bunker.bunkers[bunkerNumber].block_destruction_state[ player_front ][ xBlock ]--;
	}
	// if bunker block is destroyed adjust front
	if(bunker.bunkers[bunkerNumber].block_destruction_state[player_front][xBlock] == 0)
	{
		if(player_front > 0)
			bunker.bunkers[bunkerNumber].player_front[xBlock]--;
	}

	vKillPlayerBullet();
}


void vDestructBunkerBlockInvadersSide(short bunkerNumber, short aliens_front, short xBlock)
{
	// check bunker block state
	if(bunker.bunkers[bunkerNumber].block_destruction_state[ aliens_front ][ xBlock ] > 0)
	{
		bunker.bunkers[bunkerNumber].block_destruction_state[ aliens_front ][ xBlock ] = 0;
	}

	// if bunker block is destroyed adjust front
	if(bunker.bunkers[bunkerNumber].block_destruction_state[aliens_front][xBlock] == 0)
	{
		if(aliens_front < NUM_BUNKER_BLOCK_Y - 1)
			bunker.bunkers[bunkerNumber].aliens_front[xBlock]++;
	}

	vKillInvadersBullet();
}



void vPlayerGotHit(unsigned char *invaders_won)
{
	vKillInvadersBullet();

	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{
		if(!game_wrapper.infinite_life_flag) {
			game_wrapper.remaining_life --;
			prints("player lose a life \n");
			if(game_wrapper.remaining_life <= 0)
			{
				prints("player lost too many lifes\n");
				if(*invaders_won == 0)
					*invaders_won = 1;
			}
		}
	}
	xSemaphoreGive(game_wrapper.lock);
}




void vCheckBulletBulletCollision(short invaders_bullet_pos_x, short invaders_bullet_pos_y, short player_bullet_pos_x, short player_bullet_pos_y)
{
	// check if player bullet hit invaders bullet

	if(invaders_bullet_pos_x > player_bullet_pos_x - BULLET_SIZE_X && invaders_bullet_pos_x < player_bullet_pos_x + BULLET_SIZE_X)
	{
		// Check if in future (3 Bullet size y ahead) it will be behind
		if(invaders_bullet_pos_y + BULLET_SIZE_Y * 3 > player_bullet_pos_y)
		{
			vKillPlayerBullet();
			vKillInvadersBullet();
		}
	}
}

void vCheckIfPlayerGotHit(short invaders_bullet_pos_x, short invaders_bullet_pos_y, short player_pos_x, short player_pos_y, unsigned char *invaders_won)
{
	// check if bullet reached player y
	if (invaders_bullet_pos_y > player_pos_y)
	{
//		prints("player bullet pos x: %d, player pos x: %d\n", invaders_bullet_pos_x, player_pos_x);
		if (invaders_bullet_pos_x >= player_pos_x - BULLET_SIZE_X && invaders_bullet_pos_x <= player_pos_x + PLAYER_SIZE_X)
		{
			vPlayerGotHit(invaders_won);
		}
	}
}

void vCheckIfBunkerGotHitInvaders(short invaders_bullet_pos_x, short invaders_bullet_pos_y, short player_pos_x, short player_pos_y)
{
	if (xSemaphoreTake(bunker.lock, portMAX_DELAY) == pdTRUE)
	{
		// check if bunker reached and not already passed
		if(invaders_bullet_pos_y > bunker.pos_y - BULLET_SIZE_Y && invaders_bullet_pos_y < bunker.pos_y + BUNKER_BLOCK_SIZE_Y * NUM_BUNKER_BLOCK_Y )
		{

			for(short bunkerNumber = 0; bunkerNumber < 4; bunkerNumber++)
			{
				// check at which bunker
				if(invaders_bullet_pos_x > bunker.bunkers[bunkerNumber].pos_x && invaders_bullet_pos_x < bunker.bunkers[bunkerNumber].pos_x + BUNKER_BLOCK_SIZE_X*NUM_BUNKER_BLOCK_X)
				{
					for( short xBlock = 0; xBlock < NUM_BUNKER_BLOCK_X; xBlock++)
					{
						// check column of bunker
						if((invaders_bullet_pos_x > bunker.bunkers[bunkerNumber].pos_x + xBlock * BUNKER_BLOCK_SIZE_X - BULLET_SIZE_X) && (invaders_bullet_pos_x < bunker.bunkers[bunkerNumber].pos_x + BUNKER_BLOCK_SIZE_X + xBlock * BUNKER_BLOCK_SIZE_X ))
						{
							if(bunker.bunkers[bunkerNumber].block_destruction_state[bunker.bunkers[bunkerNumber].player_front[xBlock]][xBlock] > 0)
							{
								vDestructBunkerBlockInvadersSide(bunkerNumber, bunker.bunkers[bunkerNumber].aliens_front[xBlock], xBlock);
							}
						}
//							prints("x column: %d\n",t);
					}
//						prints("bunker number: %d\n",s);
				}
			}
		}
	}
	xSemaphoreGive(bunker.lock);
}




void vCheckAliensBulletCollision(unsigned char *invaders_won)
{
	short player_pos_x = 0;
	short player_pos_y = 0;


	short invaders_bullet_alive = 0;
	short invaders_bullet_pos_y = SCREEN_HEIGHT;
	short invaders_bullet_pos_x = 0;

	unsigned char player_bullet_alive = 0;
	short player_bullet_pos_x = 0;
	short player_bullet_pos_y = 0;

	// fetch invaders bullet position
	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
	{
		invaders_bullet_alive = invaders.bullet.alive;
		invaders_bullet_pos_y = invaders.bullet.pos_y;
		invaders_bullet_pos_x = invaders.bullet.pos_x;
	}
	xSemaphoreGive(invaders.lock);


	if(invaders_bullet_alive)
	{
		// fetch player position and player bullet position
		if (xSemaphoreTake(player.lock, portMAX_DELAY) == pdTRUE)
		{
			player_pos_x = player.pos_x;
			player_pos_y = player.pos_y;
			player_bullet_alive = player.bullet.alive;
			player_bullet_pos_x = player.bullet.pos_x;
			player_bullet_pos_y = player.bullet.pos_y;
		}
		xSemaphoreGive(player.lock);

		if(player_bullet_alive)
			vCheckBulletBulletCollision(invaders_bullet_pos_x, invaders_bullet_pos_y, player_bullet_pos_x, player_bullet_pos_y);

		vCheckIfPlayerGotHit(invaders_bullet_pos_x, invaders_bullet_pos_y, player_pos_x, player_pos_y, invaders_won);
		vCheckIfBunkerGotHitInvaders(invaders_bullet_pos_x, invaders_bullet_pos_y, player_pos_x, player_pos_y);
	}
}



void vCheckIfInvadersGotHit(short invaders_pos_x, short invaders_pos_y, short player_bullet_pos_x, short player_bullet_pos_y, unsigned char * player_won, short * invaders_front)
{
	// check if invaders block reached and not already passed
	if(player_bullet_pos_y < invaders_pos_y + ALIEN_SIZE_Y + ALIEN_DISTANCE * NUMBER_OF_ALIENS_Y  && player_bullet_pos_y > invaders_pos_y - INVADERS_SIZE_Y / 2)
	{
		prints("block reached\n");
		for(short xAlien = 0; xAlien < NUMBER_OF_ALIENS_X; xAlien++)
		{
			// for every alien check x
			if( (player_bullet_pos_x > invaders_pos_x + xAlien * ALIEN_DISTANCE - BULLET_SIZE_X) && (player_bullet_pos_x < invaders_pos_x + ALIEN_SIZE_X + xAlien * ALIEN_DISTANCE ))
			{
				prints("x fits\n");
				// for every alien whose x the bullet is, check all y alien
				for(short yAlien = invaders_front[xAlien]; yAlien >= 0; yAlien--)
				{
					prints("checks yAlien %d, xAlien %d\n", yAlien, xAlien);
					if ((player_bullet_pos_y < invaders_pos_y + ALIEN_SIZE_Y + ALIEN_DISTANCE * yAlien) && (player_bullet_pos_y > invaders_pos_y + ALIEN_DISTANCE * yAlien ))
					{
						prints("y fits yAlien %d, xAlien %d\n", yAlien, xAlien);
						vKillAlien(yAlien, xAlien, player_won);
					}
				}
			}
		}
	}
}


void vCheckIfBunkerGotHitPlayer(short player_bullet_pos_x, short player_bullet_pos_y)
{
	if (xSemaphoreTake(bunker.lock, portMAX_DELAY) == pdTRUE)
	{
		// check if bunker reached and not already passed
		if(player_bullet_pos_y < bunker.pos_y + BUNKER_BLOCK_SIZE_Y * NUM_BUNKER_BLOCK_Y && player_bullet_pos_y > bunker.pos_y - BULLET_SIZE_Y)
		{

			for(short bunkerNumber = 0; bunkerNumber < 4; bunkerNumber++)
			{
				// check at which bunker
				if(player_bullet_pos_x > bunker.bunkers[bunkerNumber].pos_x && player_bullet_pos_x < bunker.bunkers[bunkerNumber].pos_x + BUNKER_BLOCK_SIZE_X*NUM_BUNKER_BLOCK_X)
				{
					for( short xBlock = 0; xBlock < NUM_BUNKER_BLOCK_X; xBlock++)
					{
						// check column
						if((player_bullet_pos_x > bunker.bunkers[bunkerNumber].pos_x + xBlock * BUNKER_BLOCK_SIZE_X - BULLET_SIZE_X) && (player_bullet_pos_x < bunker.bunkers[bunkerNumber].pos_x + BUNKER_BLOCK_SIZE_X + xBlock * BUNKER_BLOCK_SIZE_X ))
						{
							if(bunker.bunkers[bunkerNumber].block_destruction_state[bunker.bunkers[bunkerNumber].player_front[xBlock]][xBlock] > 0)
							{
								vDestructBunkerBlockPlayerSide(bunkerNumber, bunker.bunkers[bunkerNumber].player_front[xBlock], xBlock);
							}
						}
					}
				}
			}
		}
	}
	xSemaphoreGive(bunker.lock);
}


void vCheckIfMothershipGotHit(short player_bullet_pos_x, short player_bullet_pos_y, TickType_t * last_time_mothership, unsigned char* AI_control_ON)
{
	if (xSemaphoreTake(mothership.lock, portMAX_DELAY) == pdTRUE)
	{
		// check if mothership reached and not already passed
		if(player_bullet_pos_y < MOTHERSHIP_POS_Y + MOTHERSHIP_SIZE_Y && player_bullet_pos_y > MOTHERSHIP_POS_Y - BULLET_SIZE_Y*2)
		{
			if(player_bullet_pos_x > mothership.pos_x - BULLET_SIZE_X && player_bullet_pos_x < mothership.pos_x + MOTHERSHIP_SIZE_X)
			{
				vKillMothership(last_time_mothership, AI_control_ON);
			}
		}
	}
	xSemaphoreGive(mothership.lock);
}


void vCheckPlayerBulletCollision(unsigned char * player_won, TickType_t * last_time_mothership, unsigned char* AI_control_ON)
{
	short invaders_pos_y = 0;
	short invaders_pos_x = 0;
	short invaders_front[NUMBER_OF_ALIENS_X];

	short player_bullet_alive = 0;
	short player_bullet_pos_y = SCREEN_HEIGHT;
	short player_bullet_pos_x = 0;

	// fetch player bullet information
	if (xSemaphoreTake(player.lock, portMAX_DELAY) == pdTRUE)
	{
		player_bullet_alive = player.bullet.alive;
		player_bullet_pos_y = player.bullet.pos_y;
		player_bullet_pos_x = player.bullet.pos_x;
	}
	xSemaphoreGive(player.lock);

	if(player_bullet_alive)
	{
		// fetch invaders information
		if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
		{
			invaders_pos_y = invaders.pos_y;
			invaders_pos_x = invaders.pos_x;
			for (unsigned char u = 0; u < NUMBER_OF_ALIENS_X; u++)
			{
				invaders_front[u] = invaders.front[u];
			}
		}
		xSemaphoreGive(invaders.lock);

		vCheckIfInvadersGotHit(invaders_pos_x, invaders_pos_y, player_bullet_pos_x, player_bullet_pos_y, player_won, invaders_front);
		vCheckIfBunkerGotHitPlayer(player_bullet_pos_x, player_bullet_pos_y);
		vCheckIfMothershipGotHit(player_bullet_pos_x, player_bullet_pos_y, last_time_mothership, AI_control_ON);
	}

}


void vSetLastTimeAfterResume(TickType_t* last_time, TickType_t* last_time_mothership)
{
	if (xSemaphoreTake(invaders.lock, portMAX_DELAY) == pdTRUE)
	{
		if(invaders.resume)
		{
			prints("start with new last time\n");

			*last_time = xTaskGetTickCount();
			*last_time_mothership = xTaskGetTickCount();
			invaders.resume = 0;
		}

	}
	xSemaphoreGive(invaders.lock);
}



void vEndMatch(end_game_reason_t reason)
{

	if (xSemaphoreTake(game_wrapper.lock, portMAX_DELAY) == pdTRUE)
	{

		if(reason == INVADERS_WON)	// invaders won
		{
			if(game_wrapper.highscore < game_wrapper.score) game_wrapper.highscore = game_wrapper.score;
			game_wrapper.next_level_flag = 0;
			game_wrapper.level = 0;

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
			game_wrapper.level = 0;
			game_wrapper.next_level_flag = 0;
		}

		xSemaphoreGive(game_wrapper.lock);
	}
}



void vCheckForExtraLife()
{
	if(game_wrapper.get_extra_life_scores > 150)
	{
		if(game_wrapper.remaining_life < 3)
			game_wrapper.remaining_life++;

		game_wrapper.get_extra_life_scores = 0;
	}
}


void vGameHandlerTask(void *pvParameters)
{

	unsigned char invaders_won = 0;
	unsigned char player_won = 0;
	unsigned char moving_left = 0;
	unsigned char moving_right = 0;
	unsigned char AI_control_ON = 0;
	TickType_t last_time = 0;
	TickType_t last_time_mothership = 0;

	while(1){

		vSetLastTimeAfterResume(&last_time, &last_time_mothership);
		vMoveInvaders(&invaders_won, &last_time);
		vMoveMothership(&last_time_mothership, &AI_control_ON);

		vMovePlayer(&moving_left, &moving_right, AI_control_ON);
		vCheckPlayerBulletCollision(&player_won, &last_time_mothership, &AI_control_ON);
		vCheckAliensBulletCollision(&invaders_won);
		vAlienShoot();


		if(invaders_won)
		{
			vEndMatch(INVADERS_WON);
			invaders_won = 0;

			prints("invaders won.\n");

			vTaskResume(DrawPopUpPageTask);
		}

		if(player_won)
		{
			vEndMatch(PLAYER_WON);
			player_won = 0;

			prints("player won.\n");

			if (StateQueue) xQueueSend(StateQueue, &four_state_signal, 0);
		}

		vTaskDelay((TickType_t)20); // Basic sleep of 20ms
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
    DifficultyQueue = xQueueCreate(5, sizeof(unsigned char));
    if (!DifficultyQueue) {
        exit(EXIT_FAILURE);
    }

    NextKeyQueue = xQueueCreate(1, sizeof(opponent_cmd_t));
    if (!NextKeyQueue) {
        exit(EXIT_FAILURE);
    }


    if (xTaskCreate(vInitGameTaks, "InitGameTaks",
    						mainGENERIC_STACK_SIZE * 3, NULL, mainGENERIC_PRIORITY + 3,
    						&InitGameTaks) != pdPASS) {
        PRINT_TASK_ERROR("InitGameTaks");
        goto err_InitGameTaks;
    }

    if (xTaskCreate(vGameHandlerTask, "GameHandlerTask",
    						mainGENERIC_STACK_SIZE * 3, NULL, mainGENERIC_PRIORITY + 3,
    						&GameHandlerTask) != pdPASS) {
        PRINT_TASK_ERROR("GameHandlerTask");
        goto err_GameHandlerTask;
    }

    if (xTaskCreate(vDrawPopUpPageTask, "DrawPopUpPageTask",
    						mainGENERIC_STACK_SIZE * 3, NULL, mainGENERIC_PRIORITY + 3,
    						&DrawPopUpPageTask) != pdPASS) {
        PRINT_TASK_ERROR("DrawPopUpPageTask");
        goto err_DrawPopUpPageTask;
    }

    if (xTaskCreate(vUDPControlTask, "UDPControlTask",
                    mainGENERIC_STACK_SIZE, NULL, mainGENERIC_PRIORITY,
                    &UDPControlTask) != pdPASS) {
        PRINT_TASK_ERROR("UDPControlTask");
        goto err_udpcontrol;
    }


    vTaskSuspend(GameHandlerTask);
    vTaskSuspend(InitGameTaks);
    vTaskSuspend(UDPControlTask);
    vTaskSuspend(DrawPopUpPageTask);


    return 0;


err_udpcontrol:
	vTaskDelete(DrawPopUpPageTask);
err_DrawPopUpPageTask:
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
	vTaskDelete(GameHandlerTask);
err_GameHandlerTask:
	vTaskDelete(InitGameTaks);
err_InitGameTaks:

	return -1;
}
