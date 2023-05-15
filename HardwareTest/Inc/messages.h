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

struct HardwareTestResultMessage
{
    int result_code;
    char *failure_description; // only valid until next StartTest message
};

enum TestResult {
    OkTestResult = 0,
    GpioHighTestResult = 1,
    InternalErrorTestResult = 2,
    HardwareTestTimeoutTestResult = 3,
    NetworkTimeoutTestResult = 4,
    HttpErrorTestResult = 4,
};
#endif // MESSAGES_H
