#include "Http_conn.h"
#include <fcntl.h>
#include <unistd.h>
#include<sys/mman.h>
#include<sys/stat.h>

const std::string ok_200_title = "OK";
const std::string error_400_title = "Bad Request";
const std::string error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const std::string error_403_title = "Forbidden";
const std::string error_403_form = "You do not have permission to get file form this server.\n";
const std::string error_404_title = "Not Found";
const std::string error_404_form = "The requested file was not found on this server.\n";
const std::string error_500_title = "Internal Error";
const std::string error_500_form = "There was an unusual problem serving the request file.\n";


bool Http_conn::f_read() {
	// ѭ����������
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
				//�����ˣ����Ϸ�
				return false;
			}
			else if (m_buffer[check_idx + 1] == '\n') {
				//��������
				line_end = check_idx;
				m_buffer[check_idx] = '\0';
				check_idx += 2;
				return true;
			}
			else {
				//��ʽ���Ϸ�
				return false;
			}
		}
	}
	if (check_idx == read_idx and status == REQUEST_TEXT) {
		// ��ζ�ŵ��ı�ĩ�ˣ��ҺϷ�
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
		// ����Ĭ��λ��
		url += "index.html";
	}
	s_httpd = tmp.substr(pos + 1, (int)tmp.size() - pos - 1);
	status = REQUEST_HEAD;
	return true;
}
bool Http_conn::read_request_head() {
	if (line_start == line_end) {
		// ��������
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
void Http_conn::add_content_length(long content_l) {
	send_content += "Content-Length:" + std::to_string(content_l) + "\r\n";
}
void Http_conn::add_content(const std::string& content_s) {
	send_content += content_s;
}
void Http_conn::get_text() {
	send_content += "HTTP/1.1 ";
	switch (http_status)
	{
	case FILE_REQUEST:
		send_content += "200 " + ok_200_title + "\r\n";
		add_connection();
		add_content_length(m_stat.st_size);
		add_blank();
		break;
	case BAD_REQUEST:
		send_content += "400 " + error_400_title + "\r\n";
		add_connection();
		add_content_length(error_400_form.size());
		add_blank();
		add_content(error_400_form);
		break;
	case NO_RESOURCE:
		send_content += "404 " + error_404_title + "\r\n";
		add_connection();
		add_content_length(error_404_form.size());
		add_blank();
		add_content(error_404_form);
		break;
	default:
		send_content += "500 " + error_500_title + "\r\n";
		add_connection();
		add_content_length(error_500_form.size());
		add_blank();
		add_content(error_500_form);
		break;
	}
	m_iovec[0].iov_base = (void *)send_content.c_str();
	m_iovec[0].iov_len = (size_t)send_content.size();
	send_size = send_content.size();
	num_iov = 1;
	if (http_status == FILE_REQUEST) {
		m_iovec[1].iov_base = (void*)file_p;
		m_iovec[1].iov_len = m_stat.st_size;
		num_iov = 2;
		send_size = m_stat.st_size + send_content.size();
	}
}
bool Http_conn::get_file() {
	url = cwd_s + url;
	if (stat(url.c_str(), &m_stat) < 0) {
		http_status = NO_RESOURCE;
		return false;
	}
	int file_fd = open(url.c_str(), O_RDONLY);
	if (file_fd <= 0) {
		http_status = NO_RESOURCE;
		return false;
	}
	file_p = (char*)mmap(0, m_stat.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
	close(file_fd);
	http_status = FILE_REQUEST;
	return true;
}
bool Http_conn::http_read() {
	if (!f_read()) {
		return false;
	}
	status = REQUEST_LINE;
	bool ret = true;
	while (check_idx < read_idx) {
		switch (status)
		{
		case REQUEST_LINE:
			read_line();
			ret = read_request_line();
			if (!ret) {
				http_status = BAD_REQUEST;
				return false;
			}
			break;
		case REQUEST_HEAD:
			read_line();
			ret = read_request_head();
			if (!ret) {
				http_status = BAD_REQUEST;
				return false;
			}
			break;
		case REQUEST_TEXT:
			ret = read_request_content();
			if (!ret) {
				http_status = BAD_REQUEST;
				return false;
			}
			break;
		default:
			http_status = INTERNAL_ERROR;
			return false;
			break;
		}
	}
	deal_with_head();
	if (!get_file()) {
		return false;
	}
	return true;
	//munmap(file_p, m_stat.st_size);
}
bool Http_conn::http_process() {
	bool ret = http_read();
	LOG_DEBUG("http_read finsh");
	//if (!ret) {
	//	return false;
	//}
	get_text();
	std::cout << send_content << "\n";
	if (!modfd(EPOLLOUT)) {
		std::cout << "OK?\n";
		return false;
	}
	return true;
}
bool Http_conn::http_send() {
	// false -> deltimer;
	long send_tmp = 0;
	while (true) {
		send_tmp = writev(sockfd, m_iovec, num_iov);
		if (send_tmp < 0) {
			// ���ͳ���
			if(num_iov > 1)munmap(file_p, m_stat.st_size);
			return false;
		}
		have_send_size += send_tmp;
		if (have_send_size >= (int)m_iovec[0].iov_len and num_iov > 1) {
			m_iovec[1].iov_base = file_p + have_send_size - send_content.size();
			m_iovec[1].iov_len = m_stat.st_size - have_send_size + send_content.size();
		}
		else {
			m_iovec[0].iov_base = (void *)(send_content.c_str() + have_send_size);
			m_iovec[0].iov_len = send_content.size() - have_send_size;
		}
		if (have_send_size >= send_size) {
			// �������
			if (num_iov > 1) {
				munmap(file_p, m_stat.st_size);
			}
			modfd(EPOLLIN);
			LOG_DEBUG("sucess !");
			if (long_alive) {
				init();
				//������
				return true;
			}
			else {
				return false;
			}
		}
	}
}