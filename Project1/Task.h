#pragma once
#include "Http_conn.h"
#include <stdio.h>
#include<errno.h>
#include<memory>
#include<unistd.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include <string.h>
class Http_conn;
class Task
{
private:
	int epollfd,sockfd,jud;
	Http_conn* hptr;
public:
	Task() {
		epollfd = sockfd = -1;
	}
	Task(int epollfd, int sockfd, int jd, Http_conn* htp) :epollfd(epollfd), sockfd(sockfd), jud(jd), hptr(htp) {};
	~Task() {

	}
	void handle();
};

