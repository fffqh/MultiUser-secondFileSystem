#ifndef USERMG_H
#define USERMG_H
#define NOERROR 0
typedef int ErrorCode;
#include "FileManager.h"
#include "User.h"
#include "File.h"
#include "INode.h"
#include <iostream>
#include <string.h>
#include <map>
using namespace std;

class UserManager
{
public:
    static const int USER_N = 100; // 最多支持100个用户同时在线
    UserManager();
    ~UserManager();
    // 用户登录
    bool Login(string uname);
    // 用户登出
    bool Logout();
    // 得到当前线程的User结构
    User* GetUser();

public:
    // 一个动态的索引表
    std::map<pthread_t, int> user_addr;
    // 一个User数组
    User* pusers[USER_N]; 
};

#endif
