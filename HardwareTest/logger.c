/**
 *
 * Microvisor Log Helper
 * Version 1.0.0
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */


#include "logger.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "FreeRTOS.h"
#include "semphr.h"

// Microvisor includes
#include "mv_syscalls.h"

static SemaphoreHandle_t mutex;

static uint8_t log_buffer[1024] __attribute__((aligned(512))) = {0} ;

/**
 * @brief Issue any log message.
 *
 * @param format_string Message string with optional formatting
 * @param args          va_list of args from previous call
 */
static void do_log(const char* format_string, va_list args) {
    if(xSemaphoreTake(mutex, 10000)) {
      char buffer[512] = {0}; // beware of stack overflow

      // Write the formatted text to the message
      vsnprintf(buffer, sizeof(buffer) - 1, format_string, args);

      // Output the message using the system call
      mvServerLog((const uint8_t*)buffer, (uint16_t)strlen(buffer));
    }

    xSemaphoreGive(mutex);
}

/**
 * @brief Initialize logging.
 */

void server_log_init() {
    mutex = xSemaphoreCreateBinary();
    (void)mvServerLoggingInit(log_buffer, sizeof(log_buffer));
}

/**
 * @brief Issue a debug message.
 *
 * @param format_string Message string with optional formatting
 * @param ...           Optional injectable values
 */
void server_log(const char* format_string, ...) {
    va_list args;
    va_start(args, format_string);
    do_log(format_string, args);
    va_end(args);
}


