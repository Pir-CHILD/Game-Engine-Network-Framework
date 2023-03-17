#include "client_udp.h"

int create_udp_sock()
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(fd >= 0);

    return fd;
}

int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
    int fd = *(int *)user;
    size_t t_len = 0;
    char t_buf[BUFF_LEN];
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    printf("Client send: %s\n", buf);
    sendto(fd, buf, len, t_len, (struct sockaddr *)&server_addr, sizeof(server_addr));

    return 0;
}

void udp_msg_sender(int fd, struct sockaddr *dst)
{

    socklen_t len;
    struct sockaddr_in src;
    while (1)
    {
        char buf[BUFF_LEN] = "TEST UDP MSG!\n";
        len = sizeof(*dst);
        printf("client:%s\n", buf); // 打印自己发送的信息
        sendto(fd, buf, BUFF_LEN, 0, dst, len);
        memset(buf, 0, BUFF_LEN);
        recvfrom(fd, buf, BUFF_LEN, 0, (struct sockaddr *)&src, &len); // 接收来自server的信息
        printf("server:%s\n", buf);
        sleep(1); // 一秒发送一次消息
    }
}

/*
    client:
            socket-->sendto-->revcfrom-->close
*/

int main(int argc, char *argv[])
{
    char buf[BUFF_LEN];
    int hr = 0;
    int client_fd = create_udp_sock();
    ikcpcb *kcp = ikcp_create(0x00112233, (void *)&client_fd);

    ikcp_setoutput(kcp, udp_output);

    IUINT32 current = iclock();
    IUINT32 slap = current + 20;
    IUINT32 index = 0;
    IUINT32 next = 0;
    IINT64 sumrtt = 0;
    int count = 0;
    int maxrtt = 0;

    ikcp_wndsize(kcp, 128, 128);
    ikcp_nodelay(kcp, 0, 10, 0, 1);

    while (1)
    {
        isleep(1);
        current = iclock();
        ikcp_update(kcp, iclock());

        for (; current >= slap; slap += 20)
        {
            ((IUINT32 *)buf)[0] = index++;
            ((IUINT32 *)buf)[1] = current;

            ikcp_send(kcp, buf, 8);
        }

        while (1)
        {
            hr = ikcp_recv(kcp, buf, 10);
            if (hr < 0)
                break;
        }
    }

    // udp_msg_sender(client_fd, (struct sockaddr *)&ser_addr);

    close(client_fd);

    return 0;
}