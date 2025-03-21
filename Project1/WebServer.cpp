#include "WebServer.h"
int pipefd[2];
int client_num = 0;
//#include<sys/epoll.h>
bool WebServer::init() {
	getcwd(cwd_str, sizeof(cwd_str));//获取工作目录
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) {
		LOG_ERROR("listen error");
		return false;
	}
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8010);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//设置端口复用
	if (bind(listenfd, (sockaddr*)&addr, sizeof(addr)) == -1) {
		LOG_ERROR("bind error");
		return false;
	}
	if (listen(listenfd, 10) == -1) {
		LOG_ERROR("listen error");
		return false;
	}
	m_epollfd = epoll_create(5);
	if (m_epollfd == -1) {
		LOG_ERROR("epoll_create error");
		return false;
	}
	pool_pts = new ThreadPool<Task>();
	user = new Client_Data[MAXFD];
	client_conn = new Http_conn[MAXFD];
	timer_list = new Timer_List();
	client_num = 0;
	Log::get()->init("log.txt");//日志初始化
	return true;
}

bool WebServer::listen_loop() {
	sig_handler = fsig_handler;
	if (!addfd(m_epollfd, listenfd))return false;
	int ret = socketpair(PF_UNIX, SOCK_STREAM, 0,pipefd);
	if (ret == -1)return false;
	setnoblockfd(pipefd[1]);
	addfd(m_epollfd, pipefd[0]);   
	addsig(SIGALRM, sig_handler, false);
	alarm(TIMEOUT);
	while (true) {
		int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, 1000);
		if (number == -1) {
			if (errno == EINTR) {
				continue;
			}
			else {
				LOG_ERROR("epoll_wait error");
				return false;
			}
		}
		else if (number == 0) {
			continue;
		}
		for (size_t i = 0; i < (size_t)number; i++) {
			int sockfd = events[i].data.fd;
			if (sockfd == listenfd) {
				addclient();
			}
			else if (sockfd == pipefd[0] and (events[i].events & EPOLLIN)) {
				timer_checkout = true;
				dealwithsig();
				continue;
			}
			else {
				if (events[i].events & EPOLLRDHUP) {
					eraseclient(sockfd);
				}
				else if (events[i].events & EPOLLIN) {
					adjustclient(sockfd);
					pool_pts->push(std::move(Task(m_epollfd,sockfd,0,client_conn + sockfd,NULL)));
				}
				else if (events[i].events & EPOLLOUT) {
					adjustclient(sockfd);
					std::promise<bool>*pro_alive = new std::promise<bool>();
					std::future<bool> future_alive = pro_alive->get_future();
					pool_pts->push(std::move(Task(m_epollfd, sockfd, 1,client_conn + sockfd,pro_alive)));
					if (!future_alive.get()) {
						//这个任务出错或者短链接
						eraseclient(sockfd);
					}
					delete pro_alive;
				}
				else {
					LOG_WARN("unknow error");
				}
			}
		}
		if (timer_checkout) {
			LOG_INFO("timeout");
			timer_list->tick();
			timer_checkout = false;
		}
		alarm(TIMEOUT);
	}
	return true;
}


WebServer* WebServer::instance = new WebServer();
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
WebServer::WebServer() :listenfd(-1), m_epollfd(-1),user(nullptr),timer_list(nullptr),client_conn(nullptr),pool_pts(nullptr){
}
WebServer::~WebServer() {
	if(listenfd != -1)close(listenfd);
	if(m_epollfd != -1)close(m_epollfd);
	if (pool_pts != nullptr)delete pool_pts;
	if (instance != nullptr)delete instance;
	if (user != nullptr)delete[] user;
	if (timer_list != nullptr)delete timer_list;
	if (client_conn != nullptr)delete[] client_conn;
	close(pipefd[0]);
	close(pipefd[1]);
}

void Timer_List::add_timer(util_timer* timer) {
	if (head->next == tail) {
		//没有一个定时器
		head->next = timer;
		tail->prev = timer;
		timer->next = tail;
		timer->prev = head;
		return;
	}
	util_timer* ptr = head->next;
	while (ptr != tail) {
		if (timer->expire < ptr->expire) {
			timer->prev = ptr->prev;
			timer->prev->next = timer;
			timer->next = ptr;
			ptr->prev = timer;
			return;
		}
		ptr = ptr->next;
	}
	tail->prev->next = timer;
	timer->prev = tail->prev;
	tail->prev = timer;
	timer->next = tail;
	return;
}

void Timer_List::erase_timer(util_timer* timer) {
	timer->prev->next = timer->next;
	timer->next->prev = timer->prev;
	delete timer;
}

void Timer_List::adjust_timer(util_timer* timer) {
	util_timer* ptr = timer->next;
	while (ptr != tail and timer->expire >= ptr->expire) {
		ptr->prev = timer->prev;
		timer->prev->next = ptr;
		timer->prev = ptr;
		timer->next = ptr->next;
		ptr->next->prev = timer;
		ptr->next = timer;
		ptr = timer->next;
	}
	return;
}

void fsig_handler(int sig) {
	int save_errno = errno;
	int msg = sig;
	send(pipefd[1], (char*)&msg, 1, 0);
	errno = save_errno;
}

void WebServer::addsig(int sig,void(handler)(int),bool restart) {
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = handler;
	if(restart)sa.sa_flags |= SA_RESTART;
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, NULL) != -1);
}

bool WebServer::dealwithsig() {
	long ret = 0;
	char buf[128];
	ret = recv(pipefd[0], buf, sizeof(buf), 0);
	if (ret == -1 or ret == 0) {
		LOG_ERROR("dealwitherror");
		return false;
	}
	else {
		for (int i = 0; i < ret; i++) {
			switch (buf[i])
			{
			case SIGALRM:
				LOG_INFO("dealwithtimerout");
				break;
			default:
				LOG_ERROR("unknow errno");
				break;
			}
		}
	}
	return true;
}
void cb_func(Client_Data* client) {
	int epollfd = WebServer::get_instance()->get_epollfd();
	client_num--;
	int ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, client->sockfd, NULL);
	close(client->sockfd);
}
void WebServer::addclient() {
	sockaddr_in tmp_addr;
	socklen_t addrlen = sizeof(tmp_addr);
	int acceptfd = accept(listenfd, (sockaddr*)&tmp_addr, &addrlen);
	if (acceptfd == -1) {
		LOG_ERROR("accept error");
		return;
	}
	client_conn[acceptfd].set(m_epollfd, acceptfd, cwd_str);
	if (!client_conn[acceptfd].addfd(EPOLLIN)) {
		close(acceptfd);
		return;
	}
	user[acceptfd].sockfd = acceptfd;
	user[acceptfd].addr = tmp_addr;
	user[acceptfd].timer = new util_timer();
	util_timer* ptr = user[acceptfd].timer;
	ptr->user_data = &user[acceptfd];
	time_t now = time(NULL);
	ptr->expire = now + 3LL * TIMEOUT;
	ptr->cb_func = cb_func;
	client_num++;
	timer_list->add_timer(ptr);
}
void Timer_List::tick() {
	util_timer* ptr = head->next;
	time_t cur = time(NULL);
	while (ptr != tail and ptr != nullptr) {
		if (ptr->expire > cur) {
			break;
		}
		ptr->cb_func(ptr->user_data);
		util_timer* tmp = ptr->next;
		erase_timer(ptr);
		ptr = tmp;
	}
	return;
}

void WebServer::eraseclient(int sockfd) {
	timer_list->erase_timer(user[sockfd].timer);
	client_conn[sockfd].erasefd();
}

void WebServer::adjustclient(int sockfd) {
	time_t cur = time(NULL);
	user[sockfd].timer->expire = cur + 3 * TIMEOUT;
	timer_list->adjust_timer(user[sockfd].timer);
}