#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>

#define MAXBUF 1024

//三个角度剖析
//1. 支持的最大连接数：与FD_SETSIZE宏的大小有关，一般为1024
//2. 内核态到用户态的拷贝消耗：非常高，每次select调用都会重新copy一次
//3. 内核态扫描的数据结构：线性扫描，FD剧增后会造成很大的效率问题

#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while (0)

int main(){
    auto sockfd = socket(AF_INET,SOCK_STREAM,0 );
    if(sockfd<0)
        ERR_EXIT("socket error");
    //设置监听地址
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(2000);
    addr.sin_addr.s_addr = INADDR_ANY;
    //绑定服务端描述符
    if( bind(sockfd,(sockaddr*)&addr,sizeof(addr))<0)
        ERR_EXIT("bind error");
    if(listen(sockfd,5)<0)
        ERR_EXIT("listen error");

    int fds[5];
    sockaddr_in client{};
    int max = 0;
    for (int & fd : fds) {
        memset(&client,0,sizeof(client));
        auto addrlen = sizeof(client);
        fd = accept(sockfd, (sockaddr*)&client, reinterpret_cast<socklen_t *>(&addrlen));
        if(fd>max)
            max = std::max(fd,max);
    }

    //1.定义bitmap结构。  fd_set，是一个bitmap，大小为1024位，是一个长度为1024/32的int数组
    fd_set rset;
    char buffer[MAXBUF];
    while (1) {
        FD_ZERO(&rset); //重置为0
        for (int i = 0; i < 5; ++i) {
            FD_SET(fds[i],&rset);   //2.根据文件描述符标记bitmap
        }
        puts("round again\n");
        select(max+1,&rset,NULL,NULL,NULL); //3.调用select进行轮询

        for(auto &fd:fds){
            if(FD_ISSET(fd,&rset)){ //4.获取已经准备好的描述符进行相应操作
                memset(buffer, 0, MAXBUF);
                read(fd, buffer, MAXBUF);
                puts(buffer);
            }
        }
    }
}