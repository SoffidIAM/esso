/*
 * ThreadPool.cpp
 *
 *  Created on: 11 nov. 2017
 *      Author: gbuades
 */

#include "ThreadPool.h"

namespace mazinger_chrome {


ThreadPool::ThreadPool() {
#ifdef WIN32
	hMutex = CreateMutexA(NULL, FALSE, NULL);
#else
	sem_init(&semaphore, true, 1);
#endif
	first = last = NULL;
}

ThreadPool::~ThreadPool() {
}



bool ThreadPool::enterCriticalSection () {
#ifdef WIN32
	if (WaitForSingleObject (hMutex, INFINITE) != WAIT_OBJECT_0)
		ExitProcess(0);
#else
	sem_wait(&semaphore);
#endif
}

void ThreadPool::exitCriticalSection () {
#ifdef WIN32
	ReleaseMutex (hMutex);
#else
	sem_post(&semaphore);
#endif

}
ThreadStatus* ThreadPool::get(const std::string& pageId) {
	enterCriticalSection();
	ThreadStatus *t;
	for ( t = first; t != NULL; t = t->next)
	{
		if (t->pageId == pageId)
			break;
	}
	exitCriticalSection();
	return t;
}

void ThreadPool::add (ThreadStatus *thread) {
	enterCriticalSection();
	thread -> next = first;
	first = thread;
	thread -> lock();
	exitCriticalSection();

}

void ThreadPool::remove(const std::string& pageId) {
	enterCriticalSection();
	ThreadStatus *t;
	ThreadStatus *prev = NULL, *next = NULL;
	for ( t = first; t != NULL; t = next)
	{
		if (t->pageId == pageId)
		{
			if (prev == NULL)
				first = t->next;
			else
				prev -> next = t->next;
			next = t->next;
			t -> release();
		}
		else
		{
			prev = t;
			next = t->next;
		}
	}
	exitCriticalSection();
}

} /* namespace mazinger_chrome */

