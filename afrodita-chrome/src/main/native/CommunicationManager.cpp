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
#include <stdlib.h>
#include <string.h>
#include "ChromeWebApplication.h"

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
}

CommunicationManager::~CommunicationManager() {
}

CommunicationManager* CommunicationManager::getInstance() {
	if (instance == NULL)
		instance = new CommunicationManager();
	return instance;
}

std::string CommunicationManager::readMessage() {
	unsigned int messageSize;


	if (sizeof messageSize != 4)
	{
		return std::string();
	}

	std::string result;
	DEBUG ("Reading message header");

	if ( fread (&messageSize, 4, 1, stdin) == 1)
	{
		char achMsg[100];
		sprintf (achMsg, "Received message size: %d", messageSize);
		DEBUG (achMsg);
		char *buffer = (char*) malloc (messageSize+1);
		if (buffer != NULL)
		{
			if ( fread ( buffer, messageSize, 1, stdin) == 1)
			{
				buffer[messageSize] = '\0';
				DEBUG (buffer);
				result.assign (buffer);
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
	DEBUG ("Calling");
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
		msg += "\"";
		msg += messages[i];
		msg += "\"";
	}
	msg += "}";
	DEBUG (msg.c_str());
	writeMessage(msg);


	std::map<std::string,ThreadStatus*>::iterator it = threads.find(pageId);
	if (it == threads.end())
	{
		error = true;
		return NULL;
	}

	ThreadStatus *ts = it->second;

	JsonAbstractObject *jsonMsg = ts->waitForMessage();
	if (jsonMsg == NULL)
	{
		error = true;
		return NULL;
	}

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
			delete jsonMsg;
			return NULL;
		}
		JsonAbstractObject *result = map->getObject("response");
		map->remove("response");
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
  _In_  LPVOID arg
)
#else
static void* linuxThreadProc (void *arg)
#endif
{
	ThreadStatus *ts = (ThreadStatus*) arg;
	CommunicationManager::getInstance()->threadLoop(ts);
	return NULL;
}

void CommunicationManager::mainLoop() {
	do
	{
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

		std::string t;
		jsonMap->write(t, 3);
		if (messageName != NULL && messageName->value == "onLoad" && pageId != NULL)
		{
			JsonValue* title = dynamic_cast<JsonValue*>(jsonMap->getObject("title"));
			JsonValue* url = dynamic_cast<JsonValue*>(jsonMap->getObject("url"));

			ThreadStatus *ts = new ThreadStatus();
			ts->pageId = pageId->value;
			if (title != NULL)
				ts->title = title->value;
			if (url != NULL)
				ts->url = url->value;
			threads[ts->pageId] = ts;
#ifdef WIN32
			CreateThread (NULL,  0, win32ThreadProc, ts,0, NULL);
#else
			pthread_t threadId;
			pthread_create(&threadId, NULL, linuxThreadProc, ts);
#endif
		}
		else if (messageName != NULL && messageName->value == "info" && pageId != NULL)
		{
#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
			std::string response = "{\"action\": \"version\", \"version\":\"" STR(VERSION) "\", \"pageId\":\""+pageId->value +"\"}";
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
			delete jsonMap;
	} while (true);
}

void CommunicationManager::threadLoop(ThreadStatus* threadStatus) {
	ChromeWebApplication cwa (threadStatus);
	MZNWebMatch(&cwa);
}

} /* namespace mazinger_chrome */

