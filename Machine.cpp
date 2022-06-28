// fqh
#include "Machine.h"
#include "SecondFileKernel.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <iostream>
using namespace std;

Machine::Machine()
{
}
Machine::~Machine()
{
}
void Machine::Initialize()
{
    // 0. 取得 BufferManager
    this->m_BufferManager = &SecondFileKernel::Instance().GetBufferManager();

    // 1. 打开文件
    int fd = open(devpath, O_RDWR);
    if (fd == -1)
    {
        fd = open(devpath, O_RDWR | O_CREAT, 0666);
        if (fd == -1)
        {
            printf("[error] Machine Init Error: 创建 %s 失败\n", devpath);
            exit(-1);
        }
        // 对磁盘进行初始化
        this->init_img(fd);
        // cout << "[INFO] 磁盘初始化完毕." <<endl;
    }
    // 2. mmap
    mmap_img(fd);
    this->img_fd = fd;
    cout << "[info] 磁盘mmap映射完毕." << endl;
}

// 文件系统退出
void Machine::quit()
{
    struct stat st; //定义文件信息结构体
    /*取得文件大小*/
    int r = fstat(this->img_fd, &st);
    if (r == -1)
    {
        printf("[error]获取img文件信息失败，文件系统异常退出.\n");
        close(this->img_fd);
        exit(-1);
    }
    int len = st.st_size;
    msync((void *)(this->m_BufferManager->GetP()), len, MS_SYNC);
    
}

void Machine::init_spb(SuperBlock& sb)
{
    sb.s_isize = FileSystem::INODE_ZONE_SIZE;
    sb.s_fsize = FileSystem::DATA_ZONE_END_SECTOR + 1;
    sb.s_nfree = (FileSystem::DATA_ZONE_SIZE - 99) % 100;

    // 找到最后一个盘块组的第一个盘块
    int start_last_datablk = FileSystem::DATA_ZONE_START_SECTOR;
    while(true){
        if((start_last_datablk + 100 -1) < FileSystem::DATA_ZONE_END_SECTOR)
            start_last_datablk += 100;
        else
            break;
    }
    start_last_datablk--;
    // 将最后一个盘块组的盘块号填入
    for(int i = 0; i < sb.s_nfree; ++i)
        sb.s_free[i] = start_last_datablk + i;

    sb.s_ninode = 100;
    for(int i = 0; i < sb.s_ninode; ++i)
        sb.s_inode[i] = i;
    
    // sb.s_flock = 0;
    // sb.s_ilock = 0;
    sb.s_fmod  = 0;
    sb.s_ronly = 0;
}

void Machine::init_db(char* data)
{
    struct
    {
        int nfree;     //本组空闲的个数
        int free[100]; //本组空闲的索引表
    } tmp_table;

    int last_datablk_num = FileSystem::DATA_ZONE_SIZE; //未加入索引的盘块的数量
    // 初始化组长盘块
    for(int i = 0; ; i++)
    {
        if (last_datablk_num >= 100)
            tmp_table.nfree = 100;
        else
            tmp_table.nfree = last_datablk_num;
        last_datablk_num -= tmp_table.nfree;

        for (int j = 0; j < tmp_table.nfree; j++)
        {
            if (i == 0 && j == 0)
                tmp_table.free[j] = 0;
            else
            {
                tmp_table.free[j] = 100 * i + j + FileSystem::DATA_ZONE_START_SECTOR - 1;
            }
        }
        memcpy(&data[99 * 512 + i * 100 * 512], (void *)&tmp_table, sizeof(tmp_table));
        if (last_datablk_num == 0)
            break;
    }
}

void Machine::init_img(int fd)
{
    SuperBlock spb;
    init_spb(spb);
    DiskInode *di_table = new DiskInode[FileSystem::INODE_ZONE_SIZE*FileSystem::INODE_NUMBER_PER_SECTOR];

    // 设置 rootDiskInode 的初始值
    di_table[0].d_mode = Inode::IFDIR;  // 文件类型为目录文件
    di_table[0].d_mode |= Inode::IEXEC; // 文件的执行权限

    char* datablock = new char[FileSystem::DATA_ZONE_SIZE * 512];
    memset(datablock, 0, FileSystem::DATA_ZONE_SIZE * 512);
    init_db(datablock);

    // 写入文件
    write(fd, &spb, sizeof(SuperBlock));
    write(fd, di_table, FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR * sizeof(DiskInode));
    write(fd, datablock, FileSystem::DATA_ZONE_SIZE * 512);

    printf("[info] 格式化磁盘完毕...");
}

void Machine::mmap_img(int fd)
{
    struct stat st; //定义文件信息结构体
    /*取得文件大小*/
    int r = fstat(fd, &st);
    if (r == -1)
    {
        printf("[error]获取img文件信息失败，文件系统启动中止\n");
        close(fd);
        exit(-1);
    }
    int len = st.st_size;
    /*把文件映射成虚拟内存地址*/
    char *addr = (char*)mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    this->m_BufferManager->SetP(addr);
}

