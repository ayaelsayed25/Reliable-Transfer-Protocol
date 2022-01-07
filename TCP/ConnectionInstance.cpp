#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "ConnectionInstance.h"
#include <stdlib.h>

void *ConnectionInstance ::get_in_addr(struct sockaddr *sa)
{
    return &(((struct sockaddr_in *)sa)->sin_addr);
}

void ConnectionInstance ::setup_socket(struct addrinfo *ret_p, int *listener_sockfd, const char *port)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = servinfo; p != nullptr; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == nullptr)
    {
        fprintf(stderr, "server: failed to bind socket\n");
        exit(1);
    }

    *listener_sockfd = sockfd;
    memcpy(ret_p, p, sizeof(struct addrinfo));
    freeaddrinfo(servinfo);
}

ConnectionInstance ::ConnectionInstance(time_t seconds, const char *port, char **filename, long seed, double prob) : connection_sockfd(-1),
                                                                                                                     curr_seq_no(0),
                                                                                                                     loss_prob(prob)
{

    /* Set up RNG*/
    srand48(seed);

    struct addrinfo p;
    int listener_sockfd;
    setup_socket(&p, &listener_sockfd, port);

    ssize_t numbytes;
    socklen_t addr_len;

    struct packet send_packet;
    struct packet recv_packet;

    char buf[MAX_PACKET_SIZE];
    char s[INET6_ADDRSTRLEN];
    //do {
    printf("listening..\n");

    addr_len = sizeof their_addr;
    memset(&their_addr, 0, addr_len);
    do
    {
        numbytes = recvfrom(listener_sockfd,
                            buf,
                            MAX_PACKET_SIZE,
                            0,
                            (struct sockaddr *)&their_addr, &addr_len);
        if (numbytes == -1 && errno != EINTR)
        {
            perror("recvfrom");
            exit(1);
        }
    } while (numbytes == -1);

    /* Parse Message */
    {
        bool is_corrupt, is_last;
        parse_packet(&recv_packet, &is_corrupt, &is_last, buf, numbytes);
        *filename = (char *)malloc(recv_packet.len + 1);
        memcpy(*filename, recv_packet.data, recv_packet.len);
        (*filename)[recv_packet.len] = '\0';
        printf("filename: %s\n", *filename);
    }

    printf("ACKing connection..\n");
    pack_packet(&send_packet, nullptr, 0, false, recv_packet.seq_no);
    print_packet(&send_packet);
    // assuming ACKs never get lost
    if ((numbytes = sendto(listener_sockfd, &send_packet, HEADER_SIZE, 0,
                           (struct sockaddr *)&their_addr, addr_len)) == -1)
    {
        perror("sendto");
        exit(1);
    }
    //} while (fork());

    printf("in child\n");
    //close(listener_sockfd);
    connection_sockfd = listener_sockfd;

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
    printf("connecting to %s\n", s);

    /*connection_sockfd = socket(p.ai_family, p.ai_socktype, p.ai_protocol);
    if (connection_sockfd == -1){
        perror("child: socket");
        exit(1);
    }*/

    /* Set timeout */
    struct timeval timeout
    {
        seconds, 0
    };
    if (setsockopt(connection_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) == -1)
    {
        perror("setsockopt");
        exit(1);
    }

    /*if(bind(connection_sockfd, p.ai_addr, p.ai_addrlen) == -1){
        close(connection_sockfd);
        perror("child: bind");
        exit(1);
    }*/

    if (connect(connection_sockfd, (struct sockaddr *)&their_addr, addr_len) == -1)
    {
        perror("connect");
        exit(1);
    }

    //begin sending file
    FILE *in = fopen(*filename, "r");
    int num_read;
    uint8_t buffer[BUFFER_SIZE];
    uint8_t packet[500];
    uint32_t start = 0;
    uint32_t window = CWND < RWND ? CWND : RWND;
    uint32_t end = window;
    uint32_t last_packet_size = 0;
    num_read = fread(buffer, sizeof(uint8_t), (end - start) * 500, in);
    //GO-BACK-N
    while (!feof(in))
    {
        printf("Beginning of window");
        if (num_read < (end - start) * 500)
        {
            end = num_read / 500;
            last_packet_size = num_read % 500;
        }
        /* Restart timer */
        for (int i = start; i < end; i += 1)
        {
            for (int j = 0; j < 500; j++)
            {
                packet[j] = buffer[i * 500 + j];
            }
            tcp_send(packet, 500, feof(in));
            tcp_receive_ack();
        }
        if (last_packet_size > 0)
        {
            tcp_send(packet, last_packet_size, feof(in));
            tcp_receive_ack();
        }

        //after window
        //update cwnd
        window = CWND < RWND ? CWND : RWND;
        //last acknowledged packet
        num_read = fread(buffer, sizeof(uint8_t), (last_ack - start) * 500, in);
        curr_seq_no = (last_ack + 1) % RWND;
        start = last_ack + 1;
        //update buffer
        end = start + window;
        printf("\nwindoww endd\n");
    }
    fclose(in);
}

void ConnectionInstance ::tcp_send(void *data, int len, bool is_last)
{
    if (len > 500)
    {
        fprintf(stderr, "Length of data to end exceeds 500 %d\n", len);
        exit(1);
    }

    struct packet send_packet
    {
    };
    pack_packet(&send_packet, (uint8_t *)data, len, is_last, curr_seq_no);
    // TODO corruption
    double prob = drand48();
    // printf("Should loss happen? %f\n", prob);
    if (prob <= loss_prob)
    {
        printf("Dropping packet %d\n", curr_seq_no);
    }
    else
    {
        printf("sending packet %d\n", curr_seq_no);
    }
    if (prob > loss_prob && send(connection_sockfd, &send_packet, len + HEADER_SIZE, 0) == -1)
    {
        perror("send");
    }
}

void ConnectionInstance::next_seq_no()
{
    //implement for window size
    curr_seq_no = (curr_seq_no + 1) % RWND;
}

void ConnectionInstance::tcp_receive_ack()
{

    void *buf[MAX_PACKET_SIZE];
    struct packet recv_packet
    {
    };
    uint8_t numbytes = recv(connection_sockfd, buf, MAX_PACKET_SIZE, 0);

    if (numbytes == -1)
    {
        //number of ACKs received less than sent
        perror("recv");
    }
    else
    {

        bool is_corrupt;
        parse_packet(&recv_packet, &is_corrupt, nullptr, buf, numbytes);
        //is_corrupt is discarded from client ??
        // if (is_corrupt || recv_packet.seq_no != curr_seq_no)
        printf("received packet %d\n", recv_packet.seq_no);

        if (recv_packet.seq_no != curr_seq_no)
        {
            // if (prev_ack == recv_packet.seq_no)
            //     duplicate_ack++;
            // else
            //     duplicate_ack = 0;
            // if (duplicate_ack == 3)
            //     three_duplicate = true;
            // //send same packet
            //     continue; //not exactly how the FSM works but for the time being
        }
        else
        {
            last_ack = curr_seq_no;
        }
        next_seq_no();
    }
}

void ConnectionInstance::timer_handler()
{
    //timeout

    if (acks_reveived == 0)
    {
    }
    else if (three_duplicate)
    {
        CWND = CWND >> 1;
    }
    three_duplicate = false;
}

ConnectionInstance ::~ConnectionInstance()
{
    if (connection_sockfd == -1)
        close(connection_sockfd);
}
