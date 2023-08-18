/**
 *
 * FreeRTOS thread wrapper
 * Version 1.0.0
 * Copyright © 2023, Kore Wireless
 * Licence: Apache 2.0
 *
 */

#ifndef FREERTOS_THREAD_H
#define FREERTOS_THREAD_H

#include "FreeRTOS.h"
#include "logger.h"
#include "task.h"

#include <cassert>

class FreeRTOSThread {
private:
  const char *name;
  const size_t stack_size;
  const int prio;

  static void callback(void* self) {
    ((FreeRTOSThread*)self)->run();
    vTaskDelete(nullptr);
  }

  virtual void run() = 0;

protected:
  void start_task() {
    auto res = xTaskCreate(FreeRTOSThread::callback, name, stack_size, this, prio, nullptr);
    assert(res);
  }

public:
  FreeRTOSThread(const char* name,
                 size_t stack_size = configMINIMAL_STACK_SIZE/sizeof(StackType_t),
                 int prio = 0) :
    name{name},
    stack_size{stack_size},
    prio{prio}
  {};
};

#endif /* FREERTOS_THREAD_H */
