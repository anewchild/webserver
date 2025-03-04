#pragma once
#include<sys/epoll.h>
#define MAX_EVENT_NUMBER 10000
class WebServer
{
private:
	bool addfd(int epollfd, int sockfd, bool one_shoot = false);
public:
	bool init();
	bool listen_loop();
private:
	int listenfd;
	int m_epollfd;
	epoll_event events[MAX_EVENT_NUMBER];
};

