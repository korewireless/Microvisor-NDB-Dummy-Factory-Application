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
    int result_code;
};

enum TestResult {
    OkTestResult = 0,
    GpioHighTestResult = 1,
    InternalErrorTestResult = 2,
    GpioShortsTestTimeoutTestResult = 3,
    NetworkTimeoutTestResult = 4,
    HttpErrorTestResult = 4,
};
#endif // MESSAGES_H
