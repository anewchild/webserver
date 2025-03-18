#pragma once
#include "WebServer.h"
#include "method.h"
#include<string>
#include<cstring>
#include<map>
#include<sys/stat.h>
#include<sys/uio.h>
#include<unistd.h>
enum METHOD{
	GET = 0,
	POST
};
enum STATUS {
	REQUEST_LINE = 0,
	REQUEST_HEAD,
	REQUEST_TEXT
};
enum HTTP_STATUS {
	FILE_REQUEST,
	BAD_REQUEST,
	NO_RESOURCE,
	INTERNAL_ERROR,
	INIT_REQUEST
};//http响应状态码
#define MAXBUFFER 2048

class Http_conn
{
private:
	bool f_read();
	bool read_line();
	bool read_request_line();//读请求行
	bool read_request_head();
	bool read_request_content();
	bool deal_with_head();// 处理请求头
	bool get_file();//读取文件资源
	void get_text();//处理响应
	void add_connection() {
		send_content += "Connection:";
		if (long_alive) {
			send_content += "keep-alive\r\n";
		}
		else {
			send_content += "close\r\n";
		}
	}
	void add_blank() {
		send_content += "\r\n";
	}
	void add_content(const std::string&);
	void add_content_length(long);
public:
	bool modfd(int ev) {
		epoll_event event;
		event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
		event.data.fd = sockfd;
		if (epoll_ctl(m_epollfd, EPOLL_CTL_MOD, sockfd, &event) == -1) {
			return false;
		}
		return true;
	}
	bool addfd(int ev) {
		epoll_event event;
		event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
		event.data.fd = sockfd;
		if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, sockfd, &event) == -1) {
			return false;
		}
		if (!setnoblockfd(sockfd))return false;
		return true;
	}
	void erasefd() {
		epoll_ctl(m_epollfd, EPOLL_CTL_DEL, sockfd, NULL);
		close(sockfd);
	}
	bool http_read();
	bool http_process();
	bool http_send();
	void set(int epollfd,int acceptfd,char *cwd_string) {
		m_epollfd = epollfd;
		sockfd = acceptfd;
		cwd_s = std::string(cwd_string);
		init();
	};
	void init() {
		status = REQUEST_LINE;
		http_status = INIT_REQUEST;
		request_head.clear();
		send_content.clear();
		read_text.clear();
		m_host.clear();
		long_alive = false;
		content_length = 0;
		line_start = line_end = 0;
		check_idx = read_idx = 0;
		send_size = have_send_size = 0;
		memset(m_buffer, '\0', sizeof(m_buffer));
	};
	Http_conn(int _sockfd,char *cwd_string):sockfd(_sockfd){
		cwd_s = std::string(cwd_string);
	};
	~Http_conn() {};
	Http_conn() {};
private:
	METHOD m_method;
	STATUS status;
	HTTP_STATUS http_status;
	std::string s_httpd;// http 方法版本
	std::string url;
	std::map<std::string,std::string> request_head;//请求头里的属性
	std::string read_text;
	std::string m_host;
	std::string request_content;
	std::string send_content;
	std::string cwd_s;
	char* file_p;
	bool long_alive;
	long content_length;
	int m_epollfd;
	int sockfd;
	/*每行的开始位置和结束位置（/n）*/
	int line_start;
	int line_end;
	int check_idx;
	int read_idx;
	//Client_Data* client;
	char m_buffer[MAXBUFFER];
	struct stat m_stat;
	iovec m_iovec[2];
	int num_iov;
	long send_size;
	long have_send_size;
};

