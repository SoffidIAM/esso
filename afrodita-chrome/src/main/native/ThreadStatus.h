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

namespace mazinger_chrome {

class ThreadStatus {
public:
	ThreadStatus();
	virtual ~ThreadStatus();

#ifdef WIN32
	HANDLE hMutex;
#else
	sem_t semaphore;
#endif

	json::JsonAbstractObject* waitForMessage ();
	void notifyMessage (json::JsonAbstractObject* message);

	std::string pageId;
	std::string title;
	std::string url;
	bool end;

private:

	json::JsonAbstractObject *readObject;
};

} /* namespace mazinger_chrome */

#endif /* THREADSTATUS_H_ */
