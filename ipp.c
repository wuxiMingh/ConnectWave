#include <stdio.h>
#include "ipp.h"
#include <string.h>
unsigned int szIPToint(char *pIP) // 将字符串ip转为网络字节整数IP
{
    int ip;
    inet_pton(AF_INET, pIP, &ip);
    return ip;
}
char *intIPTostr(unsigned int ip) // 把网络字节整数转为字符串IP
{
    char *s = malloc(16);
    inet_ntop(AF_INET, &ip, s, 16);
    return s;
}
struct sockaddr_in getaddr(char *pzsIp, u_int16_t port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = szIPToint(pzsIp); // 历史原因把字符串ip转为网络字节整数
    addr.sin_port = htons(port);             // 转为大端
    return addr;
}