#include<sys/epoll.h>
#include<sys/socket.h>
#include<iostream>
#include <netinet/in.h>
#include<unistd.h>
using namespace std;

//三个角度剖析
//1. 支持的最大连接数：非常大
//2. 内核态到用户态的拷贝消耗：较小，由于在使用epoll_create的时候就创建好了整个内核里的骨架，
// 之后的epoll_ctl将骨架填充，而调用热点epoll_wait进行的copy内容则非常少了，且里面进一步用mmap进行内存映射，实现了内存共享
//3. 内部扫描的数据结构：红黑树+就绪队列，可以同时管理大量fd，每次扫描非线性，性能好

#define ERR_EXIT(m) \
    do{ \
        perror(m);\
        exit(EXIT_FAILURE); \
    }while(0)

#define ERR_LOG(m) \
    do{ \
        perror(m);\
    }while(0)

int main(){
    //服务端fd初始化
    auto server_fd = socket(AF_INET,SOCK_STREAM,0);
    if(server_fd<0)
        ERR_EXIT("server socket error");
    sockaddr_in server_addr{};
    server_addr.sin_port = htons(2222);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    //服务端socket初始化绑定
    if(bind(server_fd,(sockaddr*)&server_addr,sizeof(server_addr))<0){
        ERR_EXIT("bind error");
    }
    if(listen(server_fd,5)<0){
        ERR_EXIT("listen error");
    }

    //epoll_create主要是将内核态的数据结构创建出来并初始化，然后再将它加入进程文件表，得到文件描述符
    auto epfd = epoll_create(1); //Linux 2.6.8后，传入的参数只要大于0即可，在此之前，表示最大的文件描述符

    //接收5个客户端连接
    epoll_event evs[5];
    for(int i=0;i<5;i++){
        sockaddr_in client{};
        socklen_t socklen;
        evs[i].data.fd = accept(server_fd,(sockaddr*)&client,&socklen);
        evs[i].events = EPOLLIN; //需要读取的事件
        if(epoll_ctl(epfd,EPOLL_CTL_ADD,evs[i].data.fd,&evs[i])<0)  //添加或删除内核数据结构中指定的文件描述符对应的结点
            ERR_EXIT("epoll_ctl");
    }

    //进行epoll_wait等待事件的到来
    while(1){
        puts("round again");
        //查询就绪队列，并将已经ok的文件描述符，从左到右写入数组，返回写入的长度，返回-1表示轮询超时
        auto nfds = epoll_wait(epfd,evs,5,10000); //查询就绪队列，然后把就绪到位的文件描述符和对应的情况写入用户空间中的数组
        for(int i=0;i<nfds;i++){
            char buffer[1024]{};
            int len{};
            if((len = read(evs[i].data.fd,buffer,1024))<0){
                ERR_LOG("read error");
            }else{
                if(write(evs[i].data.fd,buffer,len)<0){
                    ERR_LOG("write error");
                }
            }
        }
    }
}
