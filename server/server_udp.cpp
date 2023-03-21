#include "server_udp.h"

struct sockaddr_in client_addr;

int create_udp_sock()
{
    int fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    assert(fd >= 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    assert(ret != -1);

    return fd;
}

int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do
    {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
    int fd = *(int *)user;

    // printf("Server send:%s\n", buf);
    sendto(fd, buf, len, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

    return 0;
}

void handle_udp_msg(int fd)
{
    char buf[BUFF_LEN]; // 接收缓冲区，1024字节
    socklen_t len;
    int count;
    struct sockaddr_in clent_addr; // clent_addr用于记录发送方的地址信息
    while (1)
    {
        memset(buf, 0, BUFF_LEN);
        len = sizeof(clent_addr);
        count = recvfrom(fd, buf, BUFF_LEN, 0, (struct sockaddr *)&clent_addr, &len); // recvfrom是拥塞函数，没有数据就一直拥塞
        if (count == -1)
        {
            printf("recieve data fail!\n");
            return;
        }
        printf("client:%s\n", buf); // 打印client发过来的信息
        memset(buf, 0, BUFF_LEN);
        sprintf(buf, "I have recieved %d bytes data!\n", count);           // 回复client
        printf("server:%s\n", buf);                                        // 打印自己发送的信息给
        sendto(fd, buf, BUFF_LEN, 0, (struct sockaddr *)&clent_addr, len); // 发送信息给client，注意使用了clent_addr结构体指针
    }
}

/*
    server:
            socket-->bind-->recvfrom-->sendto-->close
*/

int main(int argc, char *argv[])
{
    CLI::App app;

    int snd_window{32}, rcv_window{32};
    app.add_option("--sndwnd", snd_window, "sndwnd size");
    app.add_option("--rcvwnd", rcv_window, "rcvwnd size");

    int nodelay{0}, interval{100}, resend{0}, nc{0};
    app.add_option("--nodelay", nodelay, "0: disable(default), 1: enable");
    app.add_option("--interval", interval, "interval update timer interval in millisec, 100ms(default)");
    app.add_option("--resend", resend, "0: disable fast resend(default), 1: enable");
    app.add_option("--nc", nc, "0: normal congestion control(default), 1: disable congestion control");

    CLI11_PARSE(app, argc, argv);

    char buf[BUFF_LEN];
    int hr = 0;
    int server_fd = create_udp_sock();
    ikcpcb *kcp = ikcp_create(0x00112233, (void *)&server_fd);

    ikcp_wndsize(kcp, snd_window, rcv_window);
    ikcp_nodelay(kcp, nodelay, interval, resend, nc);

    // setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    ikcp_setoutput(kcp, udp_output);

    while (1)
    {
        isleep(1);
        ikcp_update(kcp, iclock());

        while (1)
        {
            socklen_t len = sizeof(client_addr);
            hr = recvfrom(server_fd, buf, BUFF_LEN, 0, (struct sockaddr *)&client_addr, &len);
            if (hr < 0)
                break;
            // if (hr > 0)
            ikcp_input(kcp, buf, hr);
        }

        while (1)
        {
            hr = ikcp_recv(kcp, buf, 10);
            if (hr < 0)
                break;
            // if (hr > 0)
            ikcp_send(kcp, buf, hr); // 回射
        }
    };

    // handle_udp_msg(sockfd); // 处理接收到的数据

    ikcp_release(kcp);
    close(server_fd);

    return 0;
}