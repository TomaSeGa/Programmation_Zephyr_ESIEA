/**
  ******************************************************************************
  * @file    main.c
  * @author  P. COURBIN
  * @version V2.0
  * @date    08-12-2023
  * @brief   WithGUI version
  ******************************************************************************
*/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#include "display.hpp"

#define DELAY_START_TIME_MS 5000

int nbWriters = 5; //nb de thread a créer

#define STACKSIZE (4096)
static K_THREAD_STACK_DEFINE(lecteur_stack, STACKSIZE);
static K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, 10, STACKSIZE);

#define PERIOD_DISPLAY_TASK 1000

myDisplay display;

char my_msgq_buffer[10 * sizeof(char[50])];
struct k_msgq my_msgq;

struct k_thread threads[10];

K_MUTEX_DEFINE(mutex_msgq);

static void lecteur_task(void *p1, void *p2, void *p3)
{
	int period = PERIOD_DISPLAY_TASK;
	struct k_timer timer;
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, K_MSEC(0), K_MSEC(period));
	while(1)
	{
		k_timer_status_sync(&timer);
		char receive[50] = {0};
		while(k_msgq_get(&my_msgq, &receive, K_FOREVER) == 0)// tant que la message queue n'est pas vide
		{
			display.text_add(receive);
		}
	}
}

static void generic_redacteur_taks(void *p1, void *p2, void *p3)
{
	int period = PERIOD_DISPLAY_TASK;

	char text[50] = {0};
	int thread_id = (int)p1;

	uint32_t start;
	struct k_timer timer;
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, K_MSEC(0), K_MSEC(period));
	while (1)
	{
		k_timer_status_sync(&timer);
		start = k_uptime_get_32();
		display.task_handler();

		// format the string 
    	snprintf(text, sizeof(text), "redacteur nb:%d is running", thread_id);
		k_msgq_put(&my_msgq, &text, K_NO_WAIT);
	}
}

int main(void)
{
	struct k_thread lecteur_t;

	int i;

	k_msgq_init(&my_msgq, my_msgq_buffer, sizeof(char[50]), 10);

	display.init(false); //mise a false pour ne pas avoir de graph

	//création thread lecteur
	k_thread_create(&lecteur_t, lecteur_stack, K_THREAD_STACK_SIZEOF(lecteur_stack), lecteur_task,
					 NULL, NULL, NULL, 1, 0, K_NO_WAIT);

	// boucle creation thread
	for (int i = 0; i < nbWriters; i++) {
        k_tid_t thread_id = k_thread_create(&threads[i],
                                           thread_stacks[i],
                                           K_THREAD_STACK_SIZEOF(thread_stacks[i]),
                                           generic_redacteur_taks,
                                           (void *)i, NULL, NULL,
                                           2, 0, K_NO_WAIT);
        k_thread_name_set(&threads[i], "Thread redacteur");
    }
}