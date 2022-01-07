#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>

using namespace std;

#define NUM_THREADS 5

void *PrintHello(void *threadid)
{
    long tid;
    tid = (long)threadid;
    usleep(5);
    cout << "Hello World! Thread ID, " << tid << endl;
    pthread_exit(NULL);
}

int main()
{
    pthread_t thread;
    int rc;
    int i;

    cout << "main() : creating thread, " << i << endl;
    rc = pthread_create(&thread, NULL, PrintHello, (void *)i);

    if (rc)
    {
        cout << "Error:unable to create thread," << rc << endl;
        exit(-1);
    }

    pthread_exit(NULL);
}