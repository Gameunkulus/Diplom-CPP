#pragma once 

#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <atomic>
#include <condition_variable>

class task_manager
{
public:
	task_manager( unsigned int thread_count );
	~task_manager();

	void wait();

	void push_task( const std::function<void()>& task );
	void push_tasks( const std::vector<std::function<void()>>& tasks );

private:
	void work();

	std::vector<std::thread> threads_;
	std::mutex mtx_;
	std::atomic_bool stop_ = false;
	std::atomic_int active_tasks_ = 0;
	std::condition_variable cv_;
	std::condition_variable cv_wait_;
	std::queue<std::function<void()>> tasks_;
};