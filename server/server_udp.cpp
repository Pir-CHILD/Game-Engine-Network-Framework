#include "server_udp.h"
// 1. 建立TCP连接
// 2. 协商conv，即握手过程(随机生成，随后递增序列号即可)
// TODO: 3. 加密该TCP连接，同时对接下来kcp所传输连接加密
//       3.1 RSA 非对称加密以密钥协商
//       3.2 AES 对称加密以进行数据通信
// TODO: 4. 引入线程，对每个kcp连接重复上述操作
bool conv_flag;
IUINT32 last_conv;
struct sockaddr_in client_addr;
socklen_t client_len;
std::map<IUINT32, ikcpcb *> conv_map;

IUINT32 get_rand_conv()
{
    last_conv = (IUINT32)rand();

    return last_conv;
}

IUINT32 get_conv()
{
    IUINT32 t = last_conv;
    last_conv = (t + 1) % UINT32_MAX;

    return last_conv;
}

void get_aes_key(char userKey[], int size)
{
    // IUINT8 *userKey = new IUINT8[32];
    for (int i = 0; i < size; i++)
    {
        int t = rand() % 65;
        userKey[i] = base64_chars[t];
    }
}

int create_tcp_sock()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd >= 0);

    int opt = 1, res = 0;
    res = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    assert(res == 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CONV_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    res = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    assert(res != -1);

    return fd;
}

int create_udp_sock()
{
    int fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    assert(fd >= 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(KCP_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    assert(ret != -1);

    return fd;
}

int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
    int fd = *(int *)user;

    // printf("Server send:%s\n", buf);
    sendto(fd, buf, len, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

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
    app.add_option("--resend", resend, "0: disable fast resend(default), 1: enable, 2: 2 ACK spans will be retransmitted directly");
    app.add_option("--nc", nc, "0: normal congestion control(default), 1: disable congestion control");

    CLI11_PARSE(app, argc, argv);

    /* Socket init */
    char buf[BUFF_LEN];
    int hr = 0;

    /* srand */
    srand((unsigned int)time(NULL));

    /* Tcp send message */
    int listen_fd = create_tcp_sock();
    int res = listen(listen_fd, 3);
    assert(res == 0);

    memset(&client_addr, 0, sizeof(client_addr));
    client_len = sizeof(client_addr);
    int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
    assert(client_fd > 0);

    if ((hr = recv(client_fd, buf, BUFF_LEN, 0)) < 0)
    {
        printf("recv error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }
    printf("client IP: %s\n", inet_ntoa(client_addr.sin_addr));
    printf("recv info: %s\n", buf);

    handshake_info *info = new handshake_info;

    // userKey
    char *t_userKey = new char[50];
    get_aes_key(t_userKey, 32);
    t_userKey[32] = '\0';
    const char *t = t_userKey;
    strcpy(info->userKey, t);
    // size
    info->size = 32;

    // conv
    IUINT32 conv_num = get_rand_conv();
    info->conv = conv_num;
    // printf("%d\n", info->conv);
    // const char *conv_num_char = std::to_string(conv_num).c_str();

    // send
    char send_buf[100];
    memcpy(send_buf, info, sizeof(handshake_info));

    if ((hr = send(client_fd, send_buf, sizeof(send_buf), 0)) < 0)
    {
        printf("send error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }
    // handshake_info *test = (handshake_info *)send_buf;
    // printf("send_buf size: %d, handshake_info size: %d, conv: %d\n", sizeof(send_buf), sizeof(handshake_info), test->conv);
    printf("Send handshake_info to client OK.\nconv num: %d\n", info->conv);

    // close connect
    close(client_fd);
    close(listen_fd);

    /* Udp init kcp */
    int kcp_fd = create_udp_sock();
    ikcpcb *kcp = ikcp_create(conv_num, (void *)&kcp_fd);
    ikcp_setoutput(kcp, udp_output);

    ikcp_wndsize(kcp, snd_window, rcv_window);
    ikcp_nodelay(kcp, nodelay, interval, resend, nc);

    /* Kcp test */
    bool test_flag = false;
    IINT16 empty_count = 0;
    while (1)
    {
        isleep(1);
        ikcp_update(kcp, iclock());

        while (1)
        {
            socklen_t len = sizeof(client_addr);
            hr = recvfrom(kcp_fd, buf, BUFF_LEN, 0, (struct sockaddr *)&client_addr, &len);
            if (hr < 0)
            {
                empty_count++;
                break;
            }
            empty_count = 0;
            ikcp_input(kcp, buf, hr);
        }

        while (1)
        {
            hr = ikcp_recv(kcp, buf, 10);
            if (hr < 0)
            {
                // printf("2 break\n");
                break;
            }
            ikcp_send(kcp, buf, hr); // 回射
        }

        if (empty_count >= SHRT_MAX)
            break;
    };

    ikcp_release(kcp);
    close(kcp_fd);

    return 0;
}