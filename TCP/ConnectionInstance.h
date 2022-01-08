#ifndef TCP_ConnectionInstance_H
#define TCP_ConnectionInstance_H

#include "packet_utils.h"
#include "sys/types.h"
#include <ctime>

#define RWND 8
#define BUFFER_SIZE RWND * 500
#define MSS 500
#define TIMEOUT_DELAY_SEC 1;

class ConnectionInstance
{
private:
    uint32_t curr_seq_no;
    int connection_sockfd;
    struct sockaddr_storage their_addr;
    double loss_prob;
    uint32_t duplicate_ack = 0;
    uint32_t CWND = 8;
    uint32_t packets_sent;
    bool three_duplicate = false;
    bool slow_start = false;
    uint32_t start = 0;
    uint32_t end = 0;
    uint32_t window = RWND;
    struct timeval tv
    {
    };

    /**
     * Resolve the server's address into @param p,
     * create a socket @param listener_sockfd
     * and bind it to @param port
     * */
    void setup_socket(struct addrinfo *p, int *listener_sockfd, const char *port);
    static void *get_in_addr(struct sockaddr *sa);
    void next_seq_no();

public:
    /**
     * Initialises data and establishes connection :)!
     * @param filename the received filename, allocated using malloc
     * @param seconds the time out in seconds
     * @param port the port to listen on
     * @param seed used for simulation
     * */
    ConnectionInstance(time_t seconds, const char *port, char **filename, long seed, double loss_prob);
    ~ConnectionInstance();
    void tcp_send(void *data, int length, bool is_last);
    void tcp_receive_ack(int number_of_acks);
    void duplicate_ack_handler();
    void timer_handler();
};

#endif //TCP_ConnectionInstance_H
