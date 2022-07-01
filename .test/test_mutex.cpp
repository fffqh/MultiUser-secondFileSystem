#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include<algorithm>
static pthread_mutex_t g_mutex_lock;
int i ;
static void* thread_fun_1(void *data)
{
    //sleep(1);
    while (1) {
        pthread_mutex_lock(&g_mutex_lock);
        printf("%d hello\n",i);
        if(i >= 10)break;
        i++;
        sleep(1);
        pthread_mutex_unlock(&g_mutex_lock);
        usleep(1);
    }

    
    return &g_mutex_lock;
}

static void *thread_fun_2(void *data)
{

    while (1) {
        pthread_mutex_lock(&g_mutex_lock);
        printf("%d world\n",i);
        if(i >= 10)break;
        i++;
        sleep(1);
        pthread_mutex_unlock(&g_mutex_lock);
        usleep(1);
    }

    return &g_mutex_lock;
}

static void do_print_hello()
{
    printf("do prient hello\n");
    pthread_t pth_id;
    pthread_create(&pth_id, NULL, thread_fun_1, NULL);
}

static void do_print_world()
{
    printf("do prient world\n");
    pthread_t pth_id;
    pthread_create(&pth_id, NULL, thread_fun_2, NULL);
}

int main(int argc, char const *argv[])
{
    int x = std::min(10,2+8+9);
    printf("%d\n",x);
    int ret;
    int cid;
    i=0;
    int loop=1;
    ret = pthread_mutex_init(&g_mutex_lock, NULL);
    if (ret != 0) {
        printf("mutex init failed\n");
        return -1;
    }

    while (loop) {
        scanf("%d", &cid);
        getchar();

        switch (cid) {
            case 0:
                do_print_hello();
            break;

            case 1:
                do_print_world();
            break;

            default:
            loop=0;
            break;
        }
    }

    pthread_mutex_destroy(&g_mutex_lock);

    return 0;
}
