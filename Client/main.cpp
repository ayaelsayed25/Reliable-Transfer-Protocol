#include <iostream>
#include <cstring>
#include "SWClient.h"

int main()
{
    FILE *input = fopen("input.in", "r");
    if (input == nullptr)
    {
        perror("fopen");
        exit(1);
    }

    /* Parse Input file */
    char *ip, *port, *filename;
    size_t ip_len = 0, port_len = 0, filename_len = 0;
    getline(&ip, &ip_len, input);
    strtok(ip, "\r\n");
    getline(&port, &port_len, input);
    strtok(port, "\r\n");
    getline(&filename, &filename_len, input);
    strtok(filename, "\r\n");

    /*printf("port: %s\n", port);
    printf("ip: %s\n", ip);
    printf("filename: %s\n", filename);*/
    SWClient client(ip, port, filename);
    bool is_last = false;
    int last_received = 0;
    FILE *out = fopen(filename, "w");
    do
    {
        uint8_t buf[500];
        int len;
        client.client_receive(buf, &len, &is_last);
        fwrite(buf, sizeof(uint8_t), len, out);
    } while (!is_last);

    fclose(out);
    free(port);
    free(ip);
    free(filename);
    return 0;
}
