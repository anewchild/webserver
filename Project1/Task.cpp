#include "Task.h"
#include "Http_conn.h"
#include <stdio.h>
#include<errno.h>
#include<memory>
#include<unistd.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include <string.h>
void Task::handle() {
	char buff[1024];
	getcwd(buff, sizeof(buff));
	Http_conn http_conn(this->sockfd,buff);
	http_conn.http_read();
	//char buf[1024];
	//while (true) {
	//	memset(buf, '\0', sizeof(buf));
	//	ssize_t ret = recv(sockfd, buf, sizeof(buf), 0);
	//	if (ret == -1) {
	//		if (errno == EWOULDBLOCK) {
	//			printf("read finish\n");
	//			//epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
	//			//close(sockfd);
	//			break;
	//		}
	//		else if (errno == EINTR) {
	//			printf("I m here\n");
	//			continue;
	//		}
	//		printf("recv error\n");
	//		break;
	//	}
	//	else if (ret == 0) {
	//		//printf("close socket\n");
	//		//epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
	//		//close(sockfd);
	//		break;
	//	}
	//	printf("%s", buf);
	//}
	return;
}