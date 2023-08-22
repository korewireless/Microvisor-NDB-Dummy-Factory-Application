/**
 *
 * Microvisor Hardware Test Sample
 * Version 1.0.0
 * Copyright © 2023, Kore Wireless
 * Licence: Apache 2.0
 *
 */

#ifndef GPIO_SHORTS_TEST_H
#define GPIO_SHORTS_TEST_H

#include "freertos_thread.h"
#include "queue.h"

class GpioShortsTest: public FreeRTOSThread {
private:
  QueueHandle_t in_queue;
  QueueHandle_t out_queue;

  bool got_start_test_message();
  void run();

public:
  GpioShortsTest(QueueHandle_t in_queue_arg, QueueHandle_t out_queue_arg) :
    FreeRTOSThread("GpioShortsTestTask"),
    in_queue{in_queue_arg},
    out_queue{out_queue_arg}
  {
    start_task();
  }
};

#endif // GPIO_SHORTS_TEST_H
