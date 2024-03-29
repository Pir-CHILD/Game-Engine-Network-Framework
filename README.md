# Game-Engine-Network-Framework
Use reliable UDP(KCP, etc.) as protocol to simulate the game engine network framework.

## server
```shell
> mkdir -p ./server/build; cd ./server/build
> cmake ..
> make
> ./server
```
You can see more info by type `./server -h`.
```shell
> ./server --help
Usage: ./server [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  --sndwnd INT                sndwnd size
  --rcvwnd INT                rcvwnd size
  --nodelay INT               0: disable(default), 1: enable
  --interval INT              interval update timer interval in millisec, 100ms(default)
  --resend INT                0: disable fast resend(default), 1: enable, 2: 2 ACK spans will be retransmitted directly
  --nc INT                    0: normal congestion control(default), 1: disable congestion control
```
## client
```shell
> mkdir -p ./client/build; cd ./client/build
> cmake ..
> make
> ./client
```
You can see more info by type `./client -h`.
```shell
> ./client --help
Usage: ./client [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  --sndwnd INT                sndwnd size
  --rcvwnd INT                rcvwnd size
  --nodelay INT               0: disable(default), 1: enable
  --interval INT              interval update timer interval in millisec, 100ms(default)
  --resend INT                0: disable fast resend(default), 1: enable, 2: 2 ACK spans will be retransmitted directly
  --nc INT                    0: normal congestion control(default), 1: disable congestion control
  --sIP TEXT                  server IP
  --size INT                  kcp test packet sn size, 1000(default)
```
The result is as follows:
```shell
> ./client
...
[RECV] sn=9999 rtt=22
[RECV] sn=10000 rtt=40
[RECV] sn=10001 rtt=21
---
server IP:          43.140.208.229
sndwnd:             256
rcvwnd:             256
nodelay:            disable
interval(ms):       40
fast resend:        disable
congestion control: enable
---
avgrtt=33 maxrtt=77
```