#include "Task.h"
#include <stdio.h>
#include<errno.h>
#include<memory>
#include<unistd.h>
#include<sys/epoll.h>
#include<sys/socket.h>
void Task::handle() {
	char buf[1024];
	while (true) {
		ssize_t ret = recv(sockfd, buf, sizeof(buf), 0);
		if (ret == -1) {
			if (errno == EWOULDBLOCK) {
				printf("read finish\n");
				break;
			}
			else if (errno == EINTR) {
				printf("I m here\n");
				continue;
			}
			printf("recv error\n");
			break;
		}
		else if (ret == 0) {
			printf("close socket\n");
			epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
			close(sockfd);
			break;
		}
	}
	return;
}