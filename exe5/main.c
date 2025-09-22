/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <stdio.h>
#include <string.h>
#include <task.h>

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

SemaphoreHandle_t xSemaphoreLedR;
SemaphoreHandle_t xSemaphoreLedY;
QueueHandle_t xQueueBtn;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) {
        if (gpio == BTN_PIN_R) {
            xQueueSendFromISR(xQueueBtn, &BTN_PIN_R, 0);
        } else if (gpio == BTN_PIN_Y) {
            xQueueSendFromISR(xQueueBtn, &BTN_PIN_Y, 0);
        }
    }
}

void led_r_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int delay = 100;
    int status = 0;

    while (true) {
        if (status) {
            gpio_put(LED_PIN_R, 1);
        }
        if (xSemaphoreTake(xSemaphoreLedR, pdMS_TO_TICKS(delay)) == pdTRUE) {
            status = !status;
        }

        if (status) {
            gpio_put(LED_PIN_R, 0);
        }
        if (xSemaphoreTake(xSemaphoreLedR, pdMS_TO_TICKS(delay)) == pdTRUE) {
            status = !status;
        }

        if (!status) gpio_put(LED_PIN_R, 0);
    }
}

void led_y_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    int delay = 100;
    int status = 0;

    while (true) {
        if (status) {
            gpio_put(LED_PIN_Y, 1);
        }
        if (xSemaphoreTake(xSemaphoreLedY, pdMS_TO_TICKS(delay)) == pdTRUE) {
            status = !status;
        }

        if (status) {
            gpio_put(LED_PIN_Y, 0);
        }
        if (xSemaphoreTake(xSemaphoreLedY, pdMS_TO_TICKS(delay)) == pdTRUE) {
            status = !status;
        }

        if (!status) gpio_put(LED_PIN_Y, 0);
    }
}

void btn_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true,
                                       &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    while (true) {
        int btn = 0;
        xQueueReceive(xQueueBtn, &btn, 0);

        if (btn == BTN_PIN_R) xSemaphoreGive(xSemaphoreLedR);
        else if (btn == BTN_PIN_Y) xSemaphoreGive(xSemaphoreLedY);
    }
}

int main() {
    stdio_init_all();
    printf("Start RTOS\n");

    xSemaphoreLedR = xSemaphoreCreateBinary();
    xSemaphoreLedY = xSemaphoreCreateBinary();
    xQueueBtn = xQueueCreate(32, sizeof(int));

    xTaskCreate(led_r_task, "LED_Task R", 256, NULL, 1, NULL);
    xTaskCreate(led_y_task, "LED_Task Y", 256, NULL, 1, NULL);
    xTaskCreate(btn_task, "BTN_Task R", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1) {

    }

    return 0;
}