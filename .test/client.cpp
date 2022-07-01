#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
using namespace std;
#define PORT 1234
#define MAXDATASIZE 100

char receiveM[100];
char sendM[100];

int main(int argc, char *argv[])
{
    int fd, numbytes;
    struct hostent *he;
    struct sockaddr_in server;

    //检查用户输入，如果用户输入不正确，提示用户正确的输入方法
    if (argc != 2)
    {
        printf("Usage: %s <IP Address>\n", argv[0]);
        exit(1);
    }

    // 通过函数 gethostbyname()获得字符串形式的ip地址，并赋给he
    if ((he = gethostbyname(argv[1])) == NULL)
    {
        printf("gethostbyname() error\n");
        exit(1);
    }

    cout << 1 << endl;
    // 产生套接字fd
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("socket() error\n");
        exit(1);
    }
    cout << 2 << endl;
    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr = *((struct in_addr *)he->h_addr);
    cout << 3 << endl;
    int ret = connect(fd, (struct sockaddr *)&server, sizeof(struct sockaddr));
    if (ret == -1)
    {
        printf("connect() error\n");
        exit(1);
    }

    cout << 4 << endl;

    // 向服务器发送数据

    while (1)
    {
        printf("send message to server:");

        fgets(sendM, 1024, stdin);
        int send_le;
        send_le = strlen(sendM);
        sendM[send_le - 1] = '\0';
        send(fd, sendM, strlen(sendM), 0);

        // 从服务器接收数据
        if ((numbytes = recv(fd, receiveM, MAXDATASIZE, 0)) == -1)
        {
            printf("recv() error\n");
            exit(1);
        }

        printf("receive message from server:%s\n", receiveM);
    }
    close(fd);
}