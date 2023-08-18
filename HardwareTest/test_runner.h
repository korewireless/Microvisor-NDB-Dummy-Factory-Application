/**
 *
 * Microvisor Test Runner
 * Version 1.0.0
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */

#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include "freertos_thread.h"
#include "queue.h"

class TestRunner: public FreeRTOSThread {
private:
  QueueHandle_t in_queue;
  QueueHandle_t out_queue;

  void run();

public:
  TestRunner(QueueHandle_t in_queue_arg, QueueHandle_t out_queue_arg) :
    FreeRTOSThread("TestRunnerTask"),
    in_queue{in_queue_arg},
    out_queue{out_queue_arg}
  {
    start_task();
  }
};

#endif /* NETWORK_H */
