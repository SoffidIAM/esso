/*
 * ThreadPool.h
 *
 *  Created on: 11 nov. 2017
 *      Author: gbuades
 */

#ifndef SRC_MAIN_NATIVE_THREADPOOL_H_
#define SRC_MAIN_NATIVE_THREADPOOL_H_

#include "ThreadStatus.h"
#include <string>

namespace mazinger_chrome {


class ThreadPool {
public:
	ThreadPool();
	virtual ~ThreadPool();
	ThreadStatus* get(const std::string& pageId);
	void remove (const std::string &pageId);
	void add (ThreadStatus *thread);

private:
#ifdef WIN32
	HANDLE hMutex;
#else
	sem_t semaphore;
#endif
	ThreadStatus *first;
	ThreadStatus *last;

	bool enterCriticalSection();
	void exitCriticalSection();
};

} /* namespace mazinger_chrome */

#endif /* SRC_MAIN_NATIVE_THREADPOOL_H_ */
