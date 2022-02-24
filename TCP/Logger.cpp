#include "Logger.h"
#include <iostream>

Logger::Logger(FILE *logfile) : logfile(logfile),
                                round(0),
                                start_time(time(NULL))
{
}

void Logger::logTransmission(uint32_t seq_no)
{
    fprintf(logfile, "%d\t%ld\ttransmitting %d\n", round, time(nullptr) - start_time, seq_no);
}

void Logger::logTimeout(uint32_t seq_no)
{
    fprintf(logfile, "%d\t%ld\ttimeout %d\n", round, time(nullptr) - start_time, seq_no);
}

void Logger::advanceRound()
{
    round++;
}

void Logger::logLoss(uint32_t seq_no)
{
    fprintf(logfile, "%d\t%ld\tlost %d\n", round, time(nullptr) - start_time, seq_no);
}

void Logger::logACK(uint32_t seq_no, int size)
{
    fprintf(logfile, "%d\t%ld\tclient correctly received %d with %d bytes\n",
            round, time(nullptr) - start_time, seq_no, size);
}

void Logger::window(int size)
{
    fprintf(logfile, "%d\t%ld\twindow %d\n", round, time(nullptr) - start_time, size);
}
