#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#define PORT 1234
#define BACKLOG 128

void *start_routine(void *ptr)
{
    int fd = *(int *)ptr;
    char buf[100];
    int numbytes;
    int i, c = 0;
    printf("this is a new thread,you got connected\n");
    printf("fd=%d\n", fd);

    if ((numbytes = recv(fd, buf, 100, 0)) == -1)
    {
        printf("recv() error\n");
        exit(1);
    }

    char str[numbytes];
    char buffer[numbytes];

    //将收到的字符串反转
    for (c = 0; c < (numbytes - 1); c++)
    {
        buffer[c] = buf[c];
    }

    printf("receive message:%s\n", buf);

    printf("numbytes=%d\n", numbytes);

    for (i = 0; i < numbytes; i++)
    {
        str[i] = buf[numbytes - 1 - i];
    }

    printf("server will send:%s\n", str);
    numbytes = send(fd, str, sizeof(str), 0);
    printf("send numbytes=%d\n", numbytes);
    close(fd);
}

int main()
{
    int listenfd, connectfd;
    struct sockaddr_in server;
    struct sockaddr_in client;
    int sin_size;
    sin_size = sizeof(struct sockaddr_in);

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Creating socket failed.");
        exit(1);
    }

    int opt = SO_REUSEADDR;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    // 绑定
    if (bind(listenfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("Bind error.");
        exit(1);
    }
    // 监听
    if (listen(listenfd, BACKLOG) == -1)
    { /* calls listen() */
        perror("listen() error\n");
        exit(1);
    }
    printf("3\n");
    while (1)
    {
        // accept
        if ((connectfd = accept(listenfd, (struct sockaddr *)&client, (socklen_t*)&sin_size)) == -1)
        {
            perror("accept() error\n");
            exit(1);
        }
        printf("You got a connection from %s\n", inet_ntoa(client.sin_addr));
        pthread_t thread; //定义一个线程号
        pthread_create(&thread, NULL, start_routine, (void *)&connectfd);
    }
    close(listenfd);
}
