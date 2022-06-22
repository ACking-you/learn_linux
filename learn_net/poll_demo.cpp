#include <poll.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
// 关于水平触发与边缘触发
// 水平触发是只要读缓冲区有数据，就会一直触发可读信号，而边缘触发仅仅在空变为非空的时候通知一次，
//三个角度剖析
//1. 支持的最大连接数：非常大
//2. 内核态到用户态的拷贝消耗：非常高，每次poll调用都会重新copy一次，且只支持水平触发
//3. 内核态扫描的数据结构：线性扫描，FD剧增后会造成很大的效率问题

#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while (0)


int main(){
    auto server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0)
        ERR_EXIT("error in get socket");
    //用于绑定的地址信息
    sockaddr_in server_addr{};
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(2222);
    server_addr.sin_family = AF_INET;
    if(bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        ERR_EXIT("error in bind");
    if(listen(server_fd, 5) < 0)
        ERR_EXIT("error in listen");

    sockaddr_in client{};
    socklen_t socklen{};
    //得到5个pollfd
    pollfd pollfds[5];
    for(auto& pollfd: pollfds){
        pollfd.fd = accept(server_fd,(sockaddr*)&client,&socklen);
        if(pollfd.fd<0)
            ERR_EXIT("fd accept error");
        pollfd.events = POLLIN; //检测读取事件
    }

    //开始进行poll轮询
    while (1){
        puts("round again");
        if(poll(pollfds,5,5000)<0)  //通过把需要监听的fd拷贝到内核态，如果有事件可读，则设置revents
            ERR_EXIT("poll error");
        for(auto & pollfd:pollfds){
            if(pollfd.revents&POLLIN){
                pollfd.revents = 0; //重新设置
                char buffer[1024]{};
                int len;
                if((len= read(pollfd.fd,buffer,1024))<0)
                    ERR_EXIT("pollfd read error");
                if(write(pollfd.fd,buffer,len)<0){
                    ERR_EXIT("pollfd write error");
                }
            }
        }
    }
}