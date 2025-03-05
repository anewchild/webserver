#pragma once
class Task
{
private:
	int epollfd, sockfd;
public:
	Task() {
		epollfd = sockfd = -1;
	}
	Task(int epollfd, int sockfd) :epollfd(epollfd), sockfd(sockfd) {}
	~Task() {

	}
	void handle();
};

