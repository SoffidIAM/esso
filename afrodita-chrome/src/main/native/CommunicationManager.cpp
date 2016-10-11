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

namespace mazinger_chrome {

CommunicationManager *CommunicationManager::instance = NULL;

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
	writeMessage(msg);



	std::map<std::string,ThreadStatus*>::iterator it = threads.find(pageId);
	if (it == threads.end())
	{
		error = true;
//		MZNSendDebugMessage("ERROR :Thread not found for %s", pageId.c_str());
		return NULL;
	}


	ThreadStatus *ts = it->second;

	JsonAbstractObject *jsonMsg = ts->waitForMessage();
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
			delete jsonMsg;
			return NULL;
		}
		JsonAbstractObject *result = map->getObject("response");
		map->remove("response");
//		MZNSendDebugMessage("Respnse got");
		delete jsonMsg;
		error = false;
		return result;
	}
	else
	{
		error = true;
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

static long parseInt (JsonMap *map, const char *tag)
{
	JsonValue *v = dynamic_cast<JsonValue*>(map->getObject(tag));
	if (v == NULL)
		return 0;
	else
	{
		long result;
		sscanf (" %d", v->value.c_str(), &result);
		return result;
	}
}

static std::string parseString (JsonMap *map, const char *tag)
{
	JsonValue *v = dynamic_cast<JsonValue*>(map->getObject(tag));
	if (v == NULL)
		return std::string("");
	else
	{
		return v->value;
	}
}

static InputData parseInput (JsonMap* value)
{
	InputData d;
	d.clientHeight = parseInt (value, "clientHeight");
	d.clientWidth = parseInt (value, "clientWidth");
	d.data_bind = parseString (value, "data_bind");
	d.display = parseString (value, "display");
	d.id = parseString (value, "id");
	d.name = parseString(value, "name");
	d.offsetHeight = parseInt (value, "offsetHeight");
	d.offsetLeft = parseInt (value, "offsetLeft");
	d.offsetTop = parseInt (value, "offsetTop");
	d.offsetWidth = parseInt (value, "offsetWidth");
	d.soffidId = parseString(value,"soffidId");
	d.style = parseString (value, "style");
	d.text_align = parseString(value, "text_align");
	d.type = parseString(value,"type");
	d.rightAlign = d.text_align == "right";

	return d;
}

static FormData parseForm (JsonMap* value)
{
	FormData d;
	d.action = parseString(value, "action");
	d.id = parseString(value, "id");
	d.method = parseString(value, "method");
	d.name = parseString(value, "name");
	d.soffidId = parseString(value,"soffidId");
	JsonVector *v = dynamic_cast<JsonVector*> (value->getObject("inputs"));
	if (v != NULL)
	{
		std::vector<JsonAbstractObject*> values = v->objects;
		for (std::vector<JsonAbstractObject*>::iterator it = values.begin();
				it != values.end(); it++)
		{
			JsonMap *input =  dynamic_cast<JsonMap*>(*it);
			if (input != NULL)
				d.inputs.push_back( parseInput(input));
		}
	}
	return d;
}

static PageData * parsePageData (ThreadStatus *status, JsonMap* map)
{
	if (map == NULL)
		return NULL;
	PageData *pd = new PageData();

	pd->title = status->title;
	pd->url = status->url;
	JsonVector *v = dynamic_cast <JsonVector*> (map->getObject("inputs"));
	if (v != NULL)
	{
		std::vector<JsonAbstractObject*> values = v->objects;
		for (std::vector<JsonAbstractObject*>::iterator it = values.begin ();
				it != values.end();
				it ++)
		{
			JsonMap *i = dynamic_cast<JsonMap*> (*it);
			if (i != NULL)
			{
				pd->inputs.push_back(parseInput (i));
			}
		}
	}
	v = dynamic_cast <JsonVector*> (map->getObject("forms"));
	if (v != NULL)
	{
		std::vector<JsonAbstractObject*> values = v->objects;
		for (std::vector<JsonAbstractObject*>::iterator it = values.begin ();
				it != values.end();
				it ++)
		{
			JsonMap *i = dynamic_cast<JsonMap*> (*it);
			if (i != NULL)
			{
				pd->forms.push_back(parseForm (i));
			}
		}
	}
	return pd;
}

void CommunicationManager::mainLoop() {
	do
	{
		DEBUG("Main loop");
		JsonAbstractObject *message = getEventMessage();
		if (message == NULL)
		{
			DEBUG ("End message");
			return;
		}
		bool deleteMap = true;
		JsonMap *jsonMap = dynamic_cast<JsonMap*>(message);
		JsonValue* messageName = dynamic_cast<JsonValue*>(jsonMap->getObject("message"));
		JsonValue* pageId = dynamic_cast<JsonValue*>(jsonMap->getObject("pageId"));
		JsonMap *jsonPageData = dynamic_cast<JsonMap*>(jsonMap->getObject("pageData"));

		std::string t;
		jsonMap->write(t, 3);
//		MZNSendDebugMessage("RECEIVED MSG**********************");
//		MZNSendDebugMessage("Received %s", t.c_str());
		if (messageName != NULL && messageName->value == "onLoad" && pageId != NULL)
		{
			JsonValue* title = dynamic_cast<JsonValue*>(jsonMap->getObject("title"));
			JsonValue* url = dynamic_cast<JsonValue*>(jsonMap->getObject("url"));

			ThreadStatus *ts = threads[pageId->value];
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
				ts = new ThreadStatus();
				if (pageId != NULL)
				{
					ts->pageId = pageId->value;
					if (title != NULL)
						ts->title = title->value;
					if (url != NULL)
						ts->url = url->value;
					ts->pageData = parsePageData ( ts, jsonPageData );
					threads[ts->pageId] = ts;
#ifdef WIN32
					CreateThread (NULL,  0, win32ThreadProc, ts,0, NULL);
#else
					pthread_t threadId;
					pthread_create(&threadId, NULL, linuxThreadProc, ts);
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
				ThreadStatus *ts = threads[pageId->value];
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

			if (pageId != NULL && eventId != NULL)
			{
				ThreadStatus *ts = threads[pageId->value];
				if (ts != NULL)
				{
//					MZNSendDebugMessage("Received event %s target %s", eventId->value.c_str(), target->value.c_str());
					std::map<std::string,ActiveListenerInfo*>::iterator it = activeListeners.find(eventId->value);
					if (it != activeListeners.end())
					{
						ActiveListenerInfo *ali = it->second;
						Event *ev = new Event();
//						MZNSendDebugMessage("ALI event = (%lx) ali = (%lx) %s", (long) ev, (long) ali, ali->event.c_str());
						ev->target = (target == NULL ? "" : target->value.c_str());
						ev->listener = ali;
						ts->pendingEvents.push(ev);
						ts->notifyEventMessage();
//						MZNSendDebugMessage("Notified event");
					}
				}
			}
		}
		if (messageName != NULL && messageName->value == "search" && pageId != NULL)
		{
			JsonValue* text = dynamic_cast<JsonValue*>(jsonMap->getObject("text"));
//			MZNSendDebugMessage("Received event %s : %s", messageName->value.c_str(), text->value.c_str());
			WebAddonHelper h;
			std::vector<UrlStruct> result;
			h.searchUrls (MZNC_utf8towstr(text->value.c_str()), result);
			std::string response = " {\"action\":\"searchResult\", \"pageId\":\""+pageId->value +"\",\"result\": [";
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
				response += "}";
			}
			response += "]}";
//			MZNSendDebugMessage("Response %s", response.c_str());
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
				std::map<std::string,ThreadStatus*>::iterator it = threads.find(pageId->value);
				if (it != threads.end() && ! it->second->end)
				{
					it->second->notifyMessage(jsonMap);
					deleteMap = false;
				}
			}
		}
		if (deleteMap)
			delete message;
	} while (true);
}

void CommunicationManager::threadLoop(ThreadStatus* threadStatus) {
	ChromeWebApplication *cwa = new ChromeWebApplication (threadStatus);
	cwa->setPageData(threadStatus->pageData);
	threadStatus->refresh = false;
	MZNWebMatch(cwa);
	while (! threadStatus->end)
	{
		Event *event = threadStatus->waitForEvent();
		if (event != NULL)
		{
			ActiveListenerInfo *listener = event->listener;
			if (! event->target.empty())
			{
				AbstractWebElement *element = new ChromeElement(listener->app, event->target.c_str());
				listener->listener->onEvent(listener->event.c_str(), listener->app, element);
				element->release();
			} else {
				listener->listener->onEvent(listener->event.c_str(), listener->app, listener->element);
			}
			delete event;
		}
		if (threadStatus->refresh)
		{
			threadStatus->refresh = false;
			cwa->setPageData(threadStatus->pageData);
			if (threadStatus->pageData != NULL)
			{
				delete threadStatus -> pageData;
				threadStatus -> pageData = NULL;
			}
			MZNWebMatch(cwa);
		}
	}
	threads.erase(threadStatus->pageId);
	for (std::map<std::string,ActiveListenerInfo*>::iterator it = activeListeners.begin(); it != activeListeners.end();)
	{
		if (it->second->app == cwa)
		{
			ActiveListenerInfo *ali = it->second;
			std::map<std::string,ActiveListenerInfo*>::iterator it2 = it ++;
			activeListeners.erase(it2);
			ali->element->release();
			ali->listener->release();
			delete ali;
		}
		else
			it ++;
	}
	if (threadStatus->pageData != NULL)
		delete threadStatus->pageData;
	cwa->release();
	delete threadStatus;
}

std::string CommunicationManager::registerListener(ChromeElement* element,
		const char* event, WebListener* listener) {
	char ach[20];
	sprintf (ach, "%d", nextListener++);
	ActiveListenerInfo *al = new ActiveListenerInfo();
	al->event = event;
	al->element = element;
	ChromeWebApplication * app = dynamic_cast<ChromeWebApplication*>(element->getApplication());
	if (app != NULL)
	{
		al->app = app;
	}
	al->listener = listener;
	al->element->lock();
	al->listener->lock();
	std::string id = ach;
	activeListeners[ach] = al;
//	MZNSendDebugMessageA("Registering listener %s [ %s ]",event, ach);
	return id;
}

std::string CommunicationManager::unregisterListener(ChromeElement* element,
		const char* event, WebListener* listener) {
	for (std::map<std::string,ActiveListenerInfo*>::iterator it = activeListeners.begin(); it != activeListeners.end(); it++)
	{
		ActiveListenerInfo *al = it->second;
		if (al->element->equals (element) && al->event == event && al->listener == listener)
		{
			std::string id = it->first;
			al->element->release();
			al->listener->release();
			activeListeners.erase(id);
			return  id;
		}
	}
	return std::string ("");

}

} /* namespace mazinger_chrome */

