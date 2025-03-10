#pragma once
#include<sys/epoll.h>
#include<memory>
#include"ThreadPool.h"
#include <netinet/in.h>
#define MAX_EVENT_NUMBER 10000
#define MAXFD 65536
void fsig_handler(int sig);
class Client_Data;
class Timer_List;
void cb_func(Client_Data* client);
class WebServer
{
private:
	bool addfd(int epollfd, int sockfd, bool one_shoot = false);
	void addsig(int sig, void(handler)(int),bool restart);
	bool dealwithsig();
	void addclient();
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
	void (*sig_handler)(int);
	epoll_event events[MAX_EVENT_NUMBER];
	Client_Data* user;
	ThreadPool* pool_pts;
	Timer_List* timer_list;
	static WebServer* instance;
	bool timer_checkout;
};
//#ifndef INIT_WEB_SERVER
//#define INIT_WEB_SERVER
//WebServer* WebServer::instance = new WebServer();
//#endif


class util_timer;
/* 定时器*/
class Client_Data {
public:
	sockaddr_in addr;
	int sockfd;
	util_timer* timer;
};
class util_timer {
public:
	time_t expire;//超时时间
	void(*cb_func)(Client_Data*);//回调函数
	Client_Data* user_data;
	util_timer* prev;
	util_timer* next;
	util_timer() : prev(nullptr), next(nullptr) {}
};
class Timer_List {
private:
	util_timer* head;
	util_timer* tail;
public:
	Timer_List(){
		head = new util_timer();
		tail = new util_timer();
		head->next = tail;
		tail->prev = head;
	}
    ~Timer_List() {
        util_timer* tmp = head;
        while (head != nullptr) {
            tmp = head;
            head = head->next;
            delete tmp;
        }
    }
	void add_timer(util_timer* timer);
	void adjust_timer(util_timer* timer);
	void erase_timer(util_timer* timer);
	void tick();
};
