/*
 * ThreadStatus.h
 *
 *  Created on: 15/12/2014
 *      Author: gbuades
 */

#ifndef THREADSTATUS_H_
#define THREADSTATUS_H_
#ifdef WIN32
	#include <windows.h>
#else
	#include <semaphore.h>
#endif

#include "json/JsonAbstractObject.h"

#include <map>

#include <WebListener.h>
#include "ChromeWebApplication.h"
#include "ChromeElement.h"
#include <list>

namespace mazinger_chrome {

class ChromeWebApplication;
class ChromeElement;

class ActiveListenerInfo
{
public:
	std::string event;
	WebListener *listener;
	ChromeElement *element;
	ChromeWebApplication *app;
};


class ThreadStatus {
public:
	ThreadStatus();
	virtual ~ThreadStatus();

#ifdef WIN32
	HANDLE hMutex;
	HANDLE hEventMutex;
#else
	sem_t semaphore;
	sem_t eventSemaphore;
#endif

	ActiveListenerInfo *waitForEvent ();
	json::JsonAbstractObject* waitForMessage ();
	void notifyEventMessage ();
	void notifyMessage (json::JsonAbstractObject* message);

	std::string pageId;
	std::string title;
	std::string url;
	bool end;
	bool refresh;
	std::list<ActiveListenerInfo*> pendingEvents;

private:

	json::JsonAbstractObject *readObject;
};

} /* namespace mazinger_chrome */

#endif /* THREADSTATUS_H_ */
