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

ConnectionInstance::ConnectionInstance(time_t seconds, const char *port,
                                       char **filename, long seed,
                                       double prob, double corrupt_prob,
                                       const Logger &logger) : connection_sockfd(-1),
                                                               curr_seq_no(0),
                                                               loss_prob(prob),
                                                               corrupt_prob(corrupt_prob),
                                                               logger(logger)
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
    // printf("connecting to %s\n", s);
    printf("fmmm");

    /* Set timeout */
    tv.tv_sec = 0;
    tv.tv_usec = 500;
    printf("filkkkkkkkkkk");

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
    printf("fileeeee");
    int num_read;
    uint8_t packet[MSS];

    fseek(in, 0L, SEEK_END);
    int file_size = ftell(in);
    rewind(in);
    packets_sent = file_size / MSS;
    last_packet_size = file_size % MSS;
    if (last_packet_size > 0)
        packets_sent++;
    printf("laaaaast size %d\n", last_packet_size);
    printf("File size is %d bytes,, Sending it in %d packets \n", file_size, packets_sent);
    //GO-BACK-N
    //buffer iterator
    start = 0;
    end = window;
    while (b < packets_sent)
    {
        window = CWND < RWND ? CWND : RWND;
        fseek(in, b * MSS, SEEK_SET);
        //start window
        int i;
        for (i = 0; i < window; i++)
        {
            if (curr_seq_no == end)
                break;
            num_read = fread(packet, sizeof(uint8_t), MSS, in);
            tcp_send(packet, curr_seq_no == (packets_sent - 1) ? last_packet_size : MSS, feof(in));
        }
        //after window

        printf("\nEnd of window\n\n");

        tcp_receive_ack(window);
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

    double prob_loss = drand48();
    printf("Should loss happen? %s\n", (prob_loss < loss_prob) ? "true" : "false");
    if (prob_loss > this->loss_prob)
    {
        double prob_corrupt = drand48();
        uint16_t original_checksum = send_packet.checksum; // save correct checksum
        printf("Should corruption happen? %s\n", (prob_corrupt < corrupt_prob) ? "true" : "false");
        if (prob_corrupt < corrupt_prob)
        {
            //change checksum
            send_packet.checksum++;
        }

        // send
        logger.logTransmission(curr_seq_no);
        if (send(connection_sockfd, &send_packet, len + HEADER_SIZE, 0) == -1)
        {
            perror("send");
        }
        send_packet.checksum = original_checksum; // return checksum as it was
    }
    else
    {
        logger.logLoss(curr_seq_no);
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
    recv_packet.seq_no = start - 1;
    u_int32_t acks_received = 0;
    uint8_t numbytes;
    for (int i = 0; i < number_of_acks; i++)
    {
        numbytes = recv(connection_sockfd, buf, MAX_PACKET_SIZE, 0);

        if (numbytes == -1)
        {
            break;
        }
        else
        {
            bool is_corrupt;
            parse_packet(&recv_packet, &is_corrupt, nullptr, buf, numbytes);
            acks_received++;
        }
    }

    printf("received packet %d, b = %d\n", recv_packet.seq_no, b);
    uint16_t expected = end - start;
    uint16_t received = recv_packet.seq_no - start + 1;

    uint16_t counter = start;
    for (int i = 0; i < received; i++)
    {
        logger.logACK(counter, (counter == packets_sent - 1 ? last_packet_size : MSS) + HEADER_SIZE);
        counter++;
    }
    //timeout
    if (acks_received == 0)
    {
        printf("TIMEOUT\n");
        logger.logTimeout(curr_seq_no);

        ssthresh = CWND / 2;
        CWND = 1;
        state = 0;
    }
    //3 duplicate ack
    else if (expected - received >= 3)
    {
        if (state == 0 || state == 1)
        {
            state = 2;
            ssthresh = CWND / 2;
            CWND = ssthresh + 3;
        }
        printf("DUP ACKKKKKK\n");
    }
    else
    {
        if (state == 0)
        {
            CWND += acks_received;
        }
        else if (state == 1)
            CWND++;
        else if (state == 2)
        {
            if (acks_received - received > 0)
            {
                CWND += acks_received;
            }
            else
            {
                CWND = ssthresh;
                state = 1;
            }
        } //duplicate acks
    }
    if (state == 0 && CWND >= ssthresh)
        state = 1;

    printf("recv_packet.seq_no: %d, start: %d, end:%d\n", recv_packet.seq_no, start, end);
    printf("expected %d, received %d\n", expected, received);
    //update cwnd

    window = CWND < RWND ? CWND : RWND;
    printf("new window: %d\n", window);
    //update congestion window, start, end
    start = recv_packet.seq_no + 1;
    curr_seq_no = start;
    end = start + window;
    end = packets_sent < end ? packets_sent : end;
    b += received;
    logger.advanceRound();
}

ConnectionInstance ::~ConnectionInstance()
{
    if (connection_sockfd == -1)
        close(connection_sockfd);
}
