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
#include "PageData.h"
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

class Event
{
public:
	std::string data;
	std::string target;
	ActiveListenerInfo *listener;
};


class PendingEventList
{
public:
	PendingEventList ();
	~PendingEventList ();
	Event * pop ();
	void push (Event* event);

private:
#ifdef WIN32
	HANDLE hMutex;
#else
	sem_t semaphore;
#endif
	std::list<Event*> list;
};

class ThreadStatus: public LockableObject {
public:
	ThreadStatus();
protected:
	virtual ~ThreadStatus();

public:
#ifdef WIN32
	HANDLE hMutex;
	HANDLE hEventMutex;
#else
	sem_t semaphore;
	sem_t eventSemaphore;
#endif

	Event *waitForEvent ();
	json::JsonAbstractObject* waitForMessage ();
	void notifyEventMessage ();
	void notifyMessage (json::JsonAbstractObject* message);

	std::string pageId;
	std::string title;
	std::string url;
	PageData *pageData;
	bool end;
	bool refresh;
	PendingEventList pendingEvents;

	virtual std::string toString () { return std::string("ThreadStatus") ;};
private:

	json::JsonAbstractObject *readObject;
};

} /* namespace mazinger_chrome */

#endif /* THREADSTATUS_H_ */
