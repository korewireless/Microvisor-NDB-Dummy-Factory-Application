/**
 *
 * Microvisor Log Helper
 * Version 1.0.0
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */

#ifndef LOGGER_H
#define LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

void server_log_init();
void server_log(const char* format_string, ...);

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_H */
