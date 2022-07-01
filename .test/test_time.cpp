#include <time.h>
#include <chrono>  
#include <iostream>   // std::cout
#include<string>

std::time_t getTimeStamp()
{
    std::chrono::time_point<std::chrono::system_clock,std::chrono::milliseconds> tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());//获取当前时间点
    std::time_t timestamp =  tp.time_since_epoch().count(); //计算距离1970-1-1,00:00的时间长度
    return timestamp;
}
void demo(const char *p){
    std::string s ="                                                             "; 
    std::cout<<p;
}
int main(){
    time_t timestamp;
    std::cout << sizeof(char*)<<" "<<time(&timestamp) << std::endl<<int(timestamp)<<std::endl;//秒级时间戳
    // unsigned int timestamp =  getTimeStamp();
    printf("pf %d\n",timestamp);
    std::cout<<INT32_MAX<<std::endl;
    //std::cout << getTimeStamp() << std::endl;//毫秒级时间戳
    std::string s="123456789qwertyuiopasdfghjklzxcvbnm";
    demo(s.c_str());
}
