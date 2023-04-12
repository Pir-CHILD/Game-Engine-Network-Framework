#include "client_udp.h"
// TODO：1. 建立TCP连接
// TODO: 2. 协商conv，即握手过程
// TODO: 3. 加密该TCP连接，同时对接下来kcp所传输连接加密
struct sockaddr_in server_addr;

int create_udp_sock()
{
    int fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    assert(fd >= 0);

    return fd;
}

int create_tcp_sock()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd >= 0);

    return fd;
}

int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
    int fd = *(int *)user;

    sendto(fd, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    return 0;
}

int main(int argc, char *argv[])
{
    /* CLI init */
    CLI::App app;

    int snd_window{32}, rcv_window{32};
    app.add_option("--sndwnd", snd_window, "sndwnd size");
    app.add_option("--rcvwnd", rcv_window, "rcvwnd size");

    int nodelay{0}, interval{100}, resend{0}, nc{0};
    app.add_option("--nodelay", nodelay, "0: disable(default), 1: enable");
    app.add_option("--interval", interval, "interval update timer interval in millisec, 100ms(default)");
    app.add_option("--resend", resend, "0: disable fast resend(default), 1: enable");
    app.add_option("--nc", nc, "0: normal congestion control(default), 1: disable congestion control");

    std::string server_ip = "127.0.0.1";
    app.add_option("--sIP", server_ip, "server IP");

    CLI11_PARSE(app, argc, argv);

    /* Socket init */
    char buf[BUFF_LEN];
    int hr = 0;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addr.sin_port = htons(CONV_PORT);

    /* Tcp get conv */
    int tcp_fd = create_tcp_sock();
    int res = connect(tcp_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    assert(res == 0);

    strcpy(buf, "Request for handshake info");
    if ((hr = send(tcp_fd, buf, sizeof(buf), 0)) < 0)
    {
        printf("send error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }

    char recv_buf[100];
    if ((hr = recv(tcp_fd, recv_buf, sizeof(recv_buf), MSG_WAITALL)) < 0)
    {
        printf("recv error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }
    handshake_info *info = (handshake_info *)recv_buf;
    printf("Get handshake_info from server OK.\n");

    IUINT32 conv_num = 0;
    // sscanf(buf, "%u", &conv_num);
    conv_num = info->conv;
    printf("Recv conv num: %d\n", conv_num);
    close(tcp_fd);

    /* Udp init kcp */
    server_addr.sin_port = htons(KCP_PORT);
    int client_fd = create_udp_sock();
    ikcpcb *kcp = ikcp_create(conv_num, (void *)&client_fd);
    ikcp_setoutput(kcp, udp_output);

    IUINT32 current = iclock();
    IUINT32 slap = current + 20;
    IUINT32 index = 0;
    IUINT32 next = 0;
    IINT64 sumrtt = 0;
    int count = 0;
    int maxrtt = 0;

    ikcp_wndsize(kcp, snd_window, rcv_window);
    ikcp_nodelay(kcp, nodelay, interval, resend, nc);

    /* Kcp test */
    while (1)
    {
        // Sleep for a short time to avoid using too much CPU
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
            socklen_t len = sizeof(server_addr);
            hr = recvfrom(client_fd, buf, BUFF_LEN, 0, (struct sockaddr *)&server_addr, &len);
            if (hr < 0)
                break;
            ikcp_input(kcp, buf, hr);
        }

        while (1)
        {
            hr = ikcp_recv(kcp, buf, 10);

            if (hr < 0)
                break;
            IUINT32 sn = *(IUINT32 *)(buf + 0);
            IUINT32 ts = *(IUINT32 *)(buf + 4);
            IUINT32 rtt = current - ts;

            if (sn != next)
            {
                printf("ERROR sn %d<->%d\n", (int)count, (int)next);
                return -1;
            }

            next++;
            sumrtt += rtt;
            count++;
            if (rtt > (IUINT32)maxrtt)
                maxrtt = rtt;

            printf("[RECV] sn=%d rtt=%d\n", (int)sn, (int)rtt);
        }
        if (next > 1000)
            break;
    }

    ikcp_release(kcp);

    printf("---\n");
    printf("sndwnd=%d rcvwnd=%d\n", snd_window, rcv_window);
    const char *able[] = {"disable", "enable"};
    printf("nodelay:            %s\n", able[nodelay]);
    printf("interval(ms):       %d\n", interval);
    printf("fast resend:        %s\n", able[resend]);
    printf("congestion control: %s\n", able[1 - nc]);
    printf("---\n");
    printf("avgrtt=%d maxrtt=%d\n", (int)(sumrtt / count), (int)maxrtt);

    close(client_fd);

    return 0;
}