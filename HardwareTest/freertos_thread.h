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
#include <atomic>

// There are not precise milliseconds, can be somewhat more or somewhat less
static inline void busy_wait(int us) {
  std::atomic_int cnt;

  const long long end_cycles = ((long long)us) * ((long long)configCPU_CLOCK_HZ);
  const int end_us = end_cycles / 100000000LL; // empirical quotient

  for (cnt = 0; cnt < end_us; cnt++);
};

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
                 int prio = 1) :
    name{name},
    stack_size{stack_size},
    prio{prio}
  {};
};

#endif /* FREERTOS_THREAD_H */
