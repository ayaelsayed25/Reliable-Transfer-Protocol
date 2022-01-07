#ifndef TIMERHELPER_H_
#define TIMERHELPER_H_

#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <iostream>

using namespace std;

#define CLOCKID CLOCK_REALTIME
#define SIG SIGUSR1

typedef void (*TimerHandler)(sigval_t signum);

class TimerTimeoutHandler
{
public:
    virtual void handlerFunction(void) = 0;
};

class Timer
{
public:
    Timer(TimerTimeoutHandler *timeHandler);
    ~Timer();

    void setDuration(long int seconds);
    void start();
    void restart();
    void timeout();
    void stop();
    void callbackWrapper(void);
    static void timeOutHandler(sigval_t This);

private:
    void createTimer(timer_t *timerid, TimerHandler handler_cb);
    void startTimer(timer_t timerid, int startTimeout, int cyclicTimeout);
    void stopTimer(timer_t timerid);

    long int m_Duration;
    TimerTimeoutHandler *timeOutHandlerImp;
    timer_t timerid;
};

class TimeTimeoutHandlerImp : public TimerTimeoutHandler
{
public:
    TimeTimeoutHandlerImp() {}
    ~TimeTimeoutHandlerImp() {}

    void handlerFunction(void);
};

#endif /* TIMERHELPER_H_ */