#include <iostream>
#include "packet_utils.h"
#include "ConnectionInstance.h"
#include <csignal>
#include "sys/wait.h"
#include "unistd.h"
#include <cstdio>

void child_terminated(int signo, siginfo_t *siginfo, void *ucontext)
{
    int status;
    wait(&status);
    write(STDOUT_FILENO, "\nChild Process Terminated\n", 26);
}

int main()
{
    /* Set up SIGCHLD handler */
    struct sigaction sa;
    sa.sa_sigaction = child_terminated;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    /* Input Data */
    FILE *input = fopen("input.in", "r");
    if (input == NULL)
    {
        fprintf(stderr, "Cannot open command file");
        exit(1);
    }
    char *port = NULL, *filename = NULL;
    double loss_prob, corrupt_prob;
    long seed;
    {
        char *line = NULL;
        size_t len = 0;
        getline(&port, &len, input);
        strtok(port, "\r\n");

        len = 0;
        getline(&line, &len, input);
        printf("seed: %s", line);
        seed = atol(line);
        getline(&line, &len, input);
        printf("loss probability: %s", line);
        loss_prob = atof(line);

        getline(&line, &len, input);
        printf("corruption probability: %s\n", line);
        corrupt_prob = atof(line);

        free(line);
    }
    printf("port: %s\n", port);
    FILE *logfile = fopen("events.log", "w");
    Logger logger(logfile);
    ConnectionInstance sw(1, port, &filename, seed, loss_prob, corrupt_prob, logger);
    fclose(input);
    free(port);
    printf("%s", filename);
    fclose(logfile);
    return 0;
}
