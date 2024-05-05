/**
  ******************************************************************************
  * @file    main.c
  * @author  P. COURBIN
  * @version V2.0
  * @date    08-12-2023
  * @brief   Sample version
  ******************************************************************************
*/

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <string.h>

#include "display.hpp"
#include "adc.hpp"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

myDisplay display;
myADC adc;

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

#define DELAY_START_TIME_MS 5000

#define STACKSIZE (4096)
static K_THREAD_STACK_DEFINE(freiner_stack, STACKSIZE);
static K_THREAD_STACK_DEFINE(affichage_stack, STACKSIZE);
static K_THREAD_STACK_DEFINE(update_adc_stack, STACKSIZE);

#define PERIOD_DISPLAY_TASK 1000
#define PERIOD_ACQUIS_TASK 500

static void freiner_task (void *p1, void *p2, void *p3)
{
	char name[10] = "FREINER";
	k_tid_t tid = k_current_get();
	uint32_t start;

	int period = PERIOD_ACQUIS_TASK;
	struct k_timer timer;
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, K_MSEC(0), K_MSEC(period));
	while(1)
	{
		k_timer_status_sync(&timer);
		display.task_handler();
		gpio_pin_set_dt(&led0, 0);
    	gpio_pin_set_dt(&led1, 0);
		if(adc.get_value() > 30)
		{
			gpio_pin_set_dt(&led0, 1);
    		gpio_pin_set_dt(&led1, 1);
		}
	}
	printf("END task %s - %dms", name, k_uptime_get_32() - start);
}

static void affichage_task(void *p1, void *p2, void *p3)
{
	char name[10] = "AFFICHAGE";
	k_tid_t tid = k_current_get();
	uint32_t start;

	int period = PERIOD_DISPLAY_TASK;
	struct k_timer timer;
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, K_MSEC(0), K_MSEC(period));
	while(1)
	{
		k_timer_status_sync(&timer);
		display.task_handler();
		char text[30] = {0};
		snprintf(text, sizeof(text), "valeur ADC : %d", adc.get_value());
		display.text_add(text);
	}
	printf("END task %s - %dms", name, k_uptime_get_32() - start);
}

static void update_adc_task(void *p1, void *p2, void *p3)
{
	char name[10] = "UPDATE";
	k_tid_t tid = k_current_get();
	uint32_t start;

	int period = PERIOD_ACQUIS_TASK;
	struct k_timer timer;
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, K_MSEC(0), K_MSEC(period));
	while(1)
	{
		k_timer_status_sync(&timer);
		adc.update_value();
	}
	printf("END task %s - %dms", name, k_uptime_get_32() - start);
}

uint8_t init_leds()
{
    uint8_t returned = 0;
    if (!device_is_ready(led0.port) || !device_is_ready(led1.port))
    {
        LOG_ERR("Error: LEDs devices are not ready (%s / %s)", led0.port->name, led1.port->name);
        returned = -1;
    }

    if (gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE) < 0 || gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE) < 0)
    {
        LOG_ERR("Error: LEDs config failed (%s / %s).", led0.port->name, led1.port->name);
        returned = -2;
    }
    return returned;
}

int main(void)
{
	struct k_thread affichage_t, frieren_t, update_t;

	display.init(false);
	adc.init();
	display.task_handler();
	init_leds();

	//creation de nos tâches
	k_thread_create(&affichage_t, affichage_stack, K_THREAD_STACK_SIZEOF(affichage_stack), 
					affichage_task,NULL, NULL, NULL, 1, 0, K_NO_WAIT);

	k_thread_create(&frieren_t, freiner_stack, K_THREAD_STACK_SIZEOF(freiner_stack), 
					freiner_task,NULL, NULL, NULL, 2, 0, K_NO_WAIT);
	
	k_thread_create(&update_t, update_adc_stack, K_THREAD_STACK_SIZEOF(update_adc_stack), 
					update_adc_task,NULL, NULL, NULL, 1, 0, K_NO_WAIT);
}
	