# Game-Engine-Network-Framework
Use reliable UDP(KCP, etc.) as protocol to simulate the game engine network framework.

## server
```shell
cd ./server/build
cmake ..
make
./server
```

## client
```shell
cd ./client/build
cmake ..
make
./client
```
You can see more info by type `./client -h`.
```
Usage: ./client [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  --sndwnd INT                sndwnd size
  --rcvwnd INT                rcvwnd size
  --nodelay INT               0: disable(default), 1: enable
  --interval INT              interval update timer interval in millisec, 100ms(default)
  --resend INT                0: disable fast resend(default), 1: enable
  --nc INT                    0: normal congestion control(default), 1: disable congestion control
  --sIP TEXT                  server IP
```
The result is as follows:
```shell
> ./client
...
[RECV] sn=1002 rtt=106
[RECV] sn=1003 rtt=86
[RECV] sn=1004 rtt=66
---
sndwnd=32 rcvwnd=32
nodelay:            disable
interval(ms):       100
fast resend:        disable
congestion control: enable
---
avgrtt=130 maxrtt=487
```