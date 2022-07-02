#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAXDATASIZE 1024

void running(int fd)
{
    int numbytes;
    char receiveM[1025] = {0};
    char sendM[1025] = {0};
    bool first_recv = 0;


    while(true){
        // 非阻塞接收：提示消息
        while(1){
            numbytes = recv(fd, receiveM, MAXDATASIZE, 0);
            if (numbytes == 0){
                printf("[ERROR] 连接被服务端关闭，客户端退出.\n");
                return;
            }
            if(numbytes == -1){
                if(errno != EAGAIN &&errno != EWOULDBLOCK && errno != EINTR){
                    printf("[ERROR] recv 错误, 错误码%d(%s)\n", errno, strerror(errno));
                    return;
                }
                if(errno == EINTR)
                    continue;
                if(!first_recv)
                    continue;
            }
            if(numbytes > 0){
                //printf("[INFO] revc %d bytes\n", numbytes);
                receiveM[numbytes] = 0;
                printf("%s", receiveM);
                if(!first_recv)
                    first_recv = 1;
            }
            break;
        }

        // 输入：用户输入
        fgets(sendM, 1024, stdin);
        int send_le;
        send_le = strlen(sendM);
        sendM[send_le - 1] = '\0'; // 去掉回车
        if(send_le == 1){
            send_le += 1;
            sendM[0] = ' ';
        }
        // 发送：用户输入
        if((numbytes=send(fd, sendM, send_le-1, 0)) == -1){
            printf("[EROOR] send error\n");
            return;
        }
        //printf("[INFO] send %d bytes\n", numbytes);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3){
        printf("please input ip and port, for example ./main 120.12.34.56 80.\n");
        return -1;
    }
    char *ipaddr = argv[1];
    unsigned int port = atoi(argv[2]);
    
    int fd = 0;
    struct sockaddr_in addr;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0){
        fprintf(stderr, "create socket failed,error:%s.\n", strerror(errno));
        return -1;
    }

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ipaddr, &addr.sin_addr);

    /*设置套接字为非阻塞*/
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0){
        fprintf(stderr, "Set flags error:%s\n", strerror(errno));
        close(fd);
        return -1;
    }

    /*建立连接：阻塞情况下linux系统默认超时时间为75s*/
    int cnt = 1;
    while(true){
        int rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
        // 连接成功
        if(rc == 0){
            printf("[INFO] 与服务器连接成功.\n");
            break;
        }
        if(cnt > 5){
            printf("[ERROR] 连接失败，客户端退出.\n");
            return 0;
        }
        printf("[INFO] 第 %d 次连接失败，尝试重连...\n", cnt++);
    }

    running(fd);
    close(fd);
    printf("[INFO] 关闭连接，客户端退出.\n");
    return 0;
}
