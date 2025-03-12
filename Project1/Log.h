#pragma once
#include<fstream>
#include<iostream>
#include<queue>
#include<string>
#include<mutex>
#include<condition_variable>
class Log
{
public:
	static Log* get() {
		static Log log;
		return &log;
	}
	bool init(const char* file_name, bool is_async = false) {
		async = is_async;
		file.open(file_name,std::ios::out | std::ios::app);
		return file.is_open();
	}
	void log_write(std::string msg, int level) {
		std::string output = format_message(msg, level);
		if (async) {
			return;
		}
		else {
			std::unique_lock<std::mutex>lck(mtx);
			if (file.is_open()) {
				file << output;
			}
		}
	}
	void flush() {
		std::unique_lock<std::mutex>lck(mtx);
		file.flush();
	}
private:
	bool async;
	int max_siz;
	std::queue<std::string>que;
	std::mutex mtx;
	std::condition_variable cv;
	std::ofstream file;
	std::string format_message(std::string& msg,int level) {
		std::string out;
		switch (level)
		{
		case 0:
			out = "[DEBUG]:";
			break;
		case 1:
			out = "[INFO]:";
			break;
		case 2:
			out = "[WARNING]:";
			break;
		case 3:
			out = "[ERROR]:";
			break;
		default:
			break;
		}
		time_t tp;
		struct tm* time_p;
		time(&tp);
		time_p = localtime(&tp);
		out = "(" + out + std::to_string(time_p->tm_year + 1900) + "_" + std::to_string(time_p->tm_mon) + "_" + std::to_string(time_p->tm_hour) + "_" + std::to_string(time_p->tm_min) + "_" + std::to_string(time_p->tm_sec) + "):";
		out += msg + "\n";
		return out;
	}
};
#define LOG_DEBUG(format) {Log::get()->log_write(format,0);Log::get()->flush();}
#define LOG_INFO(format) {Log::get()->log_write(format,1);Log::get()->flush();}
#define LOG_WARN(format) {Log::get()->log_write(format,2);Log::get()->flush();}
#define LOG_ERROR(format) {Log::get()->log_write(format,3);Log::get()->flush();}
