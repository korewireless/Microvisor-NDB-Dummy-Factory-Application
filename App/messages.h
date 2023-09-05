#ifndef MESSAGES_H
#define MESSAGES_H

enum Message {
    NoneMessage = 0,
    JoinNetworkMessage,
    DropNetworkMessage,
    NetworkConnectedMessage,
    NetworkDisconnectedMessage,
    StartTestMessage
};

struct TestResultMessage
{
    uint32_t result_code;
};

enum TestResult {
    OkTestResult = 0,
    GpioHasShortsResult = 1,
    GpioLoopbackTimeoutResult = 2,
    GpioLoopbackFailureResult = 3,
    InternalErrorTestResult = 4,
    GpioShortsTestTimeoutTestResult = 5,
    NetworkTimeoutTestResult = 6,
    HttpErrorTestResult = 7,
};
#endif // MESSAGES_H
