#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "epoll.hpp"
#include "httpServe.hpp"
#include "threadPool.hpp"

/*-
* 非阻塞版本的web server,主要利用epoll机制来实现多路IO复用.加上了线程池,这样以来可以实现更高
* 的性能.
*/

#define MAXEVENTNUM 100

void block_sigpipe(){

	sigset_t signal_mask;
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGPIPE);
	int rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
	if (rc != 0) {

		std::cout<<"block sigpipe error"<<std::endl;
	}
}

int main(int argc, char *argv[]){

	int listenfd = Open_listenfd(8000); // 8080号端口监听 
	epoll_event events[MAXEVENTNUM];
	sockaddr clnaddr;
	socklen_t clnlen = sizeof(clnaddr);

	block_sigpipe(); // 首先要将SIGPIPE消息阻塞掉 

	int epollfd = Epoll_create(1024); // 10基本上没有什么用处 
	addfd(epollfd, listenfd, false); // epollfd要监听listenfd上的可读事件 
	ThreadPool pools(20, 60000); // 10个线程,300个任务 
	HttpServe::setEpollfd(epollfd);
	HttpServe handle[2000];

	while (1){
		int eventnum = Epoll_wait(epollfd, events, MAXEVENTNUM, -1);
		for (int i = 0; i < eventnum; ++i) {
			int sockfd = events[i].data.fd;
			if (sockfd == listenfd) {  
				//有连接到来 
				std::cout<<"connection comes!"<<std::endl;
				while (1){

					int connfd = accept(listenfd, &clnaddr, &clnlen);
					if (connfd == -1) {
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { // 将连接已经建立完了 
							
							break;
						}
						std::cout<<"accept error"<<std::endl;
					}
					handle[connfd].init(connfd); // 初始化 
					addfd(epollfd, connfd, false); // 加入监听 
				}
			}
			else { // 有数据可读或者可写

				pools.append(boost::bind(&HttpServe::process, &handle[sockfd]));
			}
		}
	}
	return 0;
}

