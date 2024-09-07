#include <stdio.h>
#include "ipp.h"
#include <string.h>
#include <sys/epoll.h>
#include <sqlite3.h>
#include <semaphore.h>
sqlite3 *db;
typedef struct LOGIN_STATE // 用户登录节点
{
    int fd;
    char login_name[128];
} login_state;
// 用来数据库的查找结构体
typedef struct queryNode
{
    char **ppResult;
    int row;                    // 列
    int col;                    // 行
} QN;                           // 数据库表的读取格式
login_state logins[MAX_EVENTS]; // 用户登录集
int cout = 0;                   // 用户上线数量
void del_table(sqlite3 *db, char *table_name, char *table_columns, char *table_values)
{
    //     DELETE FROM Websites WHERE name='Facebook' AND country='USA';
    char *msg = 0;
    char sql[258];
    sprintf(sql, "DELETE FROM %s WHERE %s='%s'", table_name, table_columns, table_values);
    sqlite3_exec(db, sql, 0, 0, &msg); // 执行sql串的命令
    if (msg != 0)
    {
        // 如果是因为表格已经存在而出错就不需要退出程序，后面可以继续操作表格
        if (strcmp("table user already exists", msg) != 0)
        {
            printf("执行创建表格出错%s\n", msg);
            return;
        }
    }
    else
        printf("%s表删除成功\n", table_name);
}
void insert_table(sqlite3 *db, char *table_name, char *table_columns, char *table_values) // values中要有'隔开
{
    char *msg = 0;
    char sql[258];
    sprintf(sql, "INSERT INTO %s (%s) VALUES (%s)", table_name, table_columns, table_values);
    sqlite3_exec(db, sql, 0, 0, &msg); // 执行sql串的命令
    if (msg != 0)
    {
        // 如果是因为表格已经存在而出错就不需要退出程序，后面可以继续操作表格
        if (strcmp("table user already exists", msg) != 0)
        {
            printf("执行创建表格出错%s\n", msg);
            return;
        }
    }
    else
        printf("%s表插入成功\n", table_name);
}
QN get_table(sqlite3 *db, char *sql) // 获取表格数据
{
    // SELECT column1, column2, ...FROM table_name;
    QN qn;
    char *msg = 0;
    int data = sqlite3_get_table(db, sql, &qn.ppResult, &qn.row, &qn.col, &msg);
    if (data < 0)
    {
        printf("%s\n", msg);
    }
    return qn;
}
void creat_table(sqlite3 *db, char *table_name, char *table_column1s) // 创建表
{
    char *msg = 0;
    char sql[258];
    sprintf(sql, "CREATE TABLE %s(%s)", table_name, table_column1s);
    sqlite3_exec(db, sql, 0, 0, &msg); // 执行sql串的命令
    if (msg != 0)
    {
        // 如果是因为表格已经存在而出错就不需要退出程序，后面可以继续操作表格
        if (strcmp("table user already exists", msg) != 0)
        {
            printf("执行创建表格出错%s\n", msg);
            return;
        }
        else
            printf("%s表打开成功\n", table_name);
    }
}
// 注册处理函数
void _resgist_server(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    msg m = m1;
    char buf[128];
    user *u = (user *)&m.data;
    char sql[512];
    sprintf(sql, "SELECT * FROM user where name='%s';", u->name); // 用来判断是否注册过
    QN qn = get_table(db, sql);
    sprintf(buf, "'%s','%s'", u->name, u->password);
    // printf("%s %s\n",u->name,u->password);
    if (qn.col == 0)
    {
        m.type = 11;
        creat_table(db, u->name, "friendname varchar(128)"); // 在注册成功时，同时创建自己的friend表
        insert_table(db, "user", "name,password", buf);
        write(events[n].data.fd, &m, sizeof(m));
    }
    else
    {
        m.type = 111;
        write(events[n].data.fd, &m, sizeof(m));
    }
}
// 用户登录处理函数
void _login_server(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    msg m = m1;
    char buf[128];
    user *u = (user *)&m.data;
    char sql[512];
    sprintf(sql, "SELECT * FROM user where name='%s'and password='%s'", u->name, u->password); // 用来判断是否注册过
    QN qn = get_table(db, sql);
    // for(int i=qn.col;i<qn.col*qn.row+qn.col;i++)
    // {
    //     printf("%s\n",qn.ppResult[i]);
    // }
    if (qn.col == 0)
    {
        m.type = 222;
        write(events[n].data.fd, &m, sizeof(m));
    }
    else
    {
        logins[cout].fd = events[n].data.fd;
        strcpy(logins[cout].login_name, u->name);
        cout++;
        m.type = 22;
        write(events[n].data.fd, &m, sizeof(m));
    }
}
// 用户好友显示处理函数
void _signal_friendShow(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    msg m = m1;
    int i;
    for (i = 0; i < cout; i++)
    {
        if (events[n].data.fd == logins[i].fd)
        {
            break;
        }
    }
    char sql[128];
    sprintf(sql, "SELECT * FROM %s", logins[i].login_name);
    QN qn = get_table(db, sql);
    int j = 0;
    int nums = 0;
    for (j = qn.col; j < qn.col * qn.row + qn.col; j++)
    {
        friend f;
        for (int k = 0; k < cout; k++)
        {
            if (strcmp(qn.ppResult[j], logins[k].login_name) == 0)
            {
                strcpy(f.state, "yes");
                nums++;
                break;
            }
            else
            {
                strcpy(f.state, "no");
            }
        }
        strcpy(f.name, qn.ppResult[j]);
        memcpy(m.data, &f, sizeof(f));
        write(events[n].data.fd, &m, sizeof(m));
    }
    m.type = (short)nums;
    memset(&m.data, 0, sizeof(m.data));
    write(events[n].data.fd, &m, sizeof(m));
}
// 好友私聊函数
void _signal_friendchat(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    msg m = m1;
    int i = 0;
    int j = 0;
    friend_msg fms = *(friend_msg *)&m.data;
    for (j = 0; j < cout; j++)
    {
        if (logins[j].fd == events[n].data.fd)
        {
            break;
        }
    }
    for (i = 0; i < cout; i++)
    {
        if (strcmp(logins[i].login_name, fms.name) == 0)
        {
            strcpy(fms.name, logins[j].login_name);
            memcpy(&m.data, &fms, sizeof(fms));
            write(logins[i].fd, &m, sizeof(m));
            break;
        }
    }
}
// 添加好友函数发送请求
void _signal_friendadd(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    int i;
    int j;
    msg m = m1;
    m.type = 51;
    friend_add *f = (friend_add *)&m.data;
    for (i = 0; i < cout; i++)
    {
        if (strcmp(logins[i].login_name, f->name2) == 0) // 根据人名定位到对面的人
        {
            break;
        }
    }
    for (j = 0; j < cout; j++)
    {
        if (strcmp(logins[j].login_name, f->name1) == 0) // 根据人名定位到自己的fd
        {
            break;
        }
    }
    write(logins[i].fd, &m, sizeof(m));
}
void _signal_friendadd_52(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{

    msg m = m1;
    friend_add *f = (friend_add *)&m.data;
    int i, j;
    for (i = 0; i < cout; i++)
    {
        if (strcmp(logins[i].login_name, f->name2) == 0) // 根据人名定位到对面的人
        {
            break;
        }
    }
    for (j = 0; j < cout; j++)
    {
        if (strcmp(logins[j].login_name, f->name1) == 0) // 根据人名定位到自己的fd
        {
            break;
        }
    }
    printf("22222%s\n", f->flag);
    if (strcmp(f->flag, "yes") == 0)
    {
        m.type = 53;
        char vlaue1[128];
        char vlaue2[128];
        sprintf(vlaue1, "'%s'", logins[j].login_name);
        sprintf(vlaue2, "'%s'", logins[i].login_name);
        insert_table(db, logins[i].login_name, "friendname", vlaue1);
        insert_table(db, logins[j].login_name, "friendname", vlaue2);
        write(logins[j].fd, &m, sizeof(m));
    }
    else
    {
        m.type = 54;
        write(logins[j].fd, &m, sizeof(m));
    }
    return;
}
// 删除好友
void _signal_friendel(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    msg m = m1;
    friend_change *f = (friend_change *)&m.data;
    char sql[128];
    sprintf(sql, "SELECT * FROM %s", f->name1);
    QN qn = get_table(db, sql);
    for (int i = qn.col; i < qn.col * qn.row + qn.col; i++)
    {
        if (strcmp(qn.ppResult[i], f->name2) == 0)
        {
            m.type = 66;
            del_table(db, f->name1, "friendname", f->name2);
            write(events[n].data.fd, &m, sizeof(m));
            return;
        }
    }
    write(events[n].data.fd, &m, sizeof(m));
}

// 查看群聊所有群聊
void _group_look(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    msg m = m1;
    if (m.type == 7)
    {
        char sql[128];
        sprintf(sql, "SELECT * FROM groups");
        QN qn = get_table(db, sql);
        int j = 0;
        int nums = 0;
        for (j = qn.col; j < qn.col * qn.row + qn.col; j++)
        {
            strcpy(m.data, qn.ppResult[j]);
            write(events[n].data.fd, &m, sizeof(m));
        }
        m.type = 77;
        memset(&m.data, 0, sizeof(m.data));
        write(events[n].data.fd, &m, sizeof(m));
    }
}
// 选择群聊，同时进行聊天
void _group_chat(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    msg m = m1;
    m.type = 81;
    char sql[128];
    sprintf(sql, "SELECT * FROM %s", m.data); // 获取m.data的名字
    char sql1[128];
    sprintf(sql1, "SELECT * FROM groups");
    QN qn = get_table(db, sql);
    QN qn1 = get_table(db, sql1);
    int j = 0;
    int i = 0;
    int k = 0;
    for (i; i < cout; i++)
    {
        if (logins[i].fd == events[n].data.fd)
        {
            break;
        }
    }
    for (j = qn1.col; j < qn1.col * qn1.row + qn1.col; j++) // 在groups中找群聊
    {
        if (strcmp(qn1.ppResult[j], m.data) == 0)
        {
            break;
        }
    }
    if (j == (qn1.col * qn1.row + qn1.col))
    {
        m.type = 83;
        write(events[n].data.fd, &m, sizeof(m));
        return;
    }
    for (k = qn.col; k < qn.col * qn.row + qn.col; k++) // 在群聊里找你
    {
        if (strcmp(qn.ppResult[k], logins[i].login_name) == 0)
        {
            break;
        }
    }
    if (k == (qn.col * qn.row + qn.col))
    {
        m.type = 82;
        write(events[n].data.fd, &m, sizeof(m));
        return;
    }
    write(events[n].data.fd, &m, sizeof(m));
}
// 对81的处理
void _group_chat_81(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    msg m = m1;
    friend_msg f = *(friend_msg *)&m.data;
    char sql[128];
    sprintf(sql, "SELECT * FROM %s", f.name); // 获取m.data的名字
    QN qn = get_table(db, sql);
    int j = 0;
    int i = 0;
    for (i; i < cout; i++)
    {
        if (logins[i].fd == events[n].data.fd)
        {
            break;
        }
    }
    for (int x = 0; x < cout; x++)
    {
        for (int l = qn.col; l < qn.col * qn.row + qn.col; l++) // 在群聊里找你
        {
            if (strcmp(qn.ppResult[l], logins[x].login_name) == 0 && strcmp(logins[x].login_name, logins[i].login_name) != 0)
            {
                strcpy(f.name, logins[i].login_name);
                memcpy(m.data, &f, sizeof(f));
                write(logins[x].fd, &m, sizeof(m));
            }
        }
    }
}
// 群聊退出
void _group_quit(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    msg m = m1;
    m.type = 91;
    char sql[128];
    sprintf(sql, "SELECT * FROM %s", m.data);
    char sql1[128];
    sprintf(sql1, "SELECT * FROM groups");
    QN qn = get_table(db, sql);
    QN qn1 = get_table(db, sql1);
    int j = 0;
    int i = 0;
    int k = 0;
    for (i; i < cout; i++)
    {
        if (logins[i].fd == events[n].data.fd)
        {
            break;
        }
    }
    for (j = qn1.col; j < qn1.col * qn1.row + qn1.col; j++) // 在groups中找群聊
    {
        if (strcmp(qn1.ppResult[j], m.data) == 0)
        {
            break;
        }
    }
    if (j == (qn1.col * qn1.row + qn1.col))
    {
        m.type = 93;
        write(events[n].data.fd, &m, sizeof(m));
        return;
    }
    for (k = qn.col; k < qn.col * qn.row + qn.col; k++) // 在群聊里找你
    {
        if (strcmp(qn.ppResult[k], logins[i].login_name) == 0)
        {
            break;
        }
    }
    if (k == (qn.col * qn.row + qn.col))
    {
        m.type = 92;
        write(events[n].data.fd, &m, sizeof(m));
        return;
    }
    del_table(db, m.data, "number", logins[i].login_name);
    write(events[n].data.fd, &m, sizeof(m));
}
// 直接加入群聊无需同意的
void _group_join(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    msg m = m1;
    m.type = 101;
    char sql[128];
    sprintf(sql, "SELECT * FROM %s", m.data);
    char sql1[128];
    sprintf(sql1, "SELECT * FROM groups");
    QN qn = get_table(db, sql);
    QN qn1 = get_table(db, sql1);
    int j = 0;
    int i = 0;
    int k = 0;
    for (i; i < cout; i++)
    {
        if (logins[i].fd == events[n].data.fd)
        {
            break;
        }
    }
    for (j = qn.col; j < qn.col * qn.row + qn.col; j++)
    {
        if (strcmp(qn.ppResult[j], logins[i].login_name) == 0)
        {
            m.type = 102;
            write(events[n].data.fd, &m, sizeof(m));
            return;
        }
    }
    for (k = qn1.col; k < qn1.col * qn1.row + qn1.col; k++) // 在groups中找群聊
    {
        if (strcmp(qn1.ppResult[k], m.data) == 0)
        {
            break;
        }
    }
    if (k == (qn1.col * qn1.row + qn1.col))
    {
        m.type = 103;
        write(events[n].data.fd, &m, sizeof(m));
        return;
    }
    char b[128];
    sprintf(b, "'%s','common'", logins[i].login_name);
    insert_table(db, m.data, "number,poor", b);
    write(events[n].data.fd, &m, sizeof(m));
}
// 创建群聊
void _group_create(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    msg m = m1;
    m.type = 111;
    int j = 0;
    for (j = 0; j < cout; j++)
    {
        if (logins[j].fd == events[n].data.fd)
        {
            break;
        }
    }
    char b[128];
    creat_table(db, m.data, "number varchar(128),poor varchar(128)");
    sprintf(b, "'%s','lader'", logins[j].login_name);
    insert_table(db, m.data, "number,poor", b);
    char c[128];
    sprintf(c, "'%s'", m.data);
    insert_table(db, "groups", "groupname", c);
    write(events[n].data.fd, &m, sizeof(m));
}
// 查看自己加入的组
void _group_mygroup(msg m1, struct epoll_event events[MAX_EVENTS], int n)
{
    msg m = m1;
    char sql[128];
    char sql1[128];
    sprintf(sql1, "SELECT * FROM groups");
    QN qn1 = get_table(db, sql1);
    int j = 0;
    int i = 0;
    int k = 0;
    for (i; i < cout; i++)
    {
        if (logins[i].fd == events[n].data.fd)
        {
            break;
        }
    }
    for (j = qn1.col; j < qn1.col * qn1.row + qn1.col; j++) // 在groups中找群聊
    {
        sprintf(sql, "SELECT * FROM %s", qn1.ppResult[j]);
        QN qn = get_table(db, sql);
        for (k = qn.col; k < qn.col * qn.row + qn.col; k++) // 在群聊里找你
        {
            if (strcmp(qn.ppResult[k], logins[i].login_name) == 0)
            {
                strcpy(m.data, qn1.ppResult[j]);
                write(events[n].data.fd, &m, sizeof(m));
            }
        }
    }
    m.type = 121;
    write(events[n].data.fd, &m, sizeof(m));
}
// 文件发送
void send_file_request_to_client(int sender_fd, int receiver_fd, file_packet *packet)
{
    msg m;
    m.type = 101; // 文件传输请求消息类型
    memcpy(m.data, packet, sizeof(file_packet));
    write(receiver_fd, &m, sizeof(m));
}

void handle_file_request(msg m, struct epoll_event events[MAX_EVENTS], int n)
{
    file_packet *packet = (file_packet *)&m.data;
    int receiver_fd = -1;

    // 查找目标客户端的文件描述符
    for (int i = 0; i < cout; i++)
    {
        if (strcmp(logins[i].login_name, packet->receiver_name) == 0)
        {
            receiver_fd = logins[i].fd;
            break;
        }
    }

    if (receiver_fd != -1)
    {
        send_file_request_to_client(events[n].data.fd, receiver_fd, packet);
    }
    else
    {
        // 目标客户端不在线或未找到
        printf("Receiver not found or offline.\n");
    }
}
void handle_file_transfer1(msg m, struct epoll_event events[MAX_EVENTS], int n)
{
    file_packet packet = *(file_packet *)&m.data;
    int i = 0;
    for (i = 0; i < cout; i++)
    {
        if (strcmp(logins[i].login_name, packet.sender_name) == 0)
        {
            break;
        }
    }
    m.type = 103;
    write(logins[i].fd, &m, sizeof(m));
}
void handle_file_transfer(msg m, struct epoll_event events[MAX_EVENTS], int n)
{
    file_packet *packet = (file_packet *)&m.data;
    int receiver_fd = -1;

    // 查找目标客户端的文件描述符
    for (int i = 0; i < cout; i++)
    {
        if (strcmp(logins[i].login_name, packet->receiver_name) == 0)
        {
            receiver_fd = logins[i].fd;
            break;
        }
    }

    if (receiver_fd != -1)
    {
        write(receiver_fd, packet, sizeof(file_packet));
    }
    else
    {
        // 目标客户端不在线或未找到
        printf("Receiver not found or offline.\n");
    }
}
void file_only(msg m, struct epoll_event events[MAX_EVENTS], int n)
{
    file_packet *packet = (file_packet *)&m.data;
    int receiver_fd = -1;
    for (int i = 0; i < cout; i++)
    {
        if (strcmp(logins[i].login_name, packet->receiver_name) == 0)
        {
            receiver_fd = logins[i].fd;
            break;
        }
    }
    write(receiver_fd, &m, sizeof(m));
}
int main()
{
    // 打开数据库
    int data = -1;
    data = sqlite3_open("user.db", &db);
    if (data < 0)
    {
        printf("数据库user.db打开失败\n");
        return 0;
    }
    else
    {
        printf("user.db库打开成功\n");
    }
    int val = 1;
    struct sockaddr_in addr = getaddr("192.168.150.130", 9000);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (-1 == bind(fd, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("绑定端口");
        return 0;
    }
    listen(fd, 5);
    struct epoll_event ev, events[MAX_EVENTS];
    int listen_sock, conn_sock, nfds, epollfd;

    /* Code to set up listening socket, 'listen_sock',
       (socket(), bind(), listen()) omitted */

    epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    while (1) // 核心代码
    {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1); // 等待监听epollfd对象中的文件描述符发什事件，并存入events中
        if (nfds == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        for (int n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == fd)
            {
                struct sockaddr_in _send;
                socklen_t _send_len = sizeof(_send);
                int newfd = accept(fd, (struct sockaddr *)&_send, (socklen_t *)&_send_len);
                if (newfd == -1)
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                ev.events = EPOLLIN;
                ev.data.fd = newfd;
                printf("fd:%d上线\n", newfd);
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, newfd, &ev) == -1)
                {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            }
            else // 监听到客户端后，要执行的选择
            {
                msg m;
                int ret = read(events[n].data.fd, &m, sizeof(m));
                if (ret == -1 || ret == 0)
                {
                    printf("fd:%d下线\n", events[n].data.fd);
                    ev.events = EPOLLIN;
                    ev.data.fd = events[n].data.fd;
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev) == -1)
                    {
                        perror("epoll_ctl: conn_sock");
                    }
                    for (int i = 0; i < cout; i++)
                    {
                        if (logins[i].fd == events[n].data.fd)
                        {
                            for (int j = i; j < cout; j++)
                            {
                                logins[j] = logins[j + 1];
                            }
                            cout--;
                            break;
                        }
                    }
                }
                switch (m.type)
                {
                case 1:
                    _resgist_server(m, events, n); // 客户端注册函数处理
                    break;
                case 2:
                    _login_server(m, events, n); // 客户端发来登录，处理
                    break;
                case 3:
                    _signal_friendShow(m, events, n); // 客户端发来好友显示处理函数
                    break;
                case 4:
                    _signal_friendchat(m, events, n); // 客户端发来好友私聊处理函数
                    break;
                case 5:
                    _signal_friendadd(m, events, n); // 客户端发来加好友请求
                    break;
                case 6:
                    _signal_friendel(m, events, n); // 删除好友
                    break;
                case 7:
                    _group_look(m, events, n); // 查看所有群聊
                    break;
                case 8:
                    _group_chat(m, events, n); // 只能在自己的群聊聊天
                    break;
                case 9:
                    _group_quit(m, events, n);
                    break;
                case 10:
                    _group_join(m, events, n); // 无需同意的加入群聊
                    break;
                case 11:
                    _group_create(m, events, n); // 创建群聊，群聊里有群主和普通用户之分
                    break;
                case 12:
                    _group_mygroup(m, events, n); // 查看我加入的组
                    break;
                case 81:
                    _group_chat_81(m, events, n); // 群聊的附属函数
                    break;
                case 52:
                    _signal_friendadd_52(m, events, n);
                    break;
                case 101: // 文件传输请求消息类型
                    handle_file_request(m, events, n);
                    break;
                case 102: // 文件传输消息类型
                    handle_file_transfer1(m, events, n);
                    break;
                case 103: // 文件传输消息类型
                    handle_file_transfer(m, events, n);
                    break;
                case 104:
                    file_only(m, events, n); // 最后一步，没完成
                    break;
                default:
                    break;
                }
            }
        }
    }

    sqlite3_close(db);
    return 0;
}