#ifndef TCP_LOGGER_H
#define TCP_LOGGER_H

#include <cstdio>
#include <cstdint>
#include <ctime>

class Logger
{

private:
    FILE *logfile;
    int round;
    time_t start_time;

public:
    explicit Logger(FILE *logfile);
    void logLoss(uint32_t seq_no);
    void logTransmission(uint32_t seq_no);
    void logTimeout(uint32_t seq_no);
    void advanceRound();
    void logACK(uint32_t seq_no, int size);
    void window(int size);
};

#endif //TCP_LOGGER_H
