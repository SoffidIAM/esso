/*
 * ThreadStatus.cpp
 *
 *  Created on: 15/12/2014
 *      Author: gbuades
 */

#include "ThreadStatus.h"


namespace mazinger_chrome {

ThreadStatus::ThreadStatus() {
#ifdef WIN32
	hMutex = CreateEvent (NULL, FALSE, FALSE, NULL);
#else
	sem_init(&semaphore, true, 0);
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

json::JsonAbstractObject* ThreadStatus::waitForMessage() {
	bool acquired = false;
#ifdef WIN32
	DWORD dwResult = WaitForSingleObject (hMutex, 5000); // 5 seconds wait
	acquired = (dwResult == WAIT_OBJECT_0);
#else
	struct timespec timeout;
	timeout.tv_nsec = 0;
	timeout.tv_sec = 5;
	acquired = ( sem_timedwait (&semaphore, &timeout) == 0);
#endif
	if (acquired)
	{
		json::JsonAbstractObject *result = readObject;
		readObject = NULL;
		return result;
	}
}

void ThreadStatus::notifyMessage(json::JsonAbstractObject* message) {
	readObject = message;
#ifdef WIN32
	SetEvent (hMutex);
#else
	sem_post(&semaphore);
#endif
}

} /* namespace mazinger_chrome */
