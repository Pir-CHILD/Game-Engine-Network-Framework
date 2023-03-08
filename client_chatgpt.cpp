#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdio>
#include <ctime>
#include <unistd.h>
#include "ikcp.h"

// Define the player's struct
struct Player {
    int id;
    float x;
    float y;
    float z;
};

// Define the client info struct
struct ClientInfo {
    int id;
    uint32_t ip;
    uint16_t port;
    Player player;
};

// Define the server's IP and port
const char* server_ip = "127.0.0.1";
const uint16_t server_port = 8888;

// Define the KCP instance
ikcpcb* kcp = nullptr;

// Define the KCP send buffer
char kcp_send_buf[1024];

// Define the KCP receive buffer
char kcp_recv_buf[1024];

// Send the player's position to the server
void SendPlayerPosition() {
    // Fill in the player's info
    ClientInfo client_info;
    client_info.id = 123;
    client_info.ip = 0;
    client_info.port = 0;
    client_info.player.id = 1;
    client_info.player.x = 2.0f;
    client_info.player.y = 3.0f;
    client_info.player.z = 4.0f;

    // Copy the client info into the send buffer
    memcpy(kcp_send_buf, &client_info, sizeof(client_info));

    // Send the data using KCP
    ikcp_send(kcp, kcp_send_buf, sizeof(client_info));
}

int main() {
    // Create a UDP socket for the client
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Bind the socket to a random port
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port = htons(0);
    bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr));

    // Set up the server's address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    // Initialize the KCP instance
    kcp = ikcp_create(0x11223344, nullptr);
    ikcp_nodelay(kcp, 1, 10, 2, 1);
    ikcp_setoutput(kcp, [](const char* buf, int len, ikcpcb* kcp, void* user) -> int {
        // Send the data using UDP
        int sockfd = *((int*)user);
        sendto(sockfd, buf, len, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        return 0;
    });

    // Start the KCP update loop
    uint32_t current_time = GetTickCount();
    uint32_t next_kcp_update_time = current_time;
    while (true) {
        // Get the current time
        current_time = GetTickCount();

        // Update the KCP instance
        ikcp_update(kcp, current_time);

        // Receive data from the server
        int n = recvfrom(sockfd, kcp_recv_buf, sizeof(kcp_recv_buf), 0, nullptr, nullptr);
        if (n > 0) {
            // Feed the received data to KCP
            ikcp_input(kcp, kcp_recv_buf, n);
}
// Send the player's position to the server every 100ms
    if (current_time >= next_kcp_update_time) {
        SendPlayerPosition();
        next_kcp_update_time += 100;
    }

    // Sleep for a short time to avoid using too much CPU
    usleep(1000);
}

// Clean up the KCP instance
ikcp_release(kcp);

// Close the UDP socket
close(sockfd);

return 0;
}

// TODO
// running multiple instances of the client program and connecting each one
// to the same server to simulate communication between multiple players and 
// the game server.