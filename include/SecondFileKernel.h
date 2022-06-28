#ifndef SECONDFILEKERNEL
#define SECONDFILEKERNEL

#include <string>
// #include <stddef.h>
#include "Machine.h"

#include "FileManager.h"
#include "FileSystem.h"
#include "BufferManager.h"
#include "User.h"


// 文件开头位置
#define SYS_SEEK_SET 0
// 文件指针当前位置
#define SYS_SEEK_CUR 1
// 文件结尾位置
#define SYS_SEEK_END 2

// 文件描述符类型
typedef unsigned int FD;


class SecondFileKernel
{
public:
// 定义一些全局常量


// 工具函数
    SecondFileKernel();
    ~SecondFileKernel();
    static SecondFileKernel& Instance();
    void Initialize(); //文件系统初始化
    void Quit();       //退出文件系统
// Kernel的子组件
    Machine& GetMachine();
    BufferManager& GetBufferManager();
    FileSystem& GetFileSystem();
    FileManager& GetFileManager();
    User& GetUser(); //作为单体实例，单用户时放在这里，多用户时放在线程局部数据中

// Kernel提供的文件系统API
    FD Sys_Open(std::string& fpath,int mode=File::FWRITE);
    int Sys_Close(FD fd);
    int Sys_Creat(std::string& fpath,int mode=File::FWRITE);
    int Sys_Delete(std::string& fpath);
    int Sys_Read(FD fd, size_t size, size_t nmemb, void* ptr);
    int Sys_Write(FD fd, size_t size, size_t nmemb, void* ptr);
    /*whence : 0 设为offset；1 加offset；2 文件结束位置加offset*/
    int Sys_Seek(FD fd, long int offset, int whence);

private:
// Kernel子组件的初始化函数
    void InitMachine();
    void InitFileSystem();
    void InitBuffer();
    void InitUser();

private:
    static SecondFileKernel instance; // 单体实例

// 指向子组件的指针
    Machine* m_Machine;
    BufferManager*  m_BufferManager;
    FileSystem*  m_FileSystem;
    FileManager* m_FileManager;
    User* m_User;
    
};
#endif

