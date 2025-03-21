#pragma once
#include "Http_conn.h"
#include <stdio.h>
#include<errno.h>
#include<memory>
#include<unistd.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include <string.h>
#include<future>
class Http_conn;
class Task
{
private:
	int epollfd,sockfd,jud;
	Http_conn* hptr;
	std::promise<bool>*keep_live;
public:
	Task() {
		epollfd = sockfd = -1;
	}
	Task(int epollfd, int sockfd, int jd, Http_conn* htp,std::promise<bool>*_pro) :epollfd(epollfd), sockfd(sockfd), jud(jd), hptr(htp){
		keep_live = _pro;
	};
	~Task() {

	}
	void handle();
};

