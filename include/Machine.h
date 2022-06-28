// 负责使用LinuxAPI对模拟磁盘文件进行操作（包括磁盘的初始化）
#pragma once
#include "FileSystem.h"
#include "BufferManager.h"

class Machine
{
public:
Machine();
~Machine();

void Initialize();
void quit();

private:
    void init_spb(SuperBlock &sb);
    void init_db(char* data);
    void init_img(int fd);
    void mmap_img(int fd);
private:
    const char* devpath = "c.img";
    int img_fd; // devpath的fd，文件系统打开时 open，关闭时 close.
    BufferManager *m_BufferManager; /* FileSystem类需要缓存管理模块(BufferManager)提供的接口 */

};
