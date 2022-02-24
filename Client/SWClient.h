#ifndef CLIENT_SWCLIENT_H
#define CLIENT_SWCLIENT_H

#include <cstdint>

#define RWND 8
class SWClient
{
private:
    int sockfd;
    int32_t seq_no_to_recv;
    int32_t seq_no_received;

    void next_recv_state();

public:
    /**
     * Open connection with server on
     * @param ip
     * @param port
     * And send @param filename
     * */
    SWClient(const char *ip, const char *port, const char *filename);
    ~SWClient();
    void client_receive(void *data, int *len, bool *is_last);
};

#endif //CLIENT_SWCLIENT_H
