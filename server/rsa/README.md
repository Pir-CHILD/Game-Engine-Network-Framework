# 加密方案
## 阶段1 
使用非对称加密 **rsa** 加密C/S的首次TCP连接，商讨**阶段2密钥**细节以及**kcp协议**细节：
- userKey
- conv
- window size(snd, rcv)
- nodelay, interval, resend, nc
```shell
OpenSSL> genrsa -out privateKey.pem
Generating RSA private key, 2048 bit long modulus (2 primes)
........................................................+++++
............................................................+++++
e is 65537 (0x010001)
OpenSSL> rsa -in privateKey.pem -pubout -out publicKey.pem
writing RSA key
```
得到私钥`privateKey.pem`和公钥`publicKey.pem`
## 阶段2
使用对称加密 **AES** 加密随后的kcp通信内容，所使用的**key**来自阶段1。