#ifndef _MUTEX_HPP_
#define _MUTEX_HPP_

//参照muduo库的mutex写出的一个简易的Mutex类

#include <bits/stdc++.h>
#include <assert.h>
#include <pthread.h>

class MutexLock{

public:

	MutexLock(): holder_(0){

		pthread_mutex_init(&mutex_, NULL); // 初始化
	}
	~MutexLock(){

		assert(holder_ == 0);
		pthread_mutex_destroy(&mutex_); // 销毁锁
	}
	const bool isLockedByThisThread(){  // 测试锁是否被当前线程持有

		return holder_ == pthread_self();
	}
	const void assertLocked() {

		assert(isLockedByThisThread());
	}
	void lock(){

		pthread_mutex_lock(&mutex_);
		assignHolder(); // 指定拥有者
	}
	void unlock(){

		unassignHolder(); // 丢弃拥有者
		pthread_mutex_unlock(&mutex_);
	}
	pthread_mutex_t* getPthreadMutex(){

		return &mutex_;
	}

private:

	friend class Condition;

	class UnassignGuard{

	public:

		UnassignGuard(MutexLock& owner) : owner_(owner){

			owner_.unassignHolder();
		}
		~UnassignGuard(){

			owner_.assignHolder();
		}

	private:

		MutexLock& owner_;
	};

	void unassignHolder(){

		holder_ = 0;
	}
	void assignHolder(){

		holder_ = pthread_self();
	}
	pthread_mutex_t mutex_;
	pthread_t holder_;
};

class MutexLockGuard{

public:

	MutexLockGuard(MutexLock& mutex) : mutex_(mutex){
		mutex_.lock();
	}
	~MutexLockGuard(){

		mutex_.unlock();
	}

private:

	MutexLock& mutex_;
};

class Condition{

private:

	MutexLock& mutex_;
	pthread_cond_t pcond_;

public:

	Condition(MutexLock& mutex) : mutex_(mutex){

		pthread_cond_init(&pcond_, NULL);
	}
	~Condition(){

		pthread_cond_destroy(&pcond_); // 销毁条件变量
	}
	void wait(){

		MutexLock::UnassignGuard ug(mutex_);
		pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()); // 等待Mutex
	}
	void notify(){

		pthread_cond_signal(&pcond_); // 唤醒一个线程
	}
	void notifyAll(){

		pthread_cond_broadcast(&pcond_); // 唤醒多个线程
	}

};

#endif 
