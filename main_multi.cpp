#include "SecondFileKernel.h"
#include <iostream>
#include <string>
#include <string.h>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      
#include <strings.h>      
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include<pthread.h>
#define PORT 1235
#define BACKLOG 128

using namespace std;

bool isNumber(const string& str)
{
    for (char const &c : str) {
        if (std::isdigit(c) == 0) return false;
    }
    return true;
}

stringstream print_head(){
	stringstream send_str;
    send_str << "===============================================" << endl;
    send_str << "||请在一行中依次输入需要调用的函数名称及其参数  ||" << endl;
    send_str << "||open(char *name, int mode)                 ||" << endl;
    send_str << "||close(int fd)                              ||" << endl;
    send_str << "||read(int fd, int length)                   ||" << endl;
    send_str << "||write(int fd, char *buffer, int length)    ||" << endl;
    send_str << "||seek(int fd, int position, int ptrname)    ||" << endl;
    send_str << "||mkfile(char *name, int mode)               ||" << endl;
    send_str << "||rm(char *name)                             ||" << endl;
    send_str << "||ls()                                       ||" << endl;
    send_str << "||mkdir(char* dirname)                       ||" << endl;
    send_str << "||cd(char* dirname)                          ||" << endl;
    send_str << "||cat(char* dirname)                         ||" << endl;
    send_str << "||q/Q 退出文件系统                            ||" << endl << endl << endl;
	return send_str;
}
class sendU{
private:
    int fd;
    string username;
public:
    int send_(const stringstream& send_str){
        cout<<send_str.str()<<endl;
        int numbytes=send(fd,send_str.str().c_str(),sizeof(send_str.str()),0); 
        cout<< "["<< username<<"] send numbytes "<<numbytes<<endl;       
        return numbytes;
    };
    sendU(int fd,string username){
        this->fd=fd;
        this->username=username;
    };
};

void *start_routine( void *ptr) 
{
    int fd = *(int *)ptr;
    char buf[1024];
    int numbytes;
    numbytes=send(fd,"请输入用户名",sizeof("请输入用户名"),0); 
    cout << "[info] send函数返回值："  << numbytes << endl;
    //int i,c=0;
    printf("进入用户线程，fd=%d\n", fd);
    memset(buf, 0, sizeof(buf));
    if ((numbytes=recv(fd,buf,1024,0)) == -1){ 
        cout<<("recv() error\n"); 
        exit(1); 
    }
    string username=buf;
    cout << "[info] 用户输入用户名："  << username << endl;
    
    sendU sd(fd,username);
    sd.send_(print_head());
    string tipswords="||SecondFileSystem@"+username+"请输入函数名及参数$";

    // 初始化用户User结构和目录
    SecondFileKernel::Instance().GetUserManager().Login(username);

    while(true){
        char buf_recv[1024];
        numbytes=send(fd,tipswords.c_str(),sizeof(tipswords),0); 
        if(numbytes<=0){
            cout<<"[info] 用户 "<<username<<" 断开连接."<<endl;
            SecondFileKernel::Instance().GetUserManager().Logout();
            exit(1);
        }
        // 读取用户输入的命令行
        if ((numbytes=recv(fd,buf_recv,1024,0)) == -1){ 
            cout<<"recv() error"<<endl;
            SecondFileKernel::Instance().GetUserManager().Logout();
            exit(1);
        } 
        //解析命令名称
        stringstream ss(buf_recv);
        string api;
        ss>>api;
        stringstream send_str;
        cout<<"api"<<api<<endl;
        if(api == "cd"){
            string param1;
            ss >> param1;
            if(param1 == ""){
                send_str<< "cd [fpath]";
                send_str << "参数个数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            // 调用
            User &u=SecondFileKernel::Instance().GetUser();
			u.u_error= NOERROR;
			char dirname[300]={0};
            strcpy(dirname,param1.c_str());
            u.u_dirp=dirname;
            u.u_arg[0]=(unsigned long long)(dirname);
	        FileManager &fimanag = SecondFileKernel::Instance().GetFileManager();
	        fimanag.ChDir();
            // 打印结果
            send_str << "[result]:\n" << "now dir=" << dirname << endl;
            sd.send_(send_str);
            continue;
        }
        
        if(api == "ls"){
			User &u=SecondFileKernel::Instance().GetUser();
			u.u_error=NOERROR;
			string cur_path=u.u_curdir;
			FD fd = SecondFileKernel::Instance().Sys_Open(cur_path,(File::FREAD));
            send_str <<"fd:" <<fd<< "cur_path:" << cur_path << endl;
            char buf[33]={0};
			while(1){
				if(SecondFileKernel::Instance().Sys_Read(fd, 32, 33, buf)==0)
					break;
				else{
                    send_str << "cur_path:" << cur_path << endl << "buf:" << buf;
					DirectoryEntry *mm=(DirectoryEntry*)buf;
					// m_ino是啥时候赋值的？？
					if(mm->m_ino==0)
						continue;
					send_str << "======" << mm->m_name << "======" << endl;
					memset(buf, 0, 32);
				}
			}
			SecondFileKernel::Instance().Sys_Close(fd);
            sd.send_(send_str);
            continue;
        }

        if(api == "mkdir"){
            string path;
            ss >> path;
            if(path == ""){
                send_str << "mkdir [dirpath]";
                send_str << "参数个数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            int ret = SecondFileKernel::Instance().Sys_CreatDir(path);
            send_str<<"mkdir success (ret=" << ret << ")" <<endl;
            sd.send_(send_str);
            continue;
        }
        
        if(api == "mkfile"){
            string filename;
            ss >> filename;
            if(filename == ""){
                send_str << "mkfile [filepath]";
                send_str << "参数个数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            User &u =SecondFileKernel::Instance().GetUser();
			u.u_error = NOERROR;
			u.u_ar0[0] = 0;
            u.u_ar0[1] = 0;
            char filename_char[512];
			strcpy(filename_char,filename.c_str());
            u.u_dirp=filename_char;
			u.u_arg[1] = Inode::IRWXU;
			FileManager &fimanag=SecondFileKernel::Instance().GetFileManager();
			fimanag.Creat();
            send_str<<"mkfile sucess"<<endl;
            sd.send_(send_str);
            continue;
        }     
        if(api == "rm"){
            string filename;
            ss >> filename;
            if(filename == ""){
                send_str << "rm [filepath]";
                send_str << "参数个数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            User &u =SecondFileKernel::Instance().GetUser();
			u.u_error = NOERROR;
			u.u_ar0[0] = 0;
            u.u_ar0[1] = 0;
            char filename_char[512];
			strcpy(filename_char,filename.c_str());
            u.u_dirp=filename_char;
			FileManager &fimanag=SecondFileKernel::Instance().GetFileManager();
			fimanag.UnLink();
            send_str<<"rm success"<<endl;
            sd.send_(send_str);
            continue;
        }
        if(api == "seek"){
            string fd,position,ptrname;
            ss >> fd>>position>>ptrname;
            if(fd == ""||position==""||ptrname==""){
                send_str << "seek [fd] [position] [ptrname]";
                send_str << "参数个数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            if(!isNumber(fd)){
                send_str << "[fd] 参数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            int fd_int = atoi(fd.c_str());
            if(!isNumber(position)){
                send_str << "[position] 参数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            int position_int = atoi(position.c_str());
			if(!isNumber(ptrname)){
                send_str << "[ptrname] 参数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            int ptrname_int = atoi(ptrname.c_str());
            User &u =SecondFileKernel::Instance().GetUser();
			u.u_error = NOERROR;
			u.u_ar0[0] = 0;
            u.u_ar0[1] = 0;
			u.u_arg[0]=fd_int;
			u.u_arg[1]=position_int;
			u.u_arg[2]=ptrname_int;
			FileManager &fimanag=SecondFileKernel::Instance().GetFileManager();
			fimanag.Seek();
			send_str<<"[Results:]\n"<<"u.u_ar0="<<u.u_ar0<<endl;
            sd.send_(send_str);
            continue;
        }
        if (api == "open"){
            // FD SecondFileKernel::Sys_Open(std::string& fpath,int mode)
            string param1;
            string param2;
            ss >> param1 >> param2;
            if(param1 == "" || param2 == ""){
                send_str << "open [fpath] [mode]\n";
                send_str << "参数个数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            string fpath = param1;
            if(!isNumber(param2)){
                send_str << "[mode] 参数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            int mode = atoi(param2.c_str());

            // 调用
            FD fd = SecondFileKernel::Instance().Sys_Open(fpath, mode);
            // 打印结果
            send_str << "[ 结 果 ]:\n" << "fd=" << fd << endl;
            sd.send_(send_str);
            continue;
        }
        if (api == "read")
        {
            string p1_fd;
            string p2_size;
            ss >> p1_fd >> p2_size;
            if(p1_fd == "" || p2_size == ""){
                send_str << "read [fd] [size]\n";
                send_str << "参数个数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            if(!isNumber(p1_fd)){
                send_str << "[fd] 参数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            if(!isNumber(p2_size)){
                send_str << "[size] 参数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            int fd = atoi(p1_fd.c_str());
            if(fd < 0){
                send_str << "[fd] 应当为正整数" << endl;
                sd.send_(send_str);
                continue;
            }
            int size = atoi(p2_size.c_str());
            if(size <= 0 || size > 1024){
                send_str << "[size] size 的取值范围是(0,1024]." << endl;
                sd.send_(send_str);
                continue;
            }
            // 调用 API
            char buf[1025];
            memset(buf, 0, sizeof(buf));
            int ret = SecondFileKernel::Instance().Sys_Read(fd, size, 1025, buf);
            // 结果返回
            send_str << "[ 结 果 ]:\n"
                 << "ret=" << ret << endl 
                 << buf << endl;
            sd.send_(send_str);
            continue;
        }
        if (api == "write")
        {
            string p1_fd = "";
            string p2_content = "";
            ss >> p1_fd >> p2_content;
            if (p1_fd == "") {
                send_str << "write [fd] [content]\n";
                send_str << "参数个数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            if (!isNumber(p1_fd)){
                send_str << "[fd] 参数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            int fd = atoi(p1_fd.c_str());
            if(fd < 0){
                send_str << "[fd] 应当为正整数" << endl;
                sd.send_(send_str);
                continue;
            }
            if(p2_content.length() > 1024){
                send_str << "[content] 内容过长（不超过1024字节）" << endl;
                sd.send_(send_str);
                continue;
            }
            char buf[1025];
            memset(buf, 0, sizeof(buf));
            strcpy(buf, p2_content.c_str());
            int size = p2_content.length();
            // 调用 API
            int ret = SecondFileKernel::Instance().Sys_Write(fd, size, 1024, buf);
            // 打印结果
            send_str << "[ 结 果 ]\n" << "ret=" << ret << endl;
            sd.send_(send_str);
            continue;
        }
        if (api == "close")
        {
            string p1_fd;
            ss >> p1_fd;
            if(p1_fd == ""){
                send_str << "close [fd]\n";
                send_str << "参数个数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            if(!isNumber(p1_fd)){
                send_str <<  "[fd] 参数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            int fd = atoi(p1_fd.c_str());
            if(fd < 0){
                send_str << "[fd] fd应当为正整数" << endl;
                sd.send_(send_str);
                continue;
            }
            // 调用 API
            int ret = SecondFileKernel::Instance().Sys_Close(fd);
            send_str << "[ 结 果 ]\n" << "ret=" << ret << endl;
            sd.send_(send_str);
            continue;
        }
        if (api == "cat"){
            string p1_fpath;
            ss >> p1_fpath;
            if(p1_fpath == "")
            {
                send_str << "cat [fpath]\n";
                send_str << "参数个数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            string fpath = p1_fpath;
            // Open
            FD fd = SecondFileKernel::Instance().Sys_Open(fpath, 0x1);
            if(fd < 0){
                send_str << "[cat] 打开文件出错." << endl;
                sd.send_(send_str);
                continue;
            }
            // Read
            char buf[257];
            while(true){
                memset(buf, 0, sizeof(buf));
                int ret = SecondFileKernel::Instance().Sys_Read(fd, 256, 256, buf);
                if(ret <= 0){
                    break;
                }
                send_str << buf;
            }
            // Close
            SecondFileKernel::Instance().Sys_Close(fd);
            sd.send_(send_str);
            continue;
        }
        if (api == "copyin"){
            string p1_ofpath;
            string p2_ifpath;
            ss >> p1_ofpath >> p2_ifpath;
            if(p1_ofpath == "" || p2_ifpath == ""){
                send_str << "copyin ofpath ifpath\n";
                send_str << "参数个数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            // 打开外部文件
            int ofd = open(p1_ofpath.c_str(), O_RDONLY); //只读方式打开外部文件
            if(ofd < 0){
                send_str << "[ERROR] 打开文件失败：" << p1_ofpath << endl;
                sd.send_(send_str);
                continue;
            }
            // 创建内部文件
            SecondFileKernel::Instance().Sys_Creat(p2_ifpath, 0x1|0x2);
            int ifd = SecondFileKernel::Instance().Sys_Open(p2_ifpath, 0x1|0x2);
            if(ifd < 0){
                close(ofd);
                send_str << "[ERROR] 打开文件失败：" << p2_ifpath << endl;
                sd.send_(send_str);
                continue;
            }
            // 开始拷贝，一次 256 字节
            char buf[256];
            int all_read_num = 0;
            int all_write_num = 0;
            while(true){
                memset(buf, 0, sizeof(buf));
                int read_num = read(ofd, buf, 256);
                if(read_num <= 0){
                    break;
                }
                all_read_num += read_num;
                int write_num = \
                    SecondFileKernel::Instance().Sys_Write(ifd, read_num, 256, buf);
                if(write_num <= 0){
                    send_str << "[ERROR] 写入文件失败：" << p2_ifpath;
                    break;
                }
                all_write_num += write_num;
            }
            send_str << "共读取字节：" << all_read_num 
                 << " 共写入字节：" << all_write_num << endl;
            close(ofd);
            SecondFileKernel::Instance().Sys_Close(ifd);
            sd.send_(send_str);
            continue;
        }
        if (api == "copyout"){
            string p1_ifpath;
            string p2_ofpath;
            ss >> p1_ifpath >> p2_ofpath;
            if (p1_ifpath == "" || p2_ofpath == "")
            {
                send_str << "copyout [ifpath] [ofpath]\n";
                send_str << "参数个数错误" << endl;
                sd.send_(send_str);
                continue;
            }
            // 打开外部文件
            int ofd = open(p2_ofpath.c_str(), O_WRONLY| O_TRUNC); //截断写入方式打开外部文件
            if (ofd < 0)
            {
                send_str << "[ERROR] 创建文件失败：" << p2_ofpath << endl;
                sd.send_(send_str);
                continue;
            }
            // 创建内部文件
            SecondFileKernel::Instance().Sys_Creat(p1_ifpath, 0x1|0x2);
            int ifd = SecondFileKernel::Instance().Sys_Open(p1_ifpath, 0x1 | 0x2);
            if (ifd < 0)
            {
                close(ofd);
                send_str << "[ERROR] 打开文件失败：" << p1_ifpath << endl;
                sd.send_(send_str);
                continue;
            }
            // 开始拷贝，一次 256 字节
            char buf[256];
            int all_read_num = 0;
            int all_write_num = 0;
            while (true)
            {
                memset(buf, 0, sizeof(buf));
                int read_num = \
                    SecondFileKernel::Instance().Sys_Read(ifd, 256, 256, buf);
                if (read_num <= 0)
                {
                    break;
                }
                all_read_num += read_num;
                int write_num = write(ofd, buf, read_num);
                if (write_num <= 0)
                {
                    send_str << "[ERROR] 写入文件失败：" << p1_ifpath;
                    break;
                }
                all_write_num += write_num;
            }
            send_str << "共读取字节：" << all_read_num
                 << " 共写入字节：" << all_write_num << endl;
            close(ofd);
            SecondFileKernel::Instance().Sys_Close(ifd);
            sd.send_(send_str);
            continue;
        }
        if (api == "q" || api == "quit"){
            SecondFileKernel::Instance().GetUserManager().Logout();
            send_str << "用户登出\n";
            sd.send_(send_str);
            break;
        }
    }

    close(fd);
}


int main()
{ 
    int listenfd, connectfd;    
    struct sockaddr_in server;
    struct sockaddr_in client;      
    int sin_size; 
    sin_size=sizeof(struct sockaddr_in); 

    // 创建监听fd
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {   
        perror("Creating socket failed.");
        exit(1);
    }

    int opt = SO_REUSEADDR;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //使得端口释放后立马被复用

    bzero(&server,sizeof(server));  

    server.sin_family=AF_INET; 
    server.sin_port=htons(PORT); 
    server.sin_addr.s_addr = htonl (INADDR_ANY); 

    // 绑定
    if (bind(listenfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) { 
    perror("Bind error.");
    exit(1); 
    }   

    // 监听 
    if(listen(listenfd,BACKLOG) == -1){  /* calls listen() */ 
    perror("listen() error\n"); 
    exit(1); 
    }

    // 初始化文件系统（除了User部分）
    SecondFileKernel::Instance().Initialize();
    
    cout << "[info] 等待用户接入..." << endl;
    // 进入通信循环
    while(1){
        // accept 
        if ((connectfd = accept(listenfd,(struct sockaddr *)&client, (socklen_t*)&sin_size))==-1) {
            perror("accept() error\n"); 
            exit(1);
        }
        printf("客户端接入：%s\n",inet_ntoa(client.sin_addr) );
        string str="hello";
        send(connectfd,str.c_str(),6,0);
        pthread_t thread; //定义一个线程号
        pthread_create(&thread,NULL,start_routine,(void *)&connectfd);
    }
    close(listenfd);
    return 0;
}


