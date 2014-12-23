/*
 * CommunicationManager.h
 *
 *  Created on: 12/12/2014
 *      Author: gbuades
 */

#ifndef COMMUNICATIONMANAGER_H_
#define COMMUNICATIONMANAGER_H_

#include <string>
#include "json/JsonAbstractObject.h"
#include "ThreadStatus.h"
#include <map>


namespace mazinger_chrome {

class CommunicationManager {
public:
	CommunicationManager();
	virtual ~CommunicationManager();

	static CommunicationManager *getInstance ();


	std::string nextRequestId ();
	json::JsonAbstractObject* getEventMessage ();
	json::JsonAbstractObject* call (bool &error, const char *messages[]);

	void mainLoop ();
	void threadLoop (ThreadStatus *threadStatus);

private:
	std::map<std::string, ThreadStatus*> threads;
	std::string readMessage ();
	void writeMessage (const std::string &msg);
	static CommunicationManager *instance;
	int requestId = 0;
	json::JsonAbstractObject *pendingMessage = NULL;
#ifdef WIN32
	HANDLE hMutex;
#else
	sem_t semaphore;
#endif
};

} /* namespace mazinger_chrome */

#endif /* COMMUNICATIONMANAGER_H_ */
