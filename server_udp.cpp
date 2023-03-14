#include "server_udp.h"

int create_udp_sock()
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
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
    size_t t_len = 0;
    char t_buf[BUFF_LEN];
    struct sockaddr_in client_addr;

    ssize_t count = recvfrom(fd, t_buf, BUFF_LEN, 0, (struct sockaddr *)&client_addr, (socklen_t *)t_len);
    assert(count != -1);

    printf("Client:\n%s\n", t_buf);
    count = sendto(fd, buf, len, 0, (struct sockaddr *)&client_addr, t_len);
    assert(count != -1);

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
    int sockfd = create_udp_sock();
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    ikcpcb *kcp = ikcp_create(0x00112233, (void *)&sockfd);
    ikcp_setoutput(kcp, udp_output);

    IUINT32 current = iclock(), slap = current + 20;

    while (1)
    {
        isleep(1);
        current = iclock();
        ikcp_update(kcp, iclock());
    };

    // handle_udp_msg(sockfd); // 处理接收到的数据

    ikcp_release(kcp);
    close(sockfd);
    return 0;
}