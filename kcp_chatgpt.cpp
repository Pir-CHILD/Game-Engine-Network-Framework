#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include "ikcp.h"

int main()
{
    // create UDP socket
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    // bind UDP socket to a local port
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(12345);
    bind(udp_sock, (struct sockaddr *)&local_addr, sizeof(local_addr));

    // create KCP object
    ikcpcb *kcp = ikcp_create(0x11223344, (void *)udp_sock);

    // set output function
    kcp->output = [](const char *buf, int len, ikcpcb *kcp, void *user)->int {
        // send packet over the network using UDP
        int udp_sock = (int)user;
        struct sockaddr_in remote_addr;
        memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        remote_addr.sin_port = htons(12346);
        sendto(udp_sock, buf, len, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
        return 0;
    };

    // update KCP every 10ms
    auto current = std::chrono::high_resolution_clock::now();
    auto interval = std::chrono::milliseconds(10);
    while (true) {
        auto elapsed = std::chrono::high_resolution_clock::now() - current;
        if (elapsed >= interval) {
            ikcp_update(kcp, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
            current = std::chrono::high_resolution_clock::now();
        }

        // receive packet from network using UDP
        char buf[1024];
        struct sockaddr_in remote_addr;
        socklen_t addrlen = sizeof(remote_addr);
        int len = recvfrom(udp_sock, buf, sizeof(buf), 0, (struct sockaddr *)&remote_addr, &addrlen);
        if (len > 0) {
            // feed packet into KCP object
            ikcp_input(kcp, buf, len);
        }
    }

    // release KCP object
    ikcp_release(kcp);

    // close UDP socket
    close(udp_sock);

    return 0;
}
