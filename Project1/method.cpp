#include "method.h"
#include <fcntl.h>
#include<sys/epoll.h>
bool setnoblockfd(int sockfd) {
	int oldsockflag = fcntl(sockfd, F_GETFL, 0);
	int newsockflag = oldsockflag | O_NONBLOCK;
	if (fcntl(sockfd, F_SETFL, newsockflag) == -1)return false;
	return true;
}