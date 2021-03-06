/*
 * ThreadStatus.cpp
 *
 *  Created on: 15/12/2014
 *      Author: gbuades
 */

#include "ThreadStatus.h"
#include <stdio.h>
#include <MazingerInternal.h>
#include "json/JsonValue.h"

namespace mazinger_chrome {

ThreadStatus::ThreadStatus() {
#ifdef WIN32
	hMutex = CreateEvent (NULL, FALSE, FALSE, NULL);
	hEventMutex = CreateEvent (NULL, FALSE, FALSE, NULL);
#else
	sem_init(&semaphore, false, 0);
	sem_init(&eventSemaphore, false, 0);
#endif
	readObject = NULL;
	pageData = NULL;
	end = false;

}

PendingEventList::PendingEventList() {
#ifdef WIN32
	hMutex = CreateMutex (NULL, FALSE, NULL);
#else
	sem_init(&semaphore, false, 1);
#endif
}


ThreadStatus::~ThreadStatus() {
#ifdef WIN32
	CloseHandle (hMutex);
	CloseHandle (hEventMutex);
#else
	sem_close(&semaphore);
	sem_close(&eventSemaphore);
#endif

	if (pageData != NULL)
		delete pageData;
}

PendingEventList::~PendingEventList() {
#ifdef WIN32
	CloseHandle (hMutex);
#else
	sem_close(&semaphore);
#endif

}

void PendingEventList::push (Event *event)
{
#ifdef WIN32
	DWORD dwResult = WaitForSingleObject (hMutex, INFINITE); // Forever
	if (dwResult == WAIT_OBJECT_0)
#else
	if ( sem_wait (&semaphore) == 0)
#endif
	{
		list.push_back(event);
#ifdef WIN32
		ReleaseMutex (hMutex);
#else
		sem_post(&semaphore);
#endif
	}
}

Event* PendingEventList::pop ()
{
	Event * result = NULL;
#ifdef WIN32
	DWORD dwResult = WaitForSingleObject (hMutex, INFINITE); // Forever
	if (dwResult == WAIT_OBJECT_0)
#else
	if ( sem_wait (&semaphore) == 0)
#endif
	{
		if (!list.empty())
		{
			result = list.front();
			list.pop_front();
		}
#ifdef WIN32
		ReleaseMutex (hMutex);
#else
		sem_post(&semaphore);
#endif
	}
	return result;
}

static json::JsonValue NULL_MESSAGE;

json::JsonAbstractObject* ThreadStatus::waitForMessage() {
	while (true)
	{

		bool acquired = false;
	#ifdef WIN32
		DWORD dwResult = WaitForSingleObject (hMutex, 10000); // 10 seconds wait
		acquired = (dwResult == WAIT_OBJECT_0);
	#else
		struct timespec timeout;
		time (&timeout.tv_sec);
		timeout.tv_nsec = 0;
		timeout.tv_sec += 10;
		acquired = ( sem_timedwait (&semaphore, &timeout) == 0);
	#endif
		if (acquired)
		{
			json::JsonAbstractObject *result = readObject;
			readObject = NULL;
			if (result == NULL)
			{
				// Loop again
			}
			else if (result == &NULL_MESSAGE)
			{
				return NULL;
			}
			else
			{
				return result;
			}
		}
		else
		{
			return NULL;
		}
	}
}

void ThreadStatus::notifyMessage(json::JsonAbstractObject* message) {
	if (readObject != NULL)
	{
		std::string s;
		readObject->write(s, 3);
		json::JsonAbstractObject* oldMessage = readObject;
		readObject = NULL;
		delete oldMessage;
	}
	readObject = message;
	if (message == NULL)
		readObject = &NULL_MESSAGE;
#ifdef WIN32
	SetEvent (hMutex);
#else
	sem_post(&semaphore);
#endif
}

void ThreadStatus::notifyEventMessage() {
#ifdef WIN32
	SetEvent (hEventMutex);
#else
	sem_post(&eventSemaphore);
#endif
}

Event* ThreadStatus::waitForEvent() {
	bool acquired = false;

	// Search pending events
	Event *ev  = pendingEvents.pop();
	if (ev != NULL)
		return ev;


	// Wait for a new event
	#ifdef WIN32
	DWORD dwResult = WaitForSingleObject (hEventMutex, 10000); // 5 seconds wait
	acquired = (dwResult == WAIT_OBJECT_0);
#else
	struct timespec timeout;
	time (&timeout.tv_sec);
	timeout.tv_nsec = 0;
	timeout.tv_sec += 10;
	acquired = ( sem_timedwait (&eventSemaphore, &timeout) == 0);
#endif
	if (acquired)
	{
//		MZNSendDebugMessageA("Got event message signal");
		fflush(stderr);
		Event *ev  = pendingEvents.pop();
		return ev;
	}
	else
	{
		return NULL;
	}
}

} /* namespace mazinger_chrome */

