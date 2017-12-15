/*
 * CommunicationManager.cpp
 *
 *  Created on: 12/12/2014
 *      Author: gbuades
 */

#include "AfroditaC.h"
#include "CommunicationManager.h"
#include <stdio.h>
#include "json/JsonMap.h"
#include "json/JsonValue.h"
#include "json/JsonVector.h"
#include <stdlib.h>
#include <string.h>
#include "ChromeWebApplication.h"
#include "ChromeElement.h"
#include "json/Encoder.h"
#include <exception>
#include "WebAddonHelper.h"

#ifndef WIN32
#include <pthread.h>
#endif

#include <MazingerInternal.h>

using namespace json;

// #undef DEBUG
// #define DEBUG(x) SeyconCommon::warn("%s",x)

namespace mazinger_chrome {

/**
 * Semaphore control
 */
#ifdef WIN32
HANDLE hMutex = NULL;
static HANDLE getMutex() {
	if (hMutex == NULL)
	{
		hMutex = CreateMutexA(NULL, FALSE, NULL);

	}
	return hMutex;
}


static bool waitMutex () {
	HANDLE hMutex = getMutex();
	if (hMutex != NULL)
	{
		DWORD dwResult = WaitForSingleObject (hMutex, INFINITE);
		if (dwResult == WAIT_OBJECT_0)
		{
			return true;
		}
	}
	return false;
}

static void endMutex () {
	HANDLE hMutex = getMutex();
	if (hMutex != NULL)
	{
		ReleaseMutex (hMutex);
	}
}

#else
static sem_t semaphore;
static bool sem_initialized= false;
sem_t* getSemaphore () {
	if (! sem_initialized)
		sem_init ( &semaphore, 0, 1);

	return &semaphore;
}

static bool waitMutex () {
	sem_t* s = getSemaphore();
	if (s != NULL && sem_wait(s) == 0)
			return true;
	return false;
}

static void endMutex () {
	sem_t* s = getSemaphore();
	if (s != NULL)
		sem_post(s);
}


#endif

CommunicationManager *CommunicationManager::instance = NULL;


static void dumpMessage(const char* action, JsonAbstractObject *json)
{
#if 0
	std::string s;
	json->write(s, 0);
	fprintf(stderr, "%s %s\n", action, s.c_str());
#endif
}

CommunicationManager::CommunicationManager() {
#ifdef WIN32
	hMutex = CreateMutexA(NULL, FALSE, NULL);
#else
	sem_init(&semaphore, true, 1);
#endif
	requestId = 0;
	pendingMessage = NULL;
	nextListener = 1;
}

CommunicationManager::~CommunicationManager() {
}

CommunicationManager* CommunicationManager::getInstance() {
	if (instance == NULL)
		instance = new CommunicationManager();
	return instance;
}

std::string CommunicationManager::readMessage() {
	unsigned int messageSize = 0;


	if (sizeof messageSize != 4)
	{
		return std::string();
	}

	std::string result;
	DEBUG ("Reading message header");

	if ( fread (&messageSize, 4, 1, stdin) == 1)
	{
		if (messageSize < 1 || messageSize > 128000)
		{
			SeyconCommon::warn("Protocol error. Received message with size %d",
					messageSize);
#ifdef WIN32
			ExitProcess(0);
#else
			exit(0);
#endif
		}
		char *buffer = (char*) malloc (messageSize+1);
		if (buffer != NULL)
		{
			if ( fread ( buffer, messageSize, 1, stdin) == 1)
			{
				buffer[messageSize] = '\0';

				result.assign (buffer);
//				MZNSendDebugMessage("Received %s", buffer);
				free (buffer);
			}
			else
			{
				DEBUG ("Error reading message");
				free (buffer);
			}
		}
		else
		{
			DEBUG ("Error allocating message");
		}
	}
	else
	{
		DEBUG ("ERror receiving message size");
	}

	return result;
}

json::JsonAbstractObject* CommunicationManager::getEventMessage() {
	std::string textMsg = readMessage();
	json::JsonAbstractObject *msg = NULL;

	DEBUG("MESSAGE");
	DEBUG(textMsg.c_str());

//	MZNSendDebugMessage("Received %s", textMsg.c_str());
	if (!textMsg.empty())
	{
		const char * strMsg = textMsg.c_str();
		msg = JsonAbstractObject::readObject(strMsg);
		dumpMessage("GOT MESSAGE", msg);
	}
	return msg;
}

void CommunicationManager::writeMessage(const std::string& msg) {
#ifdef WIN32
	if (WaitForSingleObject (hMutex, INFINITE) != WAIT_OBJECT_0)
		return;
#else
	sem_wait(&semaphore);
#endif
//	MZNSendDebugMessage("Send %s", msg.c_str());
	//	MZNSendDebugMessage("Sent %s", msg.c_str());
	unsigned int messageSize = msg.length();
	fwrite (&messageSize, 4, 1, stdout);
	fwrite (msg.c_str(), messageSize, 1, stdout);
	fflush(stdout);
#ifdef WIN32
	ReleaseMutex (hMutex);
#else
	sem_post(&semaphore);
#endif
}

std::string CommunicationManager::nextRequestId() {
	int id = ( requestId + 1 ) % 9999;
	requestId = id;
	char ach[10];
	sprintf (ach,"%d", id);
	return std::string(ach);
}


JsonAbstractObject* CommunicationManager::call(bool &error, const char*messages[]) {
	std::string pageId;
	if (pendingMessage != NULL)
	{
		error = true;
		return NULL;
	}
	std::string msgId = nextRequestId();
	std::string msg = "{ \"requestId\": \"";
	msg += msgId;
	msg += "\"";
	bool left = true;
	bool nextPageId = false;
	for (int i = 0; messages[i] != NULL; i++)
	{
		if (left)
		{
			msg += ", ";
			nextPageId =  (strcmp (messages[i], "pageId") == 0);
		}
		else
		{
			if (nextPageId) pageId = messages[i];
			msg += ": ";
		}
		left = ! left;
		msg += Encoder::encode(messages[i]);
	}
	msg += "}";

	return call (error, pageId, msgId, msg);
}

json::JsonAbstractObject* CommunicationManager::call (bool &error, json::JsonMap *message)
{
	std::string pageId;
	std::string msgId = nextRequestId();
	JsonValue* v = new JsonValue();
	v->value = msgId;
	message->setObject("requestId", v);

	JsonValue* p = dynamic_cast<JsonValue*>(message->getObject("pageId"));
	if (p != NULL)
		pageId = p->value;

	std::string msg;
	message->write(msg, 0);

	return call (error, pageId, msgId, msg);

}

JsonAbstractObject* CommunicationManager::call(bool &error, const std::string& pageId, const std::string & msgId, const std::string msg) {
	if (pendingMessage != NULL)
	{
		error = true;
		return NULL;
	}

	writeMessage(msg);



	ThreadStatus *ts = threadPool.get( pageId) ;

	if (ts == NULL)
	{
		error = true;
		return NULL;
	}

	ts->lock();
	JsonAbstractObject *jsonMsg = ts->waitForMessage();
	ts->release();
	if (jsonMsg == NULL)
	{
		MZNSendDebugMessage("ERROR : No response got for %s", msg.c_str());
		error = true;
		return NULL;
	}


//	std::string s;
//	jsonMsg->write(s, 3);
//	MZNSendDebugMessage("Message      %s", msg.c_str());
//	MZNSendDebugMessage("Got response %s", s.c_str());
	JsonMap  *map = dynamic_cast<JsonMap*> (jsonMsg);
	if (map != NULL)
	{
		JsonValue *v  = dynamic_cast<JsonValue*> ( map->getObject("requestId"));
		if ( v == NULL || v->value != msgId)
		{
			error = true;
			dumpMessage("Delete 1", jsonMsg);
			delete jsonMsg;
			return NULL;
		}
		v  = dynamic_cast<JsonValue*> ( map->getObject("error"));
		if ( v != NULL && v->value != "false")
		{
			error = true;
			JsonValue *ex = dynamic_cast<JsonValue*>(map->getObject("exception"));
			if (ex != NULL)
			{
				MZNSendDebugMessage("Error got from Chrome port: %s", ex->value.c_str());
			}
			dumpMessage("Delete 2", jsonMsg);
			delete jsonMsg;
			return NULL;
		}
		JsonAbstractObject *result = map->getObject("response");
		map->remove("response");
//		MZNSendDebugMessage("Respnse got");
		dumpMessage("Delete 3", jsonMsg);
		delete jsonMsg;
		error = false;
		return result;
	}
	else
	{
		error = true;
		dumpMessage("Delete 4", jsonMsg);
		delete jsonMsg;
		return NULL;
	}
}

#ifdef WIN32
static DWORD WINAPI win32ThreadProc(
  LPVOID arg
)
#else
static void* linuxThreadProc (void *arg)
#endif
{
	ThreadStatus *ts = (ThreadStatus*) arg;
//	MZNSendDebugMessage("Created THREAD [%s] %s", ts->pageId.c_str(), ts->url.c_str());
	CommunicationManager::getInstance()->threadLoop(ts);
//	MZNSendDebugMessage("Remvoed THREAD [%s] %s", ts->pageId.c_str(), ts->url.c_str());
	return NULL;
}

static PageData * parsePageData (ThreadStatus *status, JsonMap* map)
{
	if (map == NULL)
		return NULL;
	PageData *pd = new PageData();

	pd->loadJson(map);

	return pd;
}

void CommunicationManager::mainLoop() {
	do
	{
		DEBUG("Main loop");
		JsonAbstractObject *message = getEventMessage();
		if (message == NULL)
		{
//			fprintf(stderr, "End message\n");
#ifdef WIN32
			ExitProcess(0);
#else
			exit (0);
#endif
			return;
		}
		bool deleteMap = true;
		JsonMap *jsonMap = dynamic_cast<JsonMap*>(message);
		JsonValue* messageName = dynamic_cast<JsonValue*>(jsonMap->getObject("message"));
		JsonValue* pageId = dynamic_cast<JsonValue*>(jsonMap->getObject("pageId"));
		JsonMap *jsonPageData = dynamic_cast<JsonMap*>(jsonMap->getObject("pageData"));

//		std::string t;
//		jsonMap->write(t, 3);
//		MZNSendDebugMessage("RECEIVED MSG**********************");
//		MZNSendDebugMessage("Received %s", t.c_str());
		if (messageName != NULL && messageName->value == "onLoad" && pageId != NULL)
		{
			JsonValue* title = dynamic_cast<JsonValue*>(jsonMap->getObject("title"));
			JsonValue* url = dynamic_cast<JsonValue*>(jsonMap->getObject("url"));

			ThreadStatus *ts =  threadPool.get(pageId->value);

			if (ts != NULL && ! ts->end)
			{
				ts->refresh = true;
				if (ts->pageData != NULL)
					delete ts->pageData;
				ts->pageData = parsePageData(ts,jsonPageData);
				ts->notifyEventMessage();
			}
			else
			{
				if (ts != NULL) // Old thread status
				{
					ts->end = true;
					ts->notifyEventMessage();
					threadPool.remove(ts->pageId);
				}
				ts = new ThreadStatus();
				if (pageId != NULL)
				{
					ts->pageId = pageId->value;
					if (title != NULL)
						ts->title = title->value;
					if (url != NULL)
						ts->url = url->value;
					ts->pageData = parsePageData ( ts, jsonPageData );
					threadPool.add(ts);
#ifdef WIN32
					if ( CreateThread (NULL,  0, win32ThreadProc, ts,0, NULL) == NULL)
						ExitProcess(1);
#else
					pthread_attr_t att;
					pthread_t threadId;
					pthread_attr_init(& att);
					pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED);
					if (pthread_create(&threadId, &att, linuxThreadProc, ts) != 0)
						exit(1);
#endif
				}
			}
		}
		else if (messageName != NULL &&
				(messageName->value == "onUnload" || messageName->value == "onUnload2") &&
				pageId != NULL)
		{
//			MZNSendDebugMessageA("THREAD TO END");
			if (pageId != NULL)
			{
				ThreadStatus *ts = threadPool.get(pageId->value);
				if (ts != NULL)
				{
//					MZNSendDebugMessageA("THREAD TO END %s", ts->url.c_str());
					ts->end = true;
					ts->notifyEventMessage();
				}
			}
		}
		else if (messageName != NULL && messageName->value == "event" && pageId != NULL)
		{
			JsonValue* eventId = dynamic_cast<JsonValue*>(jsonMap->getObject("eventId"));
			JsonValue* target = dynamic_cast<JsonValue*>(jsonMap->getObject("target"));
			JsonValue* data = dynamic_cast<JsonValue*>(jsonMap->getObject("data"));

			if (pageId != NULL && eventId != NULL)
			{
				ThreadStatus *ts = threadPool.get(pageId->value);
				if (ts != NULL)
				{

					ActiveListenerInfo *ali = activeListeners.get(eventId->value);
					if (ali != NULL)
					{
						Event *ev = new Event();
						ev->target = (target == NULL ? "" : target->value.c_str());
						ev->data = (data == NULL ? "" : data->value.c_str());
						ev->listener = ali;
						ts->pendingEvents.push(ev);
						ts->notifyEventMessage();
					}
				}
			}
		}
		else if (messageName != NULL && messageName->value == "search" && pageId != NULL)
		{
			JsonValue* text = dynamic_cast<JsonValue*>(jsonMap->getObject("text"));
			std::string response = " {\"action\":\"searchResult\", \"pageId\":\""+pageId->value +"\",\"result\": [";
			if (text != NULL)
			{
				WebAddonHelper h;
				std::vector<UrlStruct> result;
				h.searchUrls (MZNC_utf8towstr(text->value.c_str()), result);
				bool first = true;
				for ( std::vector<UrlStruct>::iterator it = result.begin(); it != result.end (); it++)
				{
					UrlStruct s = *it;
					if (first) first = false;
					else response += ",";
					response += "{\"url\":";
					response += Encoder::encode (MZNC_wstrtoutf8(s.url.c_str()).c_str());
					response += ",\"name\":";
					response += Encoder::encode (MZNC_wstrtoutf8(s.description.c_str()).c_str());
					response += ",\"account\":";
					response += Encoder::encode (MZNC_wstrtoutf8(s.name.c_str()).c_str());
					response += ",\"system\":";
					response += Encoder::encode (MZNC_wstrtoutf8(s.server.c_str()).c_str());
					response += "}";
				}
			}
			response += "]}";
			writeMessage(response);
		}
		else if (messageName != NULL && messageName->value == "searchForServer" && pageId != NULL)
		{
			JsonValue* text = dynamic_cast<JsonValue*>(jsonMap->getObject("text"));
			std::string response = " {\"action\":\"searchForServerResult\", \"pageId\":\""+pageId->value +"\",\"result\": [";
			if (text != NULL)
			{
				WebAddonHelper h;
				std::vector<UrlStruct> result;
				h.searchUrlsForServer(MZNC_utf8towstr(text->value.c_str()), result);
				bool first = true;
				for ( std::vector<UrlStruct>::iterator it = result.begin(); it != result.end (); it++)
				{
					UrlStruct s = *it;
					if (first) first = false;
					else response += ",";
					response += "{\"url\":";
					response += Encoder::encode (MZNC_wstrtoutf8(s.url.c_str()).c_str());
					response += ",\"name\":";
					response += Encoder::encode (MZNC_wstrtoutf8(s.description.c_str()).c_str());
					response += ",\"account\":";
					response += Encoder::encode (MZNC_wstrtoutf8(s.name.c_str()).c_str());
					response += ",\"system\":";
					response += Encoder::encode (MZNC_wstrtoutf8(s.server.c_str()).c_str());
					response += "}";
				}
			}
			response += "]}";
			writeMessage(response);
		}
		else if (messageName != NULL && messageName->value == "info" && pageId != NULL)
		{
			std::string link;
			SeyconCommon::readProperty("AutoSSOURL", link);

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
			std::string response = "{\"action\": \"version\", \"version\":\"" STR(VERSION) "\", \"url\":";
			response += Encoder::encode(link.c_str());
			response += ",\"pageId\":\""+pageId->value +"\"}";
			writeMessage(response);
		}
		else if ( messageName != NULL && messageName->value == "response")
		{
			if ( pageId != NULL)
			{
				ThreadStatus *ts = threadPool.get(pageId->value);
				if (ts != NULL)
				{
					ts->notifyMessage(jsonMap);
					deleteMap = false;
				}
			}
		}
		if (deleteMap)
		{
			dumpMessage("Delete 5", message);
			delete message;
		}
	} while (true);
}

void CommunicationManager::threadLoop(ThreadStatus* threadStatus) {
	ChromeWebApplication *cwa = new ChromeWebApplication (threadStatus);
	std::string url;
	cwa->getUrl(url);

//	fprintf (stderr, "Created thread loop %s\n", threadStatus->pageId.c_str() );

//	MZNSendDebugMessageA("Started thread for %s", url.c_str());
	cwa->setPageData( threadStatus->pageData );
	threadStatus->pageData = NULL;
	threadStatus->refresh = false;
	MZNWebMatch(cwa);
	int last = 0;
	while (! threadStatus->end)
	{
		Event *event = threadStatus->waitForEvent();
		if (event != NULL)
		{
			last = 0;
			ActiveListenerInfo *listener = event->listener;
			if (! event->target.empty())
			{
				AbstractWebElement *element = new ChromeElement(listener->app, event->target.c_str());
				listener->listener->onEvent(listener->event.c_str(), listener->app, element, event->data.c_str());
				element->release();
			} else {
				listener->listener->onEvent(listener->event.c_str(), listener->app, listener->element, event->data.c_str());
			}
			delete event;
		}
		else
		{
			// Check if page has died
			last ++;
			if (last > 6)
			{
				last = 0;
				AbstractWebElement *body = cwa->getDocumentElement();
				if (body != NULL)
					body->release();
				else
				{
					threadStatus->end = true;
				}
			}
		}
		if (threadStatus->refresh)
		{
			if (cwa->getPageData() != NULL)
				delete cwa->getPageData();
			threadStatus->refresh = false;
			cwa->setPageData(threadStatus->pageData);
			threadStatus->pageData = NULL;
			MZNWebMatch(cwa);
		}
	}

	ThreadStatus *tsOld = threadPool.get(threadStatus->pageId);
	if (tsOld == threadStatus)
	{
		threadPool.remove(threadStatus->pageId);
	}

	activeListeners.removeApp(cwa);
	threadStatus->release();
	cwa->releaseWebPage();
	cwa->release();

//	fprintf (stderr, "End thread loop %s\n", threadStatus->pageId.c_str() );
}

std::string CommunicationManager::registerListener(ChromeElement* element,
		const char* event, WebListener* listener) {
	char ach[20];
	sprintf (ach, "%d", nextListener++);
	ActiveListenerInfo *al = new ActiveListenerInfo();
	al->event = event;
	al->element = element;
	ChromeWebApplication * app = dynamic_cast<ChromeWebApplication*>(element->getApplication());
	al->app = app;
	if (app != NULL)
		al->app->lock();
	al->listener = listener;
	al->element->lock();
	al->listener->lock();
	std::string id = ach;
	activeListeners.add(id, al);
//	fprintf(stderr, "Register element listener %s %s: %s\n", app->toString().c_str(), event, id.c_str());
	return id;
}

std::string CommunicationManager::unregisterListener(ChromeElement* element,
		const char* event, WebListener* listener) {
	activeListeners.removeByListener(element, event, listener);
//	fprintf(stderr, "UNREGISTER element listener %s %s\n", element->toString().c_str(), event);
	return std::string ("");

}

std::string CommunicationManager::registerListener(ChromeWebApplication* app,
		const char* event, WebListener* listener) {
	char ach[20];
	sprintf (ach, "%d", nextListener++);
	ActiveListenerInfo *al = new ActiveListenerInfo();
	al->event = event;
	al->element = NULL;
	al->app = app;
	if (app != NULL)
		al->app->lock();
	al->listener = listener;
	if (al->element != NULL)
		al->element->lock();
	al->listener->lock();
	std::string id = ach;
	activeListeners.add(id, al);
//	fprintf(stderr, "Register app listener %s %s: %s\n", app->toString().c_str(), event, ach);
	return id;
}

std::string CommunicationManager::unregisterListener(ChromeWebApplication* app,
		const char* event, WebListener* listener) {
	activeListeners.removeByApp(app, event, listener);
	return std::string ("");

}

std::string CommunicationManager::unregisterListener(ChromeWebApplication* app,
		const char* eventId) {
	activeListeners.remove(std::string(eventId));
//	fprintf(stderr, "UNREGISTER app listener %s %s\n", app->toString().c_str(), eventId);
	return std::string ("");

}

void CommunicationManager::sendEvent(const char* eventId, const char* pageId,
		const char* target, const char* data) {
	if (pageId != NULL && eventId != NULL)
	{
		ThreadStatus *ts = threadPool.get(std::string(pageId));
		if (ts != NULL)
		{
			ActiveListenerInfo *ali = activeListeners.get(std::string(eventId));
			if (ali != NULL)
			{
				Event *ev = new Event();
				ev->target = "";
				ev->data = (data == NULL ? "" : data);
				ev->listener = ali;
				ts->pendingEvents.push(ev);
				ts->notifyEventMessage();
			}
		}
	}

}

} /* namespace mazinger_chrome */

