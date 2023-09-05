/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "freertos_thread.h"
#include "mv_syscalls.h"

#include "led_thread.h"
#include "system.h"

int main(void) {
    /* Initialize the system */
    system_init();

    /* Create the threads */

    // variables are needed so that the object is not destroyed until end of scope
    static auto led_thread = LedThread();
    (void) led_thread;

    vTaskStartScheduler();

    while (1) {}
}
