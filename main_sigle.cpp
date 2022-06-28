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

using namespace std;
bool isNumber(const string& str)
{
    for (char const &c : str) {
        if (std::isdigit(c) == 0) return false;
    }
    return true;
}

void print_head(){
    cout << "===============================================================" << endl;
    cout << "||请在一行中依次输入需要调用的函数名称及其参数                ||" << endl;
    cout << "||open(char *name, int mode)                                 ||" << endl;
    cout << "||close(int fd)                                              ||" << endl;
    cout << "||read(int fd, int length)                                   ||" << endl;
    cout << "||write(int fd, char *buffer, int length)                    ||" << endl;
    cout << "||seek(int fd, int position, int ptrname)                    ||" << endl;
    cout << "||mkfile(char *name, int mode)                               ||" << endl;
    cout << "||rm(char *name)                                             ||" << endl;
    cout << "||ls()                                                       ||" << endl;
    cout << "||mkdir(char* dirname)                                       ||" << endl;
    cout << "||cd(char* dirname)                                          ||" << endl;
    cout << "||cat(char* dirname)                                         ||" << endl;
    cout << "||q/quit 退出文件系统                                        ||" << endl << endl << endl;
}
void print_line(){
    cout << "||SecondFileSystem@ 请输入函数名及参数$";
}

#include"FileSystem.h"
int main()
{
    // pthread_mutex_t t;
    // cout<<"pthread"<<sizeof(t)<<endl;
    // int tt;
    // cout<<"int"<<sizeof(tt)<<endl;
    // SuperBlock sbb;
    // cout<<"sb "<<sizeof(sbb)<<endl;
    // 初始化SecondFileKernel
    SecondFileKernel::Instance().Initialize();
    User &u = SecondFileKernel::Instance().GetUser();
    FileManager &fimanag = SecondFileKernel::Instance().GetFileManager();

    print_head();
    // 单用户控制台交互逻辑
    while(true){
        print_line();
        // 读取用户输入的命令行
        string user_cmd;

        getline(cin, user_cmd);

        //解析命令名称
        stringstream ss;
        ss << user_cmd;
        string api;
        
        ss >> api;

        if(api == "cd"){
            string param1;
            ss >> param1;
            if(param1 == ""){
                cout << "cd [fpath]";
                cout << "参数个数错误" << endl;
                continue;
            }
            // 调用
            User &u=SecondFileKernel::Instance().GetUser();
			u.u_error= NOERROR;
			char dirname[512]={0};
            //char * dirname = (char*)malloc(300);
            //if (dirname == NULL){
            //    cout << "指令调用失败，由于堆空间申请失败." <<endl;
            //    continue;
            //}
            //memset(dirname, 0, 300);
            strcpy(dirname,param1.c_str());
            u.u_dirp=dirname;
            // memcpy(&u.u_arg[0], dirname, sizeof(char*));
            u.u_arg[0]=(unsigned long long)dirname;
	        FileManager &fimanag = SecondFileKernel::Instance().GetFileManager();
	        fimanag.ChDir();
            // 打印结果
            cout << "[result]:\n" << "now dir=" << dirname << endl;
            continue;
        }
        
        if(api == "ls"){
			User &u=SecondFileKernel::Instance().GetUser();
			u.u_error=NOERROR;
			string cur_path=u.u_curdir;
			FD fd = SecondFileKernel::Instance().Sys_Open(cur_path,(File::FREAD));
            cout << "cur_path:" << cur_path << endl;
            char buf[33]={0};
			while(1){
				if(SecondFileKernel::Instance().Sys_Read(fd, 32, 33, buf)==0)
					break;
				else{
					DirectoryEntry *mm=(DirectoryEntry*)buf;
					// m_ino是啥时候赋值的？？
					if(mm->m_ino==0)
						continue;
					cout << "====== " << mm->m_name << " ======" << endl;
					memset(buf, 0, 32);
				}
			}
			SecondFileKernel::Instance().Sys_Close(fd);
            continue;
        }

        if(api == "mkdir"){
            string path;
            ss >> path;
            if(path == ""){
                cout << "mkdir [dirpath]";
                cout << "参数个数错误" << endl;
                continue;
            }
			int defaultmode=040755;
			User &u = SecondFileKernel::Instance().GetUser();
			u.u_error = NOERROR;
            char filename_char[512];
			strcpy(filename_char,path.c_str());
            u.u_dirp=filename_char;
			u.u_arg[1] = defaultmode;
			u.u_arg[2] = 0;
			FileManager &fimanag = SecondFileKernel::Instance().GetFileManager();
			fimanag.MkNod();
            continue;
        }
        
        if(api == "mkfile"){
            string filename;
            ss >> filename;
            if(filename == ""){
                cout << "mkfile [filepath]";
                cout << "参数个数错误" << endl;
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
            continue;
        }
        
        if(api == "rm"){
            string filename;
            ss >> filename;
            if(filename == ""){
                cout << "rm [filepath]";
                cout << "参数个数错误" << endl;
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
            continue;
        }
        if(api == "seek"){
            string fd,position,ptrname;
            ss >> fd>>position>>ptrname;
            if(fd == ""||position==""||ptrname==""){
                cout << "seek [fd] [position] [ptrname]";
                cout << "参数个数错误" << endl;
                continue;
            }
            if(!isNumber(fd)){
                cout << "[fd] 参数错误" << endl;
            }
            int fd_int = atoi(fd.c_str());
            if(!isNumber(position)){
                cout << "[position] 参数错误" << endl;
            }
            int position_int = atoi(position.c_str());
			if(!isNumber(ptrname)){
                cout << "[ptrname] 参数错误" << endl;
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
			cout<<"[Results:]\n"<<"u.u_ar0="<<u.u_ar0<<endl;
            continue;
        }
        if (api == "open"){
            // FD SecondFileKernel::Sys_Open(std::string& fpath,int mode)
            string param1;
            string param2;
            ss >> param1 >> param2;
            if(param1 == "" || param2 == ""){
                cout << "open [fpath] [mode]\n";
                cout << "参数个数错误" << endl;
                continue;
            }
            string fpath = param1;
            if(!isNumber(param2)){
                cout << "[mode] 参数错误" << endl;
                continue;
            }
            int mode = atoi(param2.c_str());

            // 调用
            FD fd = SecondFileKernel::Instance().Sys_Open(fpath, mode);
            // 打印结果
            cout << "[ 结 果 ]:\n" << "fd=" << fd << endl;
            continue;
        }
        if (api == "read")
        {
            string p1_fd;
            string p2_size;
            ss >> p1_fd >> p2_size;
            if(p1_fd == "" || p2_size == ""){
                cout << "read [fd] [size]\n";
                cout << "参数个数错误" << endl;
                continue;
            }
            if(!isNumber(p1_fd)){
                cout << "[fd] 参数错误" << endl;
                continue;
            }
            if(!isNumber(p2_size)){
                cout << "[size] 参数错误" << endl;
                continue;
            }
            int fd = atoi(p1_fd.c_str());
            if(fd < 0){
                cout << "[fd] 应当为正整数" << endl;
                continue;
            }
            int size = atoi(p2_size.c_str());
            if(size <= 0 || size > 1024){
                cout << "[size] size 的取值范围是(0,1024]." << endl;
                continue;
            }
            // 调用 API
            char buf[1025];
            memset(buf, 0, sizeof(buf));
            int ret = SecondFileKernel::Instance().Sys_Read(fd, size, 1025, buf);
            // 结果返回
            cout << "[ 结 果 ]:\n"
                 << "ret=" << ret << endl 
                 << buf << endl;
            continue;
        }
        if (api == "write")
        {
            string p1_fd = "";
            string p2_content = "";
            ss >> p1_fd >> p2_content;
            if (p1_fd == "") {
                cout << "write [fd] [content]\n";
                cout << "参数个数错误" << endl;
                continue;
            }
            if (!isNumber(p1_fd)){
                cout << "[fd] 参数错误" << endl;
                continue;
            }
            int fd = atoi(p1_fd.c_str());
            if(fd < 0){
                cout << "[fd] 应当为正整数" << endl;
                continue;
            }
            if(p2_content.length() > 1024){
                cout << "[content] 内容过长（不超过1024字节）" << endl;
                continue;
            }
            char buf[1025];
            memset(buf, 0, sizeof(buf));
            strcpy(buf, p2_content.c_str());
            int size = p2_content.length();
            // 调用 API
            int ret = SecondFileKernel::Instance().Sys_Write(fd, size, 1024, buf);
            // 打印结果
            cout << "[ 结 果 ]\n" << "ret=" << ret << endl;
            continue;
        }
        if (api == "close")
        {
            string p1_fd;
            ss >> p1_fd;
            if(p1_fd == ""){
                cout << "close [fd]\n";
                cout << "参数个数错误" << endl;
                continue;
            }
            if(!isNumber(p1_fd)){
                cout <<  "[fd] 参数错误" << endl;
                continue;
            }
            int fd = atoi(p1_fd.c_str());
            if(fd < 0){
                cout << "[fd] fd应当为正整数" << endl;
                continue;
            }
            // 调用 API
            int ret = SecondFileKernel::Instance().Sys_Close(fd);
            cout << "[ 结 果 ]\n" << "ret=" << ret << endl;
            continue;
        }
        if (api == "cat"){
            string p1_fpath;
            ss >> p1_fpath;
            if(p1_fpath == "")
            {
                cout << "cat [fpath]\n";
                cout << "参数个数错误" << endl;
                continue;
            }
            string fpath = p1_fpath;
            // Open
            FD fd = SecondFileKernel::Instance().Sys_Open(fpath, 0x1);
            if(fd < 0){
                cout << "[cat] 打开文件出错." << endl;
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
                cout << buf;
            }
            // Close
            SecondFileKernel::Instance().Sys_Close(fd);
            continue;
        }
        if (api == "copyin"){
            string p1_ofpath;
            string p2_ifpath;
            ss >> p1_ofpath >> p2_ifpath;
            if(p1_ofpath == "" || p2_ifpath == ""){
                cout << "copyin ofpath ifpath\n";
                cout << "参数个数错误" << endl;
                continue;
            }
            // 打开外部文件
            int ofd = open(p1_ofpath.c_str(), O_RDONLY); //只读方式打开外部文件
            if(ofd < 0){
                cout << "[ERROR] 打开文件失败：" << p1_ofpath << endl;
                continue;
            }
            // 创建内部文件
            SecondFileKernel::Instance().Sys_Creat(p2_ifpath, 0x1|0x2);
            int ifd = SecondFileKernel::Instance().Sys_Open(p2_ifpath, 0x1|0x2);
            if(ifd < 0){
                close(ofd);
                cout << "[ERROR] 打开文件失败：" << p2_ifpath << endl;
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
                    cout << "[ERROR] 写入文件失败：" << p2_ifpath;
                    break;
                }
                all_write_num += write_num;
            }
            cout << "共读取字节：" << all_read_num 
                 << " 共写入字节：" << all_write_num << endl;
            close(ofd);
            SecondFileKernel::Instance().Sys_Close(ifd);

            continue;
        }
        if (api == "copyout"){
            string p1_ifpath;
            string p2_ofpath;
            ss >> p1_ifpath >> p2_ofpath;
            if (p1_ifpath == "" || p2_ofpath == "")
            {
                cout << "copyout ifpath ofpath\n";
                cout << "参数个数错误" << endl;
                continue;
            }
            // 打开外部文件
            int ofd = open(p2_ofpath.c_str(), O_WRONLY| O_TRUNC | O_CREAT); //截断写入方式打开外部文件
            if (ofd < 0)
            {
                cout << "[ERROR] 创建文件失败：" << p2_ofpath << endl;
                continue;
            }
            // 打开内部文件
            int ifd = SecondFileKernel::Instance().Sys_Open(p1_ifpath, 0x1 | 0x2);
            if (ifd < 0)
            {
                close(ofd);
                cout << "[ERROR] 打开文件失败：" << p1_ifpath << endl;
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
                    cout << "[ERROR] 写入文件失败：" << p1_ifpath;
                    break;
                }
                all_write_num += write_num;
            }
            cout << "共读取字节：" << all_read_num
                 << " 共写入字节：" << all_write_num << endl;
            close(ofd);
            SecondFileKernel::Instance().Sys_Close(ifd);
            
            continue;
        }
        if (api == "q" || api == "quit"){
            SecondFileKernel::Instance().Quit();
            exit(0);
        }
        if (api != ""){
            cout << "[ERROR]命令错误." <<endl;
            print_head();
        }
    }
    return 0;
}

