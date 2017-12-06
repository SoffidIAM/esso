/*
 * ThreadPool.h
 *
 *  Created on: 11 nov. 2017
 *      Author: gbuades
 */

#ifndef SRC_MAIN_NATIVE_LISTENERPOOL_H_
#define SRC_MAIN_NATIVE_LISTENERPOOL_H_

#include <string>
#include "ChromeElement.h"
#include "ChromeWebApplication.h"

namespace mazinger_chrome {

class ActiveListenerInfo;
class ListenerChain ;
class ChromeWebApplication;

class ListenerPool {
public:
	ListenerPool();
	virtual ~ListenerPool();
	ActiveListenerInfo* get(const std::string& eventId);
	void remove (const std::string &eventId);
	void removeApp (ChromeWebApplication *app);
	void removeByListener (ChromeElement* element,
		const char* event, WebListener* listener);
	void removeByApp (ChromeWebApplication *app,
		const char* event, WebListener* listener);
	void add (const std::string &eventId, ActiveListenerInfo *listener);


private:
#ifdef WIN32
	HANDLE hMutex;
#else
	sem_t semaphore;
#endif
	ListenerChain *first;

	bool enterCriticalSection();
	void exitCriticalSection();
	void deleteChainLink(ListenerChain* t);
};

} /* namespace mazinger_chrome */

#endif /* SRC_MAIN_NATIVE_LISTENERPOOL_H_ */
