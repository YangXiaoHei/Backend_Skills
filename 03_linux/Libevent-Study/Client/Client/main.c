#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include <event.h>
#include <event2/util.h>



int tcp_connect_server(const char* server_ip, int port);

void cmd_msg_cb(int fd, short events, void* arg);
void socket_read_cb(int fd, short events, void *arg);

#define IP   "10.10.3.7"
#define PORT "8000"

/**
 *  1、创建 TCP 套接字
    2、监听 socket 可读事件
    3、监听终端输入事件
    4、开启事件循环
 */
int main(int argc, char** argv)
{
    /* 创建 socket，连接服务器，并返回 socket 文件描述符 */
    int sockfd = tcp_connect_server(IP, atoi(PORT));
    if( sockfd == -1)
    {
        perror("tcp_connect error ");
        return -1;
    }
    
    printf("connect to server successful\n");
    
    struct event_base* base = event_base_new();
    
    /* 监听 sockfd 的可读事件 */
    struct event *ev_sockfd = event_new(base, sockfd,
                                        EV_READ | EV_PERSIST,
                                        socket_read_cb, NULL);
    event_add(ev_sockfd, NULL);
    
    /* 监听终端输入事件 */
    struct event* ev_cmd = event_new(base, STDIN_FILENO,
                                     EV_READ | EV_PERSIST,
                                     cmd_msg_cb,
                                     (void*)&sockfd);
    event_add(ev_cmd, NULL);
    
    /* 开启事件循环 */
    event_base_dispatch(base);
    
    printf("finished \n");
    return 0;
}

void cmd_msg_cb(int fd, short events, void* arg)
{
    char msg[1024];
    
    /* 从终端读取输入 */
    ssize_t ret = read(fd, msg, sizeof(msg));
    if( ret <= 0 )
    {
        perror("read fail ");
        exit(1);
    }
    
    int sockfd = *((int*)arg);
    
    /* 将终端读到的数据发送给服务器 */
    write(sockfd, msg, ret);
}


void socket_read_cb(int fd, short events, void *arg)
{
    char msg[1024];
    
    //为了简单起见，不考虑读一半数据的情况
    ssize_t len = read(fd, msg, sizeof(msg)-1);
    if( len <= 0 )
    {
        perror("read fail ");
        exit(1);
    }
    
    msg[len] = '\0';
    
    printf("recv %s from server\n", msg);
}

typedef struct sockaddr SA;
int tcp_connect_server(const char* server_ip, int port)
{
    int sockfd = 0, save_errno;
    
    /* 服务器套接字地址 */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr) );
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_aton(server_ip, &server_addr.sin_addr);
    
    /* 检测 IP 是否有效 */
    if( server_addr.sin_addr.s_addr == 0 )
    {
        errno = EINVAL;
        return -1;
    }
    
    /* 创建套接字 */
    if( (sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        return sockfd;
    
    /* 向服务器发起 connect */
    if( connect(sockfd, (SA*)&server_addr, sizeof(server_addr)) < 0 )
    {
        save_errno = errno;
        close(sockfd);
        errno = save_errno; //the close may be error
        return -1;
    }
    
    evutil_make_socket_nonblocking(sockfd);
    
    return sockfd;
}


