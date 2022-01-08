#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "ConnectionInstance.h"
#include <stdlib.h>
#include <fstream>

unsigned int b = 0;

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
    tv.tv_sec = TIMEOUT_DELAY_SEC;
    tv.tv_usec = 0;
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
    uint8_t packet[MSS];
    uint32_t window = CWND < RWND ? CWND : RWND;
    uint32_t last_packet_size = 0;

    fseek(in, 0L, SEEK_END);
    int file_size = ftell(in);
    rewind(in);
    uint32_t packets_sent = file_size / MSS;
    last_packet_size = file_size % MSS;
    printf("File size is %d bytes,, Sending it in %d packets \n", file_size, packets_sent);
    //GO-BACK-N
    //buffer iterator
    start = 0;
    end = window;
    while (b < packets_sent)
    {
        fseek(in, b * MSS, SEEK_SET);
        //start window
        int i;
        for (i = 0; i < window; i++)
        {
            if (b == packets_sent)
                break;
            num_read = fread(packet, sizeof(uint8_t), MSS, in);
            tcp_send(packet, MSS, feof(in));
        }
        //after window
        //update cwnd
        window = CWND < RWND ? CWND : RWND;
        printf("\nEnd of window\n\n");
        tcp_receive_ack(i);
    }
    if (last_packet_size > 0)
    {
        num_read = fread(packet, sizeof(uint8_t), last_packet_size, in);
        tcp_send(packet, last_packet_size, true);
        tcp_receive_ack(1);
    }
    fclose(in);
}

void ConnectionInstance ::tcp_send(void *data, int len, bool is_last)
{
    if (len > MSS)
    {
        fprintf(stderr, "Length of data to end exceeds MSS %d\n", len);
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
    next_seq_no();
}

void ConnectionInstance::next_seq_no()
{
    //implement for window size
    curr_seq_no = curr_seq_no + 1;
}

void ConnectionInstance::tcp_receive_ack(int number_of_acks)
{
    //reset timer
    if (setsockopt(connection_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) == -1)
    {
        perror("setsockopt");
        exit(1);
    }
    void *buf[MAX_PACKET_SIZE];
    struct packet recv_packet
    {
    };
    for(int i=0; i<number_of_acks; i++){
        uint8_t numbytes = recv(connection_sockfd, buf, MAX_PACKET_SIZE, 0);

        if (numbytes == -1)
        {
            //timeout
            perror("recv");
        }
        else
        {
            bool is_corrupt;
            parse_packet(&recv_packet, &is_corrupt, nullptr, buf, numbytes);
            //is_corrupt is discarded from client ??
            // if (is_corrupt || recv_packet.seq_no != curr_seq_no)
            printf("received packet %d, b = %d\n", recv_packet.seq_no, b);
            uint16_t expected = end - start;
            uint16_t received = recv_packet.seq_no - start + 1;

            printf("recv_packet.seq_no: %d, start: %d, end:%d\n", recv_packet.seq_no, start, end);
            printf("expected %d, received %d\n", expected, received);

            //update congestion window, start, end
            start = recv_packet.seq_no;
            curr_seq_no = start;
            end = start + window;
            b += received;
        }
    }
}

// void ConnectionInstance::timer_handler()
// {
//     //timeout

//     if (acks_reveived == 0)
//     {
//     }
//     else if (three_duplicate)
//     {
//         CWND = CWND >> 1;
//     }
//     three_duplicate = false;
// }

ConnectionInstance ::~ConnectionInstance()
{
    if (connection_sockfd == -1)
        close(connection_sockfd);
}
