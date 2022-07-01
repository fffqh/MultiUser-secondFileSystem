#include<iostream>
#include<sstream>
#include<string>
using namespace std;

int main()
{
    string str;
    getline(cin, str);
    stringstream ss;
    ss << str;
    string fpath;
    int mode=-1;
    ss >> fpath >> mode;
    cout << "ss good:" << ss.good() << endl;
    cout <<"fpath:" << fpath << endl;
    cout <<"mode:" << mode << endl;
    cout << "ss:" << ss.str() << endl;
    return 0;
}

