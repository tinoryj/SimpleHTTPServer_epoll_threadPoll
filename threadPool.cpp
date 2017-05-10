#include "threadPool.hpp"

ThreadPool::ThreadPool(int threadNum, int maxQueueSize)
	: threadNum_(threadNum)
	, maxQueueSize_(maxQueueSize)
	, mutex_()
	, notEmpty_(mutex_)
	, notFull_(mutex_)
{
	assert(threadNum >= 1 && maxQueueSize >= 1);
	//构建threadNum个线程
	pthread_t tid_t;
	for (int i = 0; i < threadNum; i++) {

		Pthread_create(&tid_t, NULL, startThread, this);
	}
}

size_t ThreadPool::queueSize(){

	MutexLockGuard lock(mutex_);
	return queue_.size();
}

void* ThreadPool::startThread(void* obj){
	 //任务线程
	Pthread_detach(Pthread_self());
	ThreadPool* pool = static_cast<ThreadPool*>(obj);
	pool->run();
	return pool;
}

ThreadPool::Task ThreadPool::take(){

	MutexLockGuard lock(mutex_);
	while (queue_.empty()) {

		notEmpty_.wait();
	}
	Task task;
	if (!queue_.empty()) {

		task = queue_.front();
		queue_.pop_front();

		if (maxQueueSize_ > 0) {

			notFull_.notify();
		}
	}
	return task;
}
void ThreadPool::run(){

	while (1){
		
		Task task(take());
		if (task) {

			task();
		}
	}
}

bool ThreadPool::isFull(){

	mutex_.assertLocked();
	bool full = maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
	return full;
}

bool ThreadPool::append(Task&& task){
	 // 使用了右值引用
	{
		MutexLockGuard lock(mutex_); // 首先加锁
		while (isFull()) {

			notFull_.wait(); // 等待queue有空闲位置
		}
		assert(!isFull());
		queue_.push_back(std::move(task)); //删除任务
	}

	notEmpty_.notify(); // 通知任务队列中有任务可做
}
