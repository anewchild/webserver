#pragma once
#include<sys/epoll.h>
#include<memory>
#include"ThreadPool.h"
#include <netinet/in.h>
#define MAX_EVENT_NUMBER 10000
class WebServer
{
private:
	bool addfd(int epollfd, int sockfd, bool one_shoot = false);
public:
	bool init();
	bool listen_loop();
	~WebServer();
	//WebServer();
	int get_epollfd() { return m_epollfd; }
	static WebServer* get_instance() {
		return instance;
	}
private:
	WebServer();
	int listenfd;
	int m_epollfd;
	epoll_event events[MAX_EVENT_NUMBER];
	ThreadPool* pool_pts;
	static WebServer* instance;
};
//#ifndef INIT_WEB_SERVER
//#define INIT_WEB_SERVER
//WebServer* WebServer::instance = new WebServer();
//#endif

/* ¶¨Ê±Æ÷*/
struct Client_Data {
	sockaddr_in addr;
	int sockfd;
};

