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
	sem_init(&semaphore, true, 0);
	sem_init(&eventSemaphore, true, 0);
#endif
	end = false;

}

ThreadStatus::~ThreadStatus() {
#ifdef WIN32
	CloseHandle (hMutex);
#else
	sem_close(&semaphore);
#endif

}

static json::JsonValue NULL_MESSAGE;

json::JsonAbstractObject* ThreadStatus::waitForMessage() {
	while (true)
	{

		bool acquired = false;
	#ifdef WIN32
		DWORD dwResult = WaitForSingleObject (hMutex, 5000); // 5 seconds wait
		acquired = (dwResult == WAIT_OBJECT_0);
//		MZNSendDebugMessage ("RESULT = %x", dwResult);
	#else
		struct timespec timeout;
		time (&timeout.tv_sec);
		timeout.tv_nsec = 0;
		timeout.tv_sec += 5;
		acquired = ( sem_timedwait (&semaphore, &timeout) == 0);
	#endif
		if (acquired)
		{
			fflush(stderr);
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
			MZNSendDebugMessage ("Cannot get semaphore");
			return NULL;
		}
	}
}

void ThreadStatus::notifyMessage(json::JsonAbstractObject* message) {
	std::string s;
	if (message == NULL)
		s = "NULL";
	else
		message->write(s, 3);
//	MZNSendDebugMessageA("Notifying message %s", s.c_str());
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

ActiveListenerInfo* ThreadStatus::waitForEvent() {
	bool acquired = false;
#ifdef WIN32
	DWORD dwResult = WaitForSingleObject (hEventMutex, 5000); // 5 seconds wait
	acquired = (dwResult == WAIT_OBJECT_0);
#else
	struct timespec timeout;
	time (&timeout.tv_sec);
	timeout.tv_nsec = 0;
	timeout.tv_sec += 5;
	acquired = ( sem_timedwait (&semaphore, &timeout) == 0);
#endif
	if (acquired)
	{
//		MZNSendDebugMessageA("Got event message signal");
		fflush(stderr);
		if (pendingEvents.empty())
		{
//			MZNSendDebugMessageA("No event message found");
			return NULL;
		}
		else
		{
			ActiveListenerInfo *listener = pendingEvents.back();
//			MZNSendDebugMessageA("Got event message");
			pendingEvents.pop_back();
			return listener;
		}
	}
	else
	{
		return NULL;
	}
}

} /* namespace mazinger_chrome */

