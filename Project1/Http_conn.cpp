#include "Http_conn.h"
#include <fcntl.h>
#include <unistd.h>
#include<sys/mman.h>
#include<sys/stat.h>
bool Http_conn::f_read() {
	// 循环读入数据
	while (true) {
		int ret = recv(sockfd, m_buffer + read_idx, (size_t)(MAXBUFFER - read_idx), 0);
		if (ret <= 0) {
			if (errno == EWOULDBLOCK) {
				break;
			}
			else if (errno == EINTR) {
				continue;
			}
			return false;
			break;
		}
		read_idx += ret;
	}
	return true;
}
bool Http_conn::read_line() {
	line_start = check_idx;
	for (; check_idx < read_idx; check_idx++) {
		char c = m_buffer[check_idx];
		if (c == '\r') {
			if (check_idx + 1 == read_idx) {
				//读完了，不合法
				return false;
			}
			else if (m_buffer[check_idx + 1] == '\n') {
				//读到行了
				line_end = check_idx;
				m_buffer[check_idx] = '\0';
				check_idx += 2;
				return true;
			}
			else {
				//格式不合法
				return false;
			}
		}
	}
	if (check_idx == read_idx and status == REQUEST_TEXT) {
		// 意味着到文本末了，且合法
		line_end = read_idx;
		return true;
	}
	return false;
}
bool Http_conn::read_request_line() {
	std::string tmp(m_buffer + line_start, line_end - line_start);
	int pos = tmp.find(' ');
	int last = 0;
	if (pos == -1) {
		LOG_ERROR("read_head ");
		return false;
	}
	std::string str_tmp = tmp.substr(0, pos - last);
	if (str_tmp == "GET") {
		m_method = GET;
	}
	else if (str_tmp == "POST") {
		m_method = POST;
	}
	else {
		LOG_INFO("ERROR METHOD");
		return false;
	}
	last = pos;
	pos = tmp.find(' ', pos + 1);
	if (pos == -1) {
		LOG_ERROR("read_head error TWO");
		return false;
	}
	url = tmp.substr(last + 1, pos - last - 1);
	if (pos == -1) {
		LOG_ERROR("read_head error THR");
		return false;
	}
	if (url[0] != '/') {
		return false;
	}
	else if (url.back() == '/') {
		// 访问默认位置
		url += "index.html";
	}
	s_httpd = tmp.substr(pos + 1, (int)tmp.size() - pos - 1);
	status = REQUEST_HEAD;
	return true;
}
bool Http_conn::read_request_head() {
	if (line_start == line_end) {
		// 空行跳出
		status = REQUEST_TEXT;
		return true;
	}
	std::string tmp(m_buffer + line_start, line_end - line_start);
	int pos = tmp.find(':');
	if (pos == -1 or pos + 2 > tmp.size()){
		LOG_ERROR("read_request_head no found :");
		return false;
	}
	std::string key = tmp.substr(0, pos), value = tmp.substr(pos + 2, (int)tmp.size() - pos - 2);
	request_head[key] = value;
	return true;
}
bool Http_conn::deal_with_head() {
	for (auto& item : request_head) {
		if (item.first == "Connection" and item.second == "keep-alive") {
			long_alive = true;
		}
		if (item.first == "Host") {
			m_host = item.second;
		}
		if (item.first == "Content-length") {
			content_length = std::stol(item.second);
		}
	}
}
bool Http_conn::read_request_content() {
	if (m_method == POST and read_idx < check_idx + content_length)
		return false;
	request_content = std::string(m_buffer + check_idx, read_idx - check_idx);
	return true;
}
bool Http_conn::get_text() {
	send_content += "HTTP/1.1 200 OK\r\n";
	send_content += "Content_Length: " + std::to_string(m_stat.st_size) + "\r\n\r\n";
	m_iovec[0].iov_base = (void *)send_content.c_str();
	m_iovec[0].iov_len = (size_t)send_content.size();
	m_iovec[1].iov_base = (void*)file_p;
	m_iovec[1].iov_len = m_stat.st_size;
	writev(sockfd, m_iovec, 2);
}
bool Http_conn::get_file() {
	url = cwd_s + url;
	//std::cout << url << "\n";
	stat(url.c_str(), &m_stat);
	//std::cout << ((int)m_stat.st_size) << "\n";
	int file_fd = open(url.c_str(), O_RDONLY);
	file_p = (char*)mmap(0, m_stat.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
	//printf("%s\n", file_p);
}
bool Http_conn::http_read() {
	if (!f_read())return false;
	status = REQUEST_LINE;
	bool ret = true;
	while (check_idx < read_idx) {
		switch (status)
		{
		case REQUEST_LINE:
			read_line();
			ret = read_request_line();
			if (!ret)return false;
			break;
		case REQUEST_HEAD:
			read_line();
			ret = read_request_head();
			if (!ret)return false;
			break;
		case REQUEST_TEXT:
			read_request_content();
			break;
		default:
			break;
		}
	}
	deal_with_head();
	get_file();
	get_text();
	munmap(file_p, m_stat.st_size);
}