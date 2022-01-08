#include "SWClient.h"
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "../TCP/packet_utils.h"

SWClient::SWClient(const char *ip, const char *port, const char *filename) : sockfd(-1),
                                                                             seq_no_to_recv(0),
                                                                             seq_no_received(RWND - 1)
{
    struct addrinfo hints, *servinfo, *p;
    int rv, numbytes;
    {
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0)
        {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            exit(1);
        }

        for (p = servinfo; p != nullptr; p = p->ai_next)
        {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            {
                perror("socket");
                continue;
            }

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
            {
                close(sockfd);
                perror("connect");
                continue;
            }

            break;
        }

        if (p == nullptr)
        {
            fprintf(stderr, "could not resolve address");
            exit(1);
        }

        freeaddrinfo(servinfo);
    }

    struct packet send_packet;
    struct packet recv_packet;
    uint16_t data_len = strlen(filename);
    pack_packet(&send_packet, (uint8_t *)filename, data_len, true, 0);
    // assume this packet always arrives correctly
    // print_packet(&send_packet);
    if (send(sockfd, &send_packet, data_len + HEADER_SIZE, 0) == -1)
    {
        perror("send");
    }

    uint8_t buf[MAX_PACKET_SIZE];
    if ((numbytes = recv(sockfd, buf, MAX_PACKET_SIZE, 0)) == -1)
    {
        perror("recv");
    }
    else
    {
        bool is_corrupt;
        parse_packet(&recv_packet, &is_corrupt, nullptr, buf, numbytes);
        if (!is_corrupt && recv_packet.seq_no == 0)
        {
            printf("Filename ACK'ed\n");
        }
        else
            printf("Filename not ACK'ed\n");
    }
}

void SWClient::client_receive(void *data, int *len, bool *is_last)
{
    void *buf[MAX_PACKET_SIZE];
    int numbytes;
    struct packet recv_packet
    {
    }, send_packet{};
    do
    {
        if ((numbytes = recv(sockfd, buf, MAX_PACKET_SIZE, 0)) == -1)
        {
            perror("recv");
            continue;
        }

        // TODO checksum
        bool is_corrupt;
        parse_packet(&recv_packet, &is_corrupt, is_last, buf, numbytes);
        printf("Received %d Request %d\n", recv_packet.seq_no, seq_no_to_recv);

        if (!is_corrupt && recv_packet.seq_no == seq_no_to_recv)
        {
            //write data in file
            memcpy(data, recv_packet.data, recv_packet.len);
            *len = recv_packet.len;
            /* Send ACK for new packet */
            pack_packet(&send_packet, nullptr, 0, false, seq_no_to_recv);
            //print_packet(&send_packet);
            next_recv_state();
            if (send(sockfd, &send_packet, HEADER_SIZE, 0) == -1)
            {
                perror("send");
            }
            else
            {
                break;
            }
        }
        else
        {
            //discard data
            /* Send ACK for past packet */
            printf("Sending duplicate Ack \n");

            pack_packet(&send_packet, nullptr, 0, false, seq_no_received);
            // print_packet(&send_packet);
            if (send(sockfd, &send_packet, HEADER_SIZE, 0) == -1)
            {
                perror("send");
            }
        }

    } while (true);
}

SWClient::~SWClient()
{
    if (sockfd != -1)
        close(sockfd);
}

void SWClient::next_recv_state()
{
    seq_no_received = seq_no_to_recv;
    seq_no_to_recv = seq_no_to_recv + 1;
}
