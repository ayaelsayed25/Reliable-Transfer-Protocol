#include <iostream>
#include "packet_utils.h"
#include "ConnectionInstance.h"
#include <csignal>
#include "sys/wait.h"
#include "unistd.h"
#include <cstdio>
#include <sys/time.h>

void child_terminated(int signo, siginfo_t *siginfo, void *ucontext)
{
    int status;
    wait(&status);
    write(STDOUT_FILENO, "\nChild Process Terminated\n", 26);
}
unsigned int ACKs[RWND];
unsigned int Acks_reveived;

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
    char *port = NULL, *filename = NULL, *line = NULL;
    size_t len = 0;
    getline(&port, &len, input);
    strtok(port, "\r\n");

    len = 0;
    getline(&line, &len, input);
    printf("seed: %s", line);
    long seed = atol(line);
    len = 0;
    getline(&line, &len, input);
    printf("prob: %s\n", line);
    double prob = atof(line);

    printf("port: %s\n", port);
    ConnectionInstance sw(1, port, &filename, seed, prob);
    fclose(input);
    free(line);
    free(port);
    printf("%s", filename);

    return 0;
}
