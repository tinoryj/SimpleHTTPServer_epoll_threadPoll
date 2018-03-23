#ifndef _THREADPOOL_HPP_
#define _THREADPOOL_HPP_

#include <list>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <utility>

#include "mutex.hpp"
#include "csapp.hpp"
#include "epoll.hpp"

class ThreadPool{

public:

	typedef boost::function<void()> Task; // 需要执行的任务 
	ThreadPool(int threadNum, int maxTaskNum);
	~ThreadPool(){};
	bool append(Task&&); // 往工作队列中添加任务 
	void run();
	static void* startThread(void* arg); // 任务线程 

private:

	bool isFull();
	Task take();
	size_t queueSize();
	int threadNum_; // 线程的数目
	int maxQueueSize_;
	std::list<Task> queue_; //任务队列 
	MutexLock mutex_;
	Condition notEmpty_;
	Condition notFull_;
};

#endif  