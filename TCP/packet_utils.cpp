#include <unistd.h>
#include "packet_utils.h"

void pack_packet(
    struct packet *packet,
    uint8_t *data,
    uint16_t len,
    bool is_last,
    uint32_t seq_no)
{

    /* The idea is to pack the is_last bit in the
     * length field of the packet, since 7 bits of it are not used anyway */

    if (len > 500)
    {
        fprintf(stderr, "Length of data to end exceeds 500 %d\n", len);
        exit(1);
    }

    uint16_t packed_length = len + 8;
    if (is_last)
        packed_length ^= 0x8000;

    packet->checksum = 0;
    packet->len = htons(packed_length);
    packet->seq_no = htonl(seq_no);
    if (data != nullptr)
        memcpy(packet->data, data, len);

    /* Calculate checksum
    // TODO wrap around
    uint16_t checksum = 0;
    uint16_t *pack_arr = (uint16_t *)&packet;
    for (int i = 0; i < len + 8; i++)
        checksum += htons(pack_arr[i]);
    checksum = ~checksum;
    packet->checksum = htons(checksum);*/
}

void parse_packet(struct packet *packet,
                  bool *is_corrupt,
                  bool *is_last,
                  void *buf,
                  int len)
{
    if (len > 508 || len < 8)
    {
        fprintf(stderr, "Packet to parse has size %d\n", len);
        exit(-1);
    }

    /* Checksum TODO wrap around and ntoh
    uint16_t checksum = 0;
    uint16_t *arr = (uint16_t *) buf;
    for(int i = 0; i < len; i++)
        checksum += ntohs(arr[i]);
    if (~checksum){
        *is_corrupt = true;
        return;
    }*/
    *is_corrupt = false;

    struct packet *received = (struct packet *)buf;

    /* Handle length */
    uint16_t packed_length = ntohs(received->len);
    if (packed_length & 0x8000)
    {
        if (is_last != nullptr)
            *is_last = true;
        packed_length &= ~0x8000;
    }
    else
    {
        if (is_last != nullptr)
            *is_last = false;
    }
    packed_length -= 8;

    packet->checksum = ntohs(received->checksum);
    packet->len = packed_length;
    packet->seq_no = ntohl(received->seq_no);
    memcpy(packet->data, received->data, packed_length);
}

void print_packet(struct packet *packet)
{
    int length = (ntohs(packet->len) & ~0x8000) - 8;
    printf("checksum: %d\n", ntohs(packet->checksum));
    printf("len: %d\n", length);
    printf("seq_no: %d\n", ntohl(packet->seq_no));
    printf("data: %.*s\n", length, packet->data);
}