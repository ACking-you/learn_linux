#include <iostream>
#include<string>
#include<vector>
#include <sys/socket.h>
#include<thread>
#include<chrono>


int main(){
    for(auto i = 0;i<10;i++){
        std::cout<<socket(AF_INET,SOCK_STREAM,0)<<"  ";
    }
    std::cout.flush();
    std::this_thread::sleep_for(std::chrono::seconds(5));
}
