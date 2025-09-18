/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>


#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

SemaphoreHandle_t xSemaphoreLedR;
QueueHandle_t xQueue_r;

SemaphoreHandle_t xSemaphoreLedY;
QueueHandle_t xQueue_y;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) {
        if (gpio == BTN_PIN_R){
            xSemaphoreGiveFromISR(xSemaphoreLedR, 0);
        }
        else if (gpio == BTN_PIN_Y) {
            xSemaphoreGiveFromISR(xSemaphoreLedY, 0);
        }
    }
}

void led_r_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int delay = 0;

    while (true) {
        xQueueReceive(xQueue_r, &delay, 0);
        if (delay > 0) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        }
    }
}

void led_task_y(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);
    
    int delay = 0;

    while (true) {
        xQueueReceive(xQueue_y, &delay, 0);
        if (delay > 0) {
            gpio_put(LED_PIN_Y, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        }
    }
}

void btn_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    int flag = 0;
    int delay;
    while (true) {
        if (xSemaphoreTake(xSemaphoreLedR, pdMS_TO_TICKS(500)) == pdTRUE) {
            flag = !flag;
            delay = flag ? 100 : 0;
            xQueueSend(xQueue_r, &delay, 0);
        } 

        flag = 0;

        if (xSemaphoreTake(xSemaphoreLedY, pdMS_TO_TICKS(500)) == pdTRUE) {
            flag = !flag;
            delay = flag ? 100 : 0;
            xQueueSend(xQueue_y, &delay, 0);
        }
    }
}


int main() {
    stdio_init_all();
    printf("Start RTOS\n");

    xSemaphoreLedR = xSemaphoreCreateBinary();
    xSemaphoreLedY = xSemaphoreCreateBinary();
    xQueue_r = xQueueCreate(32, sizeof(int));
    xQueue_y = xQueueCreate(32, sizeof(int));

    xTaskCreate(led_r_task, "LED_Task R", 256, NULL, 1, NULL);
    xTaskCreate(led_task_y, "LED_Task Y", 256, NULL, 1, NULL);
    xTaskCreate(btn_task, "BTN_Task R", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1){}

    return 0;
}