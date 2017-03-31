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

#include <WebListener.h>
#include "ChromeElement.h"
#include <SeyconServer.h>

namespace mazinger_chrome {

class ActiveListenerInfo;

class CommunicationManager {
public:
	CommunicationManager();
	virtual ~CommunicationManager();

	static CommunicationManager *getInstance ();


	std::string nextRequestId ();
	json::JsonAbstractObject* getEventMessage ();
	json::JsonAbstractObject* call (bool &error, const char *messages[]);
	json::JsonAbstractObject* call (bool &error, const std::string&  pageId, const std::string& msgId, const std::string message);
	json::JsonAbstractObject* call (bool &error, json::JsonMap *message);
	void sendEvent (const char* eventId, const char *pageId, const char *target, const char* data);

	void mainLoop ();
	void threadLoop (ThreadStatus *threadStatus);
	std::string registerListener (ChromeElement *element, const char *event, WebListener *listener);
	std::string unregisterListener (ChromeElement *element, const char *event, WebListener *listener);
	std::string registerListener (ChromeWebApplication *app, const char *event, WebListener *listener);
	std::string unregisterListener (ChromeWebApplication *app, const char *event, WebListener *listener);
	std::string unregisterListener (ChromeWebApplication *app, const char *eventId);

private:
	std::map<std::string, ThreadStatus*> threads;
	std::string readMessage ();
	void writeMessage (const std::string &msg);
	static CommunicationManager *instance;
	int requestId;
	json::JsonAbstractObject *pendingMessage;
#ifdef WIN32
	HANDLE hMutex;
#else
	sem_t semaphore;
#endif
	int nextListener;
	std::map<std::string,ActiveListenerInfo*> activeListeners;
};

} /* namespace mazinger_chrome */

#endif /* COMMUNICATIONMANAGER_H_ */
