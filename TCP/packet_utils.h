#ifndef TCP_PACKET_UTILS_H
#define TCP_PACKET_UTILS_H

#include <iostream>
#include <cstdint>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include "netinet/in.h"

#define MAX_PACKET_SIZE 508
#define HEADER_SIZE 8

struct packet{
    uint16_t checksum;          /* Checksum */
    uint16_t len;               /* Length */
    uint32_t seq_no;            /* Sequence number, in ACK this is the ACK'ed packet */
    uint8_t  data[500];         /* Pointer to data if any */
}__attribute__((packed));

/**
 * Fills in packet in a serializable format and calculates checksum.
 * @param len is the length of the data WITHOUT the header
 * @param data could be null if ACK packet
 * */
void pack_packet(
        struct packet *packet,
        uint8_t *data,
        uint16_t len,
        bool is_last,
        uint32_t seq_no);

/**
 * Parses the payload received form the other side.
 * @param packet The packet to fill (output)
 * @param is_corrupt whether this packet is corrupted
 * @param is_last whether this packet is the last one in the stream
 * @param buf the buffer to parse
 * @param len the TOTAL length of the received packet */
void parse_packet(struct packet *packet,
        bool *is_corrupt,
        bool *is_last,
        void *buf,
        int len);

/**
 * Prints the packet, which is in serialisable format,
 * in a useful format
 * */
void print_packet(struct packet *packet);
#endif //TCP_PACKET_UTILS_H
