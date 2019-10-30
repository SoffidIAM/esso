/*
 * ListenerPool.cpp
 *
 *  Created on: 11 nov. 2017
 *      Author: gbuades
 */

#include "ListenerPool.h"

namespace mazinger_chrome {

class ListenerChain {
public:
	ListenerChain *next;
	std::string eventId;
	ActiveListenerInfo *listener;
};

ListenerPool::ListenerPool() {
#ifdef WIN32
	hMutex = CreateMutexA(NULL, FALSE, NULL);
#else
	sem_init(&semaphore, true, 1);
#endif
	first = NULL;
}

ListenerPool::~ListenerPool() {
}



bool ListenerPool::enterCriticalSection () {
#ifdef WIN32
	if (WaitForSingleObject (hMutex, INFINITE) != WAIT_OBJECT_0)
		ExitProcess(0);
#else
	sem_wait(&semaphore);
#endif
}

void ListenerPool::exitCriticalSection () {
#ifdef WIN32
	ReleaseMutex (hMutex);
#else
	sem_post(&semaphore);
#endif

}
ActiveListenerInfo* ListenerPool::get(const std::string& eventId) {
	enterCriticalSection();
	ListenerChain *t;
	for ( t = first; t != NULL; t = t->next)
	{
		if (t->eventId == eventId)
			break;
	}
	exitCriticalSection();
	if (t == NULL)
		return NULL;
	else
		return t->listener;;
}

void ListenerPool::add (const std::string &eventId, ActiveListenerInfo *listener)
{
	enterCriticalSection();
	ListenerChain* chain = new ListenerChain();
	chain -> next = first;
	chain -> eventId = eventId;
	chain -> listener = listener;
	first = chain;
	exitCriticalSection();

}

void ListenerPool::deleteChainLink(ListenerChain* t) {
	fprintf(stderr, "REMOVING listener %s\n", t->eventId.c_str());

	if (t->listener->element != NULL)
		t->listener->element->release();

	t->listener->listener->release();
	t->listener->app->release();
	delete t->listener;
	delete t;
}

void ListenerPool::remove(const std::string& eventId) {
	enterCriticalSection();
	ListenerChain *t;
	ListenerChain *prev = NULL, *next = NULL;
	for ( t = first; t != NULL; t = next)
	{
		if (t->eventId == eventId)
		{
			if (prev == NULL)
				first = t->next;
			else
				prev -> next = t->next;
			next = t->next;

			deleteChainLink(t);
		}
		else
		{
			prev = t;
			next = t->next;
		}
	}
	exitCriticalSection();
}

void ListenerPool::removeApp (ChromeWebApplication *app)
{
	ListenerChain *t;
	ListenerChain *prev = NULL, *next = NULL;
	enterCriticalSection();
	for ( t = first; t != NULL; t = next)
	{
		if (t->listener->app == app )
		{
			if (prev == NULL)
				first = t->next;
			else
				prev -> next = t->next;
			next = t->next;

			deleteChainLink(t);
		}
		else
		{
			prev = t;
			next = t->next;
		}
	}
	exitCriticalSection();
}

void ListenerPool::removeByListener (ChromeElement* element,
	const char* event, WebListener* listener)
{
	ListenerChain *t;
	ListenerChain *prev = NULL, *next = NULL;
	enterCriticalSection();
	for ( t = first; t != NULL; t = next)
	{
		if (t->listener->element != NULL && t->listener->element->equals (element) &&
				t->listener->event == event && t->listener->listener == listener)
		{
			if (prev == NULL)
				first = t->next;
			else
				prev -> next = t->next;
			next = t->next;

			deleteChainLink(t);
		}
		else
		{
			prev = t;
			next = t->next;
		}
	}
	exitCriticalSection();
}

void ListenerPool::removeByApp (ChromeWebApplication* app,
	const char* event, WebListener* listener)
{
	ListenerChain *t;
	ListenerChain *prev = NULL, *next = NULL;
	enterCriticalSection();
	for ( t = first; t != NULL; t = next)
	{
		if (t->listener->app != NULL && t->listener->app == app &&
				t->listener->event == event && t->listener->listener == listener)
		{
			if (prev == NULL)
				first = t->next;
			else
				prev -> next = t->next;
			next = t->next;

			deleteChainLink(t);
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

