#pragma once
#include "WebServer.h"
#include<string>
#include<map>
#include<sys/stat.h>
#include<sys/uio.h>
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

};//http响应状态码
#define MAXBUFFER 2048
class Http_conn
{
//typedef std::string str;
private:
	bool f_read();
	bool read_line();
	bool read_request_line();//读请求行
	bool read_request_head();
	bool read_request_content();
	bool deal_with_head();// 处理请求头
	bool get_file();//读取文件资源
	bool get_text();//处理响应
public:
	bool http_read();
	//void process();
	Http_conn(int _sockfd,char *cwd_string):sockfd(_sockfd){
		cwd_s = std::string(cwd_string);
	};
	~Http_conn() {};
private:
	METHOD m_method;
	STATUS status;
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
	Client_Data* client;
	char m_buffer[MAXBUFFER];
	struct stat m_stat;
	iovec m_iovec[2];
};

