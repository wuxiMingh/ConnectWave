#ifndef _IPP_H
#define _IPP_H

#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_EVENTS 15
#define MAX_UDP_PACKET_SIZE 1024

// 用来传输数据的船
typedef struct Massage {
    short type;
    char data[1022];
} msg;

// 用来注册或登录的用户名及密码
typedef struct USER {
    char name[128];
    char password[128];
} user;

// 用来传输好友集的信息
typedef struct FRIEND {
    char state[20];
    char name[128];
} friend;

typedef struct FRIEND_MSG {
    char data[888];
    char name[128];
} friend_msg;

typedef struct FRIEND_ADD {
    char name1[128];
    char name2[128];
    char flag[20];
} friend_change, friend_add;

// 文件传输相关
typedef struct FilePacket {
    int seq;          // 序列号
    int total_size;   // 文件总大小
    int chunk_size;   // 数据块大小
    char sender_name[128]; // 发送者名字
    char receiver_name[128]; // 接收者名字
    char file_name[128]; // 文件名
    char data[MAX_UDP_PACKET_SIZE - sizeof(int) * 5 - 128 * 3]; // 数据
} file_packet;

/* 常用网络编程函数 */
unsigned int szIPToint(char *pIP);                       // 将字符串ip转为网络字节整数IP
char *intIPTostr(unsigned int ip);                       // 把网络字节整数转为字符串IP
struct sockaddr_in getaddr(char *pzsIp, u_int16_t port); // 定义一个bind函数（绑定端口）中,最重要参数stucrt sockaddr

#endif
