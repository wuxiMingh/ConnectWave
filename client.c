#include <stdio.h>
#include "ipp.h"
#include <string.h>
#include <sys/epoll.h>
#include <sqlite3.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int fd;
user buf;
friend_add ff[MAX_EVENTS];
char receiver_name[128];
int cout;
int file_chengong;
/*线程work*/
void *work(void *a)
{
}

/*登录注册函数*/
void _regist() // 注册功能
{
    user buf;
    printf("请输入账号和密码\n");
    scanf("%s %s", buf.name, buf.password);
    msg m;
    m.type = 1;
    memcpy(m.data, &buf, sizeof(buf));
    write(fd, &m, sizeof(m));
    read(fd, &m, sizeof(m));
    if (m.type == 11)
    {
        printf("注册成功\n");
    }
    else
        printf("注册失败,用户已存在\n");
}
int _login()
{
    printf("请输入账号和密码\n");
    scanf("%s %s", buf.name, buf.password);
    msg m;
    m.type = 2;
    memcpy(m.data, &buf, sizeof(buf));
    write(fd, &m, sizeof(m));
    read(fd, &m, sizeof(m));
    if (m.type == 22)
    {
        int a;
        printf("登录成功\n");
        return 1;
    }
    else
    {
        printf("登录失败\n");
        return 0;
    }
}

/*群聊模块*/
// 查看群聊
void _groupchat_look()
{
    msg m;
    m.type = 7;
    int a;
    char fs[MAX_EVENTS][MAX_EVENTS];
    write(fd, &m, sizeof(m));
    printf("%5s 群名\n", "");
    while (m.type == 7)
    {
        read(fd, &m, sizeof(m));
        if (m.type != 7 || m.data == 0)
            break;
        printf("%10s\n", m.data);
    }
}
// 群聊线程
void *_groupchat_work(void *data)
{
    while (1)
    {
        msg m;
        read(fd, &m, sizeof(m));
        friend_msg f = *(friend_msg *)&m.data;
        printf("%s:%s\n", f.name, f.data);
    }
}
// 进入群聊聊天
void _groupchat_chat()
{
    msg m;
    m.type = 8;
    printf("输入你进入的群聊\n");
    scanf("%s", m.data);
    char qunmin[128];
    strcpy(qunmin, m.data);
    write(fd, &m, sizeof(m));
    while (1)
    {
        read(fd, &m, sizeof(m));
        if (m.type == 82)
        {
            printf("你没在此群聊中\n");
            break;
        }
        if (m.type == 81)
        {
            printf("进群成功\n");
            break;
        }
        if (m.type == 83)
        {
            printf("查无此群\n");
            break;
        }
    }
    if (m.type == 81)
    {
        pthread_t t;
        pthread_create(&t, 0, _groupchat_work, NULL);
        while (1)
        {
            friend_msg f;
            fflush(stdout);
            strcpy(f.name, qunmin);
            scanf("%s", f.data);
            if (strcmp(f.data, "quit") == 0)
            {
                pthread_cancel(t);
                return;
            }
            memcpy(m.data, &f, sizeof(f));
            write(fd, &m, sizeof(m));
        }
    }
}
// 退出某群聊
void _groupchat_quit()
{
    msg m;
    m.type = 9;
    printf("请输入你要退出的群名\n");
    scanf("%s", m.data);
    write(fd, &m, sizeof(m));
    while (1)
    {
        read(fd, &m, sizeof(m));
        if (m.type == 92)
        {
            printf("你没在此群聊中\n");
            break;
        }
        if (m.type == 91)
        {
            printf("退出成功\n");
            break;
        }
        if (m.type == 93)
        {
            printf("查无此群\n");
            break;
        }
    }
}
// 加入群聊
void _groupchat_join()
{
    msg m;
    m.type = 10;
    printf("请输入你要加入的群名\n");
    scanf("%s", m.data);
    write(fd, &m, sizeof(m));
    while (1)
    {
        read(fd, &m, sizeof(m));
        if (m.type == 102)
        {
            printf("你已在群聊中\n");
            break;
        }
        if (m.type == 101)
        {
            printf("加入成功\n");
            break;
        }
        if (m.type == 103)
        {
            printf("查无此群\n");
            break;
        }
    }
}
// 创建群聊
// 查看我加入的群
void _groupchat_mygroup()
{
    msg m;
    m.type = 12;
    write(fd, &m, sizeof(m));
    while (1)
    {
        read(fd, &m, sizeof(m));
        if (m.type == 121)
        {
            break;
        }
        printf("%s\n", m.data);
    }
}
void _groupchat_create()
{
    msg m;
    m.type = 11;
    printf("请输入你要创建的群名\n");
    scanf("%s", m.data);
    write(fd, &m, sizeof(m));
    while (1)
    {
        read(fd, &m, sizeof(m));
        if (m.type != 11)
        {
            printf("创建成功\n");
            break;
        }
    }
}
// 群聊主函数
void _GroupChat()
{
    int a;
    while (1)
    {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║                         群聊菜单                           ║\n");
        printf("╠════════════════════════════════════════════════════════════╣\n");
        printf("║ 1. 查看群聊                2. 进入群聊                     ║\n");
        printf("║ 3. 退出群聊                4. 加入群聊                     ║\n");
        printf("║ 5. 创建群聊                6. 查看我的群聊                 ║\n");
        printf("║ 7. 返回                                                    ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("请输入:\n");

        if (scanf("%d", &a) != 1)
        {
            // 错误处理：输入不是整数
            printf("无效输入，请输入一个数字。\n");
            // 清除输入缓冲区
            while (getchar() != '\n')
                ;
            continue;
        }

        switch (a)
        {
        case 1:
            _groupchat_look();
            break;
        case 2:
            _groupchat_chat();
            break;
        case 3:
            _groupchat_quit();
            break;
        case 4:
            _groupchat_join();
            break;
        case 5:
            _groupchat_create();
            break;
        case 6:
            _groupchat_mygroup();
            break;
        case 7:
            return;
        default:
            printf("无效选项，请重新输入。\n");
            break;
        }
    }
}

/*私聊模块*/
void *_signalchat_work(void *data) // 线程选择好友私聊函数
{
    while (1)
    {
        msg m;
        friend_msg *fmsg = (friend_msg *)&m.data;
        read(fd, &m, sizeof(m));
        printf("%s:%s\n", fmsg->name, fmsg->data);
    }
}
// 添加好友线程，用来读取好友添加信息
void *_signalfriendadd_work(void *data)
{
    while (1)
    {
        msg m;
        read(fd, &m, sizeof(m));
        friend_add f = *(friend_add *)&m.data;
        if (m.type == 51)
        {
            ff[cout] = f;
            cout++;
        }
        if (m.type == 53)
        {
            printf("%s添加成功\n", f.name2);
            break;
        }
        if (m.type == 54)
        {
            printf("%s添加失败,没同意\n", f.name2);
            break;
        }
    }
}
// 好友聊天函数
void _signalchat_friend(friend *fs, int n)
{
    char a[128];
    msg m;
    m.type = 4;
    printf("请输入好友名\n");
    scanf("%s", a);
    int i;
    for (i = 0; i < n; i++)
    {
        if (strcmp(a, fs[i].name) == 0)
        {
            if (strcmp(fs[i].state, "no") == 0)
            {
                printf("你的好友不在线上\n");
                return;
            }
            else
                break;
        }
    }
    if (i == n)
    {
        printf("你没有此好友\n");
        return;
    }
    pthread_t p;
    pthread_create(&p, 0, _signalchat_work, NULL);
    while (1)
    {
        friend_msg fmsg;
        strcpy(fmsg.name, a);
        fflush(stdout);
        scanf("%s", fmsg.data);
        memcpy(m.data, &fmsg, sizeof(fmsg));
        write(fd, &m, sizeof(m));
        if (strcmp(fmsg.data, "quit") == 0)
        {
            pthread_cancel(p);
            break;
        }
    }
}
// 添加好友函数
void _signalchat_addfriend()
{
    msg m;
    friend_add f;
    pthread_t p;
    pthread_create(&p, 0, _signalfriendadd_work, NULL); // 文件描述符传进去
    while (1)
    {
        printf("1.添加好友\t2.查看好友申请\t3.退出\n");
        int a;
        scanf("%d", &a);
        switch (a)
        {
        case 1:
        {
            printf("请输入名字\n");
            strcpy(f.name1, buf.name);
            m.type = 5;
            scanf("%s", f.name2);
            memcpy(m.data, &f, sizeof(f));
            write(fd, &m, sizeof(m));
        }
        break;
        case 2:
        {
            int i;
            while (1)
            {
                for (i = 0; i < cout; i++)
                {
                    if (strcmp(ff[i].flag, "yes") == 0 || strcmp(ff[i].flag, "no") == 0)
                    {
                        continue;
                    }
                    printf("%s好友申请\n", ff[i].name1);
                    printf("yes or no\n");
                    scanf("%s", ff[i].flag);
                    m.type = 52;
                    memcpy(m.data, &ff[i], sizeof(f));
                    write(fd, &m, sizeof(m));
                }
                if (i == cout)
                {
                    break;
                }
            }
        }
        break;
        case 3:
        {
            pthread_cancel(p);
            return;
        }
        default:
            break;
        }
    }
}
// 好友删除函数
void _signalchat_delfriend()
{
    msg m;
    m.type = 6;
    friend_change f;
    printf("请输入删除好友名\n");
    scanf("%s", f.name2);
    strcpy(f.name1, buf.name);
    memcpy(m.data, &f, sizeof(f));
    write(fd, &m, sizeof(m));
    while (1)
    {
        read(fd, &m, sizeof(m));
        if (m.type == 66)
        {
            printf("删除成功\n");
            break;
        }
        else
        {
            printf("删除失败\n");
            break;
        }
    }
}
// 显示所有好友
void _signalchat_friendshow(friend fs[MAX_EVENTS], int n, short m)
{
    printf("%-5s好友名 %-5s在线状态\n", "", "");
    for (int i = 0; i < n; i++)
    {
        printf("%10s%10s\n", fs[i].name, fs[i].state);
    }
    printf("\n");
    printf("好友在线数量:%d\n", m);
}
// 好友私聊主模块
void _SingalChat()
{
    while (1)
    {
        msg m;
        m.type = 3;
        int a;
        friend fs[MAX_EVENTS];
        write(fd, &m, sizeof(m));
        int n = 0;
        while (m.type == 3)
        {
            if (m.type != 3 || m.data == 0)
                break;
            read(fd, &m, sizeof(m));
            friend *f = (friend *)&m.data;
            fs[n] = *f;
            n++;
        }
        printf("╔════════════════════════════════════╗\n");
        printf("║             signalchat             ║\n");
        printf("╠════════════════════════════════════╣\n");
        printf("║ 1. 聊天                            ║\n");
        printf("║ 2. 在线加好友                      ║\n");
        printf("║ 3. 删除好友                        ║\n");
        printf("║ 4. 显示所有好友                    ║\n");
        printf("║ 5. 返回                            ║\n");
        printf("╚════════════════════════════════════╝\n");
        printf("请输入选项编号:\n");
        scanf("%d", &a);
        switch (a)
        {
        case 1:
            _signalchat_friend(fs, n - 1);
            break;
        case 2:
            _signalchat_addfriend();
            break;
        case 3:
            _signalchat_delfriend();
            break;
        case 4:
            _signalchat_friendshow(fs, n - 1, m.type);
            break;
        case 5:
            return;
            break;
        default:
            break;
        }
    }
}
// 文件发送请求操作
void send_file_request_to_server(char *filename, char *receiver_name)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        perror("File open error");
        return;
    }

    fseek(file, 0, SEEK_END);
    int total_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    file_packet packet;
    packet.total_size = total_size;
    packet.seq = 0;
    strcpy(packet.sender_name, buf.name);
    strcpy(packet.receiver_name, receiver_name);
    strcpy(packet.file_name, filename);

    msg m;
    m.type = 101; // 文件传输请求消息类型
    memcpy(m.data, &packet, sizeof(file_packet));
    write(fd, &m, sizeof(m));
    fclose(file);
}
char sent_name[128];
void handle_file_request()
{
    printf("有人请求发送文件，是否同意接收？\n");
    file_packet packet;
    char response[10];
    scanf("%s", response);
    if (strcmp(response, "yes") == 0)
    {
        msg ack;
        ack.type = 102;
        strcpy(packet.sender_name, sent_name); // 同意文件传输消息类型
        strcpy(packet.receiver_name, receiver_name);
        memcpy(ack.data, &packet, sizeof(file_packet));
        write(fd, &ack, sizeof(ack));
    }
}
void *_file_work(void *data)
{
    while (1)
    {
        msg m;
        read(fd, &m, sizeof(m));
        file_packet packet = *(file_packet *)&m.data;
        if (m.type == 101)
        {
            strcpy(sent_name, packet.sender_name);
            printf("有人发来文件发送请求，请处理\n");
        }
        if (m.type == 104)
        {
            FILE *file = fopen("/home/student/p/ConnectWave/1.png", "a+b");
            if (!file)
            {
                perror("File create error");
                return NULL;
            }
            fwrite(packet.data, 1, 620, file);
            fclose(file);
            file_chengong = 1;
        }
        if (m.type == 103)
        {
            printf("%d\n", m.type);
            FILE *file1 = fopen("./2.png", "rb");
            if (!file1)
            {
                perror("File open error");
                return NULL;
            }
            else
            {
                printf("文件打开成功\n");
            }
            while (feof(file1) != 1)
            {
                char data[620];
                fread(data, 1, 620, file1);
                memcpy(packet.data, data, sizeof(data));
                packet.seq++;
                strcpy(packet.receiver_name, receiver_name);
                memcpy(m.data, &packet, sizeof(file_packet));
                m.type = 104;
                write(fd, &m, sizeof(m));
            }
            fclose(file1);
        }
    }
}
void _File_do()
{
    pthread_t t;
    pthread_create(&t, 0, _file_work, NULL);
    while (1)
    {
printf("┌──────────────────────────────┐\n");
printf("│          文件操作菜单        │\n");
printf("├──────────────────────────────┤\n");
printf("│ 1.发送文件请求 2.处理文件请求│\n");
printf("│ 3.接收文件      4.退出       │\n");
printf("└──────────────────────────────┘\n");
printf("请输入选项 (1-4):               \n");

        int a;
        scanf("%d", &a);
        switch (a)
        {
        case 1:
        {
            char filename[128];
            printf("请输入文件名和接收者名字:\n");
            scanf("%s %s", filename, receiver_name);
            send_file_request_to_server(filename, receiver_name);
        }
        break;
        case 2:
            handle_file_request();
            break;
        case 3:
        {
            if (file_chengong == 1)
                printf("文件正在接收\n");
            else
                printf("没有文件需要接收\n");
        }
        break;
        case 4:
            return;
        default:
            break;
        }
    }
}

/*主函数*/
int main()
{
    struct sockaddr_in addr = getaddr("192.168.150.130", 9000);
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == connect(fd, (struct sockaddr *)&addr, sizeof(addr)))
    {
        printf("是不是没有开启服务器？\n");
        return 0;
    }
    while (1)
    {
        int a;
printf("┌──────────────────────────────┐\n");
printf("│          登录菜单            │\n");
printf("├──────────────────────────────┤\n");
printf("│ 1. 注册    2. 登录    3. 退出│\n");
printf("└──────────────────────────────┘\n");

        scanf("%d", &a);
        switch (a)
        {
        case 1:
            _regist();
            /* code */
            break;
        case 2:
        {
            if (_login())
            {
                while (1)
                {
                    printf("1.群发\t 2.私聊\t3.文件4.退出\n");
                    printf("请输入：");
                    scanf("%d", &a);
                    switch (a)
                    {
                    case 1:
                        _GroupChat();
                        break;
                    case 2:
                        _SingalChat();
                        break;
                    case 3:
                        _File_do();
                        break;
                    case 4:
                        return 0;
                    default:
                        break;
                    }
                }
            }
        };
        break;
        case 3:
            exit(1);
            break;
        default:
            break;
        }
    }
    close(fd);
    return 0;
}