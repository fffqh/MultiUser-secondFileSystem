#include "SecondFileKernel.h"
#include <string.h>

// 全局单体实例
SecondFileKernel SecondFileKernel::instance;

Machine g_Machine;
BufferManager g_BufferManager;
FileSystem g_FileSystem;
FileManager g_FileManager;
User g_User;
UserManager g_UserManager;

char returnBuf[256];

// 构建与析构
SecondFileKernel::SecondFileKernel()
{
}
SecondFileKernel::~SecondFileKernel()
{
}

// 初始化函数：子组件和Kernel
void SecondFileKernel::InitMachine()
{
    this->m_Machine = &g_Machine;
    this->m_Machine->Initialize(); // 进行img的初始化和mmap    
}
void SecondFileKernel::InitBuffer()
{
    this->m_BufferManager = &g_BufferManager;
    this->m_BufferManager->Initialize(); // 进行img的初始化和mmap   
}
void SecondFileKernel::InitFileSystem()
{
    this->m_FileSystem = &g_FileSystem;
    this->m_FileSystem->Initialize();
    this->m_FileManager = &g_FileManager;
    this->m_FileManager->Initialize();
}
void SecondFileKernel::InitUser()
{
    this->m_User = &g_User;
    this->m_UserManager = &g_UserManager;
    //unix中u_ar0指向寄存器，此处为其分配一段空间即可
    // this->m_User->u_ar0 = (unsigned int*)&returnBuf;
}

void SecondFileKernel::Initialize()
{
    InitBuffer();
    InitMachine(); // 包括初始化模拟磁盘
    InitFileSystem();
    InitUser();

    // 进入系统初始化
    FileManager &fileMgr = SecondFileKernel::Instance().GetFileManager();
    fileMgr.rootDirInode = g_InodeTable.IGet(FileSystem::ROOTINO);
    fileMgr.rootDirInode->i_flag &= (~Inode::ILOCK);
	pthread_mutex_unlock(& fileMgr.rootDirInode->mutex);

    SecondFileKernel::Instance().GetFileSystem().LoadSuperBlock();
    User &us = SecondFileKernel::Instance().GetUser();
    us.u_cdir = g_InodeTable.IGet(FileSystem::ROOTINO);
    us.u_cdir->i_flag &= (~Inode::ILOCK);
	pthread_mutex_unlock(& us.u_cdir->mutex);
    strcpy(us.u_curdir, "/");

    printf("[info] 文件系统初始化完毕.\n");
}

void SecondFileKernel::Quit()
{
    this->m_BufferManager->Bflush();
    this->m_FileManager->m_InodeTable->UpdateInodeTable();
    this->m_FileSystem->Update();
    this->m_Machine->quit();
}

// 其他文件获取单体实例对象的接口函数
SecondFileKernel& SecondFileKernel::Instance()
{
    return SecondFileKernel::instance;
}

Machine &SecondFileKernel::GetMachine()
{
    return *(this->m_Machine);
}
BufferManager & SecondFileKernel::GetBufferManager()
{
    return *(this->m_BufferManager);
}
FileSystem & SecondFileKernel::GetFileSystem()
{
    return *(this->m_FileSystem);
}
FileManager &SecondFileKernel::GetFileManager()
{
    return *(this->m_FileManager);
}

// 废弃
User &SecondFileKernel::GetSuperUser()
{
    return *(this->m_User);
}

// 使用
User &SecondFileKernel::GetUser()
{
    return *(this->m_UserManager->GetUser());
}

UserManager& SecondFileKernel::GetUserManager()
{
    return *(this->m_UserManager);
}

