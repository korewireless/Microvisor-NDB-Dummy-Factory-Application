/**
 *
 * Microvisor NDB Dummy Application
 * Version 1.0.0
 * Copyright © 2023, Kore Wireless
 * Licence: Apache 2.0
 *
 */

#ifndef LED_THREAD_H
#define LED_THREAD_H

#include "freertos_thread.h"
#include "queue.h"

class LedThread: public FreeRTOSThread {
private:
  void run();

public:
  LedThread() :
    FreeRTOSThread("LedThread")
  {
    start_task();
  }
};

#endif // LED_THREAD_H
