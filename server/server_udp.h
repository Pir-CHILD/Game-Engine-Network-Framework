#ifndef _SERVER_UDP
#define _SERVER_UDP

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <arpa/inet.h>
#include <map>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include "../3rdparty/kcp-1.7/ikcp.h"
#include "../3rdparty/CLI11/CLI11.hpp"

#define CONV_PORT 12305
#define KCP_PORT 12306
#define BUFF_LEN 2000

static const IINT8 base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890+/";

typedef struct HandshakeInfo
{
    char userKey[50];
    int size;
    IUINT32 conv;
    IINT32 snd_window = 32, rcv_window = 32;
    IINT32 nodelay = 0, interval = 100, resend = 0, nc = 0;
} handshake_info;

/* get system time */
static inline void itimeofday(long *sec, long *usec)
{
#if defined(__unix)
    struct timeval time;
    gettimeofday(&time, NULL);
    if (sec)
        *sec = time.tv_sec;
    if (usec)
        *usec = time.tv_usec;
#else
    static long mode = 0, addsec = 0;
    BOOL retval;
    static IINT64 freq = 1;
    IINT64 qpc;
    if (mode == 0)
    {
        retval = QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
        freq = (freq == 0) ? 1 : freq;
        retval = QueryPerformanceCounter((LARGE_INTEGER *)&qpc);
        addsec = (long)time(NULL);
        addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
        mode = 1;
    }
    retval = QueryPerformanceCounter((LARGE_INTEGER *)&qpc);
    retval = retval * 2;
    if (sec)
        *sec = (long)(qpc / freq) + addsec;
    if (usec)
        *usec = (long)((qpc % freq) * 1000000 / freq);
#endif
}

/* get clock in millisecond 64 */
static inline IINT64 iclock64(void)
{
    long s, u;
    IINT64 value;
    itimeofday(&s, &u);
    value = ((IINT64)s) * 1000 + (u / 1000);
    return value;
}

static inline IUINT32 iclock()
{
    return (IUINT32)(iclock64() & 0xfffffffful);
}

/* sleep in millisecond */
static inline void isleep(unsigned long millisecond)
{
#ifdef __unix /* usleep( time * 1000 ); */
    struct timespec ts;
    ts.tv_sec = (time_t)(millisecond / 1000);
    ts.tv_nsec = (long)((millisecond % 1000) * 1000000);
    /*nanosleep(&ts, NULL);*/
    usleep((millisecond << 10) - (millisecond << 4) - (millisecond << 3));
#elif defined(_WIN32)
    Sleep(millisecond);
#endif
}

int create_udp_sock();
int create_tcp_sock();

IUINT32 get_rand_conv();
IUINT32 get_conv();
void get_aes_key(char userKey[], int size);

int udp_output(const char *buf, int len, ikcpcb *kcp, void *user);

#endif