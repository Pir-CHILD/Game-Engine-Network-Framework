#include "client_udp.h"

struct sockaddr_in server_addr;

int create_udp_sock()
{
    int fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    assert(fd >= 0);

    return fd;
}

int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
    int fd = *(int *)user;

    // printf("Client send: %s\n", buf);
    sendto(fd, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

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

    char buf[BUFF_LEN];
    int hr = 0;
    int client_fd = create_udp_sock();
    ikcpcb *kcp = ikcp_create(0x00112233, (void *)&client_fd);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addr.sin_port = htons(SERVER_PORT);

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

    IUINT32 ts1 = iclock();

    while (1)
    {
        // // Sleep for a short time to avoid using too much CPU
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

            printf("[RECV] mode=%d sn=%d rtt=%d\n", 1, (int)sn, (int)rtt);
        }
        if (next > 1000)
            break;
    }

    ts1 = iclock() - ts1;

    ikcp_release(kcp);

    const char *names[3] = {"default", "normal", "fast"};
    printf("%s mode result (%dms):\n", names[1], (int)ts1);
    printf("avgrtt=%d maxrtt=%d tx=%d\n", (int)(sumrtt / count), (int)maxrtt, (int)0);

    // udp_msg_sender(client_fd, (struct sockaddr *)&ser_addr);

    close(client_fd);

    return 0;
}