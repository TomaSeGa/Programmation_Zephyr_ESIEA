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
#include "bme680.hpp"

//K_MSGQ_DEFINE(my_msgq, sizeof(char[50]), 10, 1); // message queue 

#define STACKSIZE (4096)
static K_THREAD_STACK_DEFINE(display_stack, STACKSIZE);
static K_THREAD_STACK_DEFINE(bme680_stack, STACKSIZE);
static K_THREAD_STACK_DEFINE(display_esiea_stack, STACKSIZE);

static K_THREAD_STACK_DEFINE(hello_stack, 2048);

#define PRIO_DISPLAY_TASK 1
#define PRIO_BME680_TASK 2
#define PRIO_DISPLAY_ESIEA 3 // bug avec 2 
#define PRIO_DISPLAY_HELLO 4

#define PERIOD_DISPLAY_TASK 1000
#define PERIOD_BME680_TASK 500

myDisplay display;
myBME680 bme680;

struct k_thread display_esiea_t;
k_tid_t display_esiea;

char my_msgq_buffer[10 * sizeof(char[50])];
struct k_msgq my_msgq;

K_MUTEX_DEFINE(mutex_msgq); // voir si devoir le mettre en place.

static void display_task(void *p1, void *p2, void *p3)
{
	char name[10] = "DISPLAY";
	k_tid_t tid = k_current_get();
	int period = PERIOD_DISPLAY_TASK;

	char text[50] = {0};
	uint32_t start;

	struct k_timer timer;
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, K_MSEC(0), K_MSEC(period));
	LOG_INF("Run task DISPLAY - Priority %d - Period %d\n", k_thread_priority_get(tid), period);
	while (1)
	{
		k_timer_status_sync(&timer);
		LOG_INF("START task %s", name);
		start = k_uptime_get_32();
		display.task_handler();
		display.chart_add_temperature(bme680.get_temperature());
		display.chart_add_humidity(bme680.get_humidity());

		// format the string 
		sprintf(text, "%d.%02d°C - %d.%02d\%", bme680.temperature.val1, bme680.temperature.val2 / 10000, bme680.humidity.val1, bme680.humidity.val2 / 10000);
		k_msgq_put(&my_msgq, &text, K_NO_WAIT);

		LOG_INF("END task %s - %dms", name, k_uptime_get_32() - start);
	}
}
static void bme680_task(void *p1, void *p2, void *p3)
{
	char name[10] = "BME680";
	k_tid_t tid = k_current_get();
	int period = PERIOD_BME680_TASK;

	uint32_t start;

	struct k_timer timer;
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, K_MSEC(0), K_MSEC(period));
	LOG_INF("Run task BME680 - Priority %d - Period %d\n", k_thread_priority_get(tid), period);
	while (1)
	{
		k_timer_status_sync(&timer);
		LOG_INF("START task %s", name);
		start = k_uptime_get_32();
		bme680.update_values();
		LOG_INF("END task %s - %dms", name, k_uptime_get_32() - start);		
	}
}

static void display_task_esiea(void *p1, void *p2, void *p3)
{
	int period = PERIOD_DISPLAY_TASK;
	struct k_timer timer;
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, K_MSEC(0), K_MSEC(period));
	while(1)
	{
		k_timer_status_sync(&timer);
		// tant que la message queue n'est pas vide
		char receive[50] = {0};
		while(k_msgq_get(&my_msgq, &receive, K_FOREVER) == 0)
		{
			display.text_add(receive);
		}
	}
}

static void display_task_hello(void *p1, void *p2, void *p3)
{
	int period = PERIOD_DISPLAY_TASK;
	struct k_timer timer;
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, K_MSEC(0), K_MSEC(period));
	while(1)
	{
		k_timer_status_sync(&timer);

		char text[50] = "Hello!";
		k_msgq_put(&my_msgq, &text, K_NO_WAIT);
	}
}
	
int main(void)
{
	struct k_thread display_t;
	struct k_thread bme680_t;
	struct k_thread hello_t;

	k_msgq_init(&my_msgq, my_msgq_buffer, sizeof(char[50]), 10);

	display.init(true);
	bme680.init();
	
	k_thread_create(&display_t, display_stack, K_THREAD_STACK_SIZEOF(display_stack),
					display_task, NULL, NULL, NULL,
					PRIO_DISPLAY_TASK, 0, K_NO_WAIT);

	k_thread_create(&bme680_t, bme680_stack, K_THREAD_STACK_SIZEOF(bme680_stack),
					bme680_task, NULL, NULL, NULL,
					PRIO_BME680_TASK, 0, K_NO_WAIT);

	display_esiea = k_thread_create(&display_esiea_t, display_esiea_stack, K_THREAD_STACK_SIZEOF(display_esiea_stack),
					display_task_esiea, NULL, NULL, NULL,
					PRIO_DISPLAY_ESIEA, 0, K_NO_WAIT);

	k_thread_create(&hello_t, hello_stack, K_THREAD_STACK_SIZEOF(hello_stack),
					display_task_hello, NULL, NULL, NULL,
					PRIO_DISPLAY_HELLO, 0, K_NO_WAIT);
}