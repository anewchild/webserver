#pragma once
#include<queue>
#include"Task.h"
#include<thread>
#include<mutex>
#include<condition_variable>
class ThreadPool
{
private:
	std::queue<Task> task_queue;
	std::vector<std::thread> threads;
	std::mutex mtx;
	std::condition_variable cv;
	int num;
	bool stop;
private:
	void work() {
		while (true) {
			Task task;
			{
				std::unique_lock<std::mutex> lock(this->mtx);
				this->cv.wait(lock, [this] {return !task_queue.empty() or stop; });
				if (stop and task_queue.empty()) return;
				task = task_queue.front();
				task_queue.pop();
			}
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
	bool push(Task &&task) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			if (stop) {
				printf("push error\n");
				return false;
			}
			task_queue.push(task);
		}
		cv.notify_one();
		return true;
	}
};

