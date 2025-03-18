#pragma once
#include<queue>
#include"Task.h"
#include"Log.h"
#include"Http_conn.h"
#include<thread>
#include<mutex>
#include<condition_variable>
//class Task;
template<class T>
class ThreadPool
{
private:
	std::queue<T> task_queue;
	std::vector<std::thread> threads;
	std::mutex mtx;
	std::condition_variable cv;
	int num;
	bool stop;
private:
	void work() {
		while (true) {
			T task;
			{
				std::unique_lock<std::mutex> lock(this->mtx);
				this->cv.wait(lock, [this] {return !task_queue.empty() or stop; });
				if (stop and task_queue.empty()) return;
				task = task_queue.front();
				task_queue.pop();
			}
			//LOG_DEBUG("success push");
			task.handle();
		}
		return;
	}
public:
	ThreadPool() :num(4), stop(false) {
		for (int i = 0; i < num; i++) {
			threads.push_back(std::thread([this](){
				this->work();
				}));
		}
	}
	~ThreadPool() {
		{
			std::unique_lock<std::mutex> lock(mtx);
			stop = true;
		}
		cv.notify_all();
		for (int i = 0; i < num; i++) {
			threads[i].join();
		}
	}
public:
	bool push(T &&task) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			if (stop) {
				LOG_ERROR("push error");
				return false;
			}
			task_queue.push(task);
		}
		cv.notify_one();
		return true;
	}
};
