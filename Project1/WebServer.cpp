#include "WebServer.h"
#include "method.h"
#include "Task.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <cstring>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

//#include<sys/epoll.h>
bool WebServer::init() {
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) {
		printf("listen error\n");
		return false;
	}
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8010);
	//inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//设置端口复用
	if (bind(listenfd, (sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("bind error\n");
		return false;
	}
	if (listen(listenfd, 10) == -1) {
		printf("listen error\n");
		return false;
	}
	m_epollfd = epoll_create(5);
	if (m_epollfd == -1) {
		printf("epoll_create error\n");
		return false;
	}
	pool_pts = new ThreadPool();
	return true;
}

bool WebServer::listen_loop() {
	if (!addfd(m_epollfd, listenfd))return false;
	while (true) {
		int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, 1000);
		if (number == -1) {
			if (errno == EINTR) {
				continue;
			}
			else {
				printf("epoll_wait error\n");
				return false;
			}
		}
		else if (number == 0) {
			continue;
		}
		for (size_t i = 0; i < (size_t)number; i++) {
			int sockfd = events[i].data.fd;
			if (sockfd == listenfd) {
				//有新的连接
				sockaddr_in client_addr;
				socklen_t addrlen = sizeof(client_addr);
				int acceptfd = accept(listenfd, (sockaddr*)&client_addr, &addrlen);
				if (acceptfd == -1) {
					printf("accept error\n");
					continue;                                                                                                                                     
				}
				if (!addfd(m_epollfd, acceptfd)) {
					printf("accept error %s\n", errno);
					close(acceptfd);
					continue;
				}
			}
			else {
				if (events[i].events & EPOLLIN) {
					pool_pts->push(std::move(Task(m_epollfd,sockfd)));
					//printf("read sucess\n");
				}
				else if (events[i].events & EPOLLRDHUP) {
					printf("error!\n");
					epoll_ctl(m_epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
					close(events[i].data.fd);
					continue;
				}
				else {
					printf("fill\n");
				}
			}
		}
	}
	return true;
}



bool WebServer::addfd(int epollfd, int sockfd, bool one_shoot) {
	epoll_event event;
	event.data.fd = sockfd;
	event.events = EPOLLET | EPOLLRDHUP | EPOLLIN;
	if (one_shoot)event.events |= EPOLLONESHOT;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event) == -1) {
		return false;
	}
	if (!setnoblockfd(sockfd))return false;
	return true;
}
WebServer::WebServer() :listenfd(-1), m_epollfd(-1) {
}
WebServer::~WebServer() {
	if(listenfd != -1)close(listenfd);
	if(m_epollfd != -1)close(m_epollfd);
	if (pool_pts != nullptr)delete pool_pts;
}