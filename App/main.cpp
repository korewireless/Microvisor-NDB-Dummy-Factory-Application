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

#include "messages.h"
#include "led_thread.h"
#include "system.h"
#include "logger.h"


int main(void) {
    /* Initialize the system */
    system_init();

    /* Initialize the logging */
    server_log_init();

    server_log("IN MAIN");

    /* Create the threads */

    // variables are needed so that the object is not destroyed until end of scope
    static auto led_thread = LedThread();
    (void) led_thread;

    vTaskStartScheduler();

    while (1) {}
}
