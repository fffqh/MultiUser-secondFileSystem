#include "SecondFileKernel.h"
#include "User.h"
#include"string.h"
#include"string"
FD SecondFileKernel::Sys_Open(std::string& fpath,int mode)
{
    //模仿系统调用，将参数放入user结构中
    User& u = SecondFileKernel::Instance().GetUser();
    char path[256];
    strcpy(path,fpath.c_str());
	u.u_dirp = path;
    //u.u_arg[0] = (int)path;
    u.u_arg[1] = mode;

    FileManager& fileMgr = SecondFileKernel::Instance().GetFileManager();
	fileMgr.Open();

    //从user结构取出返回值
	return u.u_ar0[User::EAX];	

}
int SecondFileKernel::Sys_Close(FD fd)
{
    User& u = SecondFileKernel::Instance().GetUser();
    u.u_arg[0] = fd;

    FileManager& fileMgr = SecondFileKernel::Instance().GetFileManager();
	fileMgr.Close();

    return u.u_ar0[User::EAX];
}

int SecondFileKernel::Sys_Creat(std::string &fpath,int mode)
{
    //模仿系统调用，将参数放入user结构中
    User& u = SecondFileKernel::Instance().GetUser();
    char path[256];
    strcpy(path,fpath.c_str());
	u.u_dirp = path;
    u.u_arg[0] = (long long)&path;
    u.u_arg[1] = mode;

    FileManager& fileMgr = SecondFileKernel::Instance().GetFileManager();
	fileMgr.Creat();

    //从user结构取出返回值
	return u.u_ar0[User::EAX];	
}
int SecondFileKernel::Sys_Delete(std::string &fpath)
{
    //模仿系统调用，将参数放入user结构中
    User& u = SecondFileKernel::Instance().GetUser();
    char path[256];
    strcpy(path,fpath.c_str());
	u.u_dirp = path;
    u.u_arg[0] = (long long)&path;

    FileManager& fileMgr = SecondFileKernel::Instance().GetFileManager();
	fileMgr.UnLink();

    //从user结构取出返回值
	return u.u_ar0[User::EAX];	
}
int SecondFileKernel::Sys_Read(FD fd, size_t size, size_t nmemb, void *ptr)
{
    if(size>nmemb)return -1;
    //模仿系统调用，将参数放入user结构中
    User& u = SecondFileKernel::Instance().GetUser();

    u.u_arg[0] = fd;
    u.u_arg[1] = (long long)ptr;
    u.u_arg[2] = size;

    FileManager& fileMgr = SecondFileKernel::Instance().GetFileManager();
	fileMgr.Read();

    //从user结构取出返回值
	return u.u_ar0[User::EAX];	

}
int SecondFileKernel::Sys_Write(FD fd, size_t size, size_t nmemb, void *ptr)
{
    if(size>nmemb)return -1;
    //模仿系统调用，将参数放入user结构中
    User& u = SecondFileKernel::Instance().GetUser();

    u.u_arg[0] = fd;
    u.u_arg[1] = (long long)ptr;
    u.u_arg[2] = size;

    FileManager& fileMgr = SecondFileKernel::Instance().GetFileManager();
	fileMgr.Write();

    //从user结构取出返回值
	return u.u_ar0[User::EAX];	
}
int SecondFileKernel::Sys_Seek(FD fd, long int offset, int whence)
{
    //模仿系统调用，将参数放入user结构中
    User& u = SecondFileKernel::Instance().GetUser();

    u.u_arg[0] = fd;
    u.u_arg[1] = offset;
    u.u_arg[2] = whence;

    FileManager& fileMgr = SecondFileKernel::Instance().GetFileManager();
	fileMgr.Seek();

    //从user结构取出返回值
	return u.u_ar0[User::EAX];	
}
