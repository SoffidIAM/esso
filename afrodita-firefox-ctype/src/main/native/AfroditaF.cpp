/*
 * MazingerFF.cpp
 *
 *  Created on: 22/11/2010
 *      Author: u07286
 */

#include "AfroditaF.h"
#include "FFWebApplication.h"
#include <MazingerInternal.h>
#include "EventHandler.h"
#include <map>
#include <SmartWebPage.h>
#include "WebAddonHelper.h"
#include <json/Encoder.h>
#include "SeyconServer.h"

#ifdef WIN32
#include <windows.h>
#else
#include <string.h>
#endif

#include <MZNcompat.h>

AfroditaHandler AfroditaHandler::handler;

AfroditaHandler::AfroditaHandler() {
	getUrlHandler = NULL;
}

extern "C" void AFRsetHandler (const char *id, void * handler) {
	if (strcmp (id, "GetUrl") == 0)
		AfroditaHandler::handler.getUrlHandler = (GetUrlHandler) handler;
	else if (strcmp (id, "GetTitle") == 0)
		AfroditaHandler::handler.getTitleHandler = (GetTitleHandler) handler;
	else if (strcmp (id, "GetDocumentElement") == 0)
		AfroditaHandler::handler.getDocumentElementHandler = (GetDocumentElementHandler) handler;
	else if (strcmp (id, "GetElementsByTagName") == 0)
		AfroditaHandler::handler.getElementsByTagNameHandler = (GetElementsByTagNameHandler) handler;
	else if (strcmp (id, "GetDomain") == 0)
		AfroditaHandler::handler.getDomainHandler = (GetDomainHandler) handler;
	else if (strcmp (id, "GetCookie" ) == 0)
		AfroditaHandler::handler.getCookieHandler = (GetCookieHandler) handler;
	else if (strcmp (id, "GetElementById" ) == 0)
		AfroditaHandler::handler.getElementByIdHandler = (GetElementByIdHandler) handler;
	else if (strcmp (id, "GetImages" ) == 0)
		AfroditaHandler::handler.getImagesHandler = (GetImagesHandler) handler;
	else if (strcmp (id, "GetLinks" ) == 0)
		AfroditaHandler::handler.getLinksHandler = (GetLinksHandler) handler;
	else if (strcmp (id, "GetAnchors" ) == 0)
		AfroditaHandler::handler.getAnchorsHandler = (GetAnchorsHandler) handler;
	else if (strcmp (id, "GetForms" ) == 0)
		AfroditaHandler::handler.getFormsHandler = (GetFormsHandler) handler;
	else if (strcmp (id, "Write" ) == 0)
			AfroditaHandler::handler.writeHandler = (WriteHandler) handler;
	else if (strcmp (id, "WriteLn" ) == 0)
			AfroditaHandler::handler.writeLnHandler = (WriteLnHandler) handler;
	else if (strcmp (id, "GetProperty" ) == 0)
			AfroditaHandler::handler.getPropertyHandler = (GetPropertyHandler) handler;
	else if (strcmp (id, "SetProperty" ) == 0)
			AfroditaHandler::handler.setPropertyHandler = (SetPropertyHandler) handler;
	else if (strcmp (id, "GetAttribute" ) == 0)
			AfroditaHandler::handler.getAttributeHandler = (GetAttributeHandler) handler;
	else if (strcmp (id, "RemoveAttribute" ) == 0)
			AfroditaHandler::handler.removeAttributeHandler = (RemoveAttributeHandler) handler;
	else if (strcmp (id, "RemoveChild" ) == 0)
			AfroditaHandler::handler.removeChildHandler = (RemoveChildHandler) handler;
	else if (strcmp (id, "SetAttribute" ) == 0)
			AfroditaHandler::handler.setAttributeHandler = (SetAttributeHandler) handler;
	else if (strcmp (id, "GetParent" ) == 0)
			AfroditaHandler::handler.getParentHandler = (GetParentHandler) handler;
	else if (strcmp (id, "GetOffsetParent" ) == 0)
			AfroditaHandler::handler.getOffsetParentHandler = (GetParentHandler) handler;
	else if (strcmp (id, "GetChildren" ) == 0)
			AfroditaHandler::handler.getChildrenHandler = (GetChildrenHandler) handler;
	else if (strcmp (id, "GetTagName" ) == 0)
			AfroditaHandler::handler.getTagNameHandler = (GetTagNameHandler) handler;
	else if (strcmp (id, "Click" ) == 0)
			AfroditaHandler::handler.clickHandler = (ClickHandler) handler;
	else if (strcmp (id, "Blur" ) == 0)
			AfroditaHandler::handler.blurHandler = (BlurHandler) handler;
	else if (strcmp (id, "Focus" ) == 0)
			AfroditaHandler::handler.focusHandler = (FocusHandler) handler;
	else if (strcmp (id, "SubscribeEvent" ) == 0)
			AfroditaHandler::handler.subscribeHandler = (SubscribeHandler) handler;
	else if (strcmp (id, "UnsubscribeEvent" ) == 0)
			AfroditaHandler::handler.unsubscribeHandler = (UnsubscribeHandler) handler;
	else if (strcmp (id, "CreateElement" ) == 0)
			AfroditaHandler::handler.createElementHandler = (CreateElementHandler) handler;
	else if (strcmp (id, "InsertBefore" ) == 0)
			AfroditaHandler::handler.insertBeforeHandler = (InsertBeforeHandler) handler;
	else if (strcmp (id, "AppendChild" ) == 0)
			AfroditaHandler::handler.appendChildHandler = (AppendChildHandler) handler;
	else if (strcmp (id, "GetNextSibling" ) == 0)
			AfroditaHandler::handler.getNextSiblingHandler = (GetNextSiblingHandler) handler;
	else if (strcmp (id, "GetPreviousSibling" ) == 0)
			AfroditaHandler::handler.getPreviousSiblingHandler = (GetPreviousSiblingHandler) handler;
	else if (strcmp (id, "Alert" ) == 0)
			AfroditaHandler::handler.alertHandler = (AlertHandler) handler;
	else if (strcmp (id, "SetTextContent" ) == 0)
			AfroditaHandler::handler.setTextContentHandler = (SetTextContentHandler) handler;
	else if (strcmp (id, "GetComputedStyle" ) == 0)
			AfroditaHandler::handler.getComputedStyleHandler = (GetComputedStyleHandler) handler;
	else
	{
#ifdef WIN32
		MessageBox (NULL, id, "Wrong Handler", MB_OK);
#endif
	}
}


static std::map<long,SmartWebPage*> status;


extern "C" void AFRevaluate (long  id) {

	//MZNC_waitMutex();

	MZNSendDebugMessage("Evaluating page %ld", id);

	FFWebApplication *app = new FFWebApplication(id);

	std::map<long,SmartWebPage*>::iterator it = status.find(id);
	if (it == status.end())
	{
		SmartWebPage * page = new SmartWebPage();
		app->setPage(page);
		status[id] = page;
	} else {
		it->second->lock();
		app->setPage(it->second);
	}

	MZNWebMatch(app);

	app->release();
	//MZNC_endMutex();
}

extern "C" void AFRevaluate2 (long  id, const char *data) {

	//MZNC_waitMutex();

	MZNSendDebugMessage("Evaluating page %ld with data", id);

	FFWebApplication *app = new FFWebApplication(id);

	app->pageData = new PageData();
	app->pageData->loadJson(data);

	std::map<long,SmartWebPage*>::iterator it = status.find(id);
	if (it == status.end())
	{
		SmartWebPage * page = new SmartWebPage();
		app->setPage(page);
		status[id] = page;
	} else {
		it->second->lock();
		app->setPage(it->second);
	}

	MZNWebMatch(app);

	app->release();
	//MZNC_endMutex();
}

extern "C" void AFRdismiss (long  id) {
	MZNSendDebugMessageA("<<<<<<<<<<<<<<<<<<<< Cleaning page %ld >>>>>>>>>>>>>>>>>>>>>>>>", id);
	FFWebApplication *app = new FFWebApplication(id);
	EventHandler::getInstance()->unregisterAllEvents(app);
	app->release();
	std::map<long,SmartWebPage*>::iterator it = status.find(id);
	if (it != status.end())
	{
		it->second->release();
		status.erase(it);
	}
}



extern "C" void AFRevent (long  eventId) {
	EventHandler::getInstance()->process (eventId);
}

extern "C" void AFRevent2 (long  eventId, long elementId) {
	EventHandler::getInstance()->process (eventId, elementId);
}


#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
extern "C" char * AFRgetVersion () {
	std::string link;
	SeyconCommon::readProperty("AutoSSOURL", link);

	std::string response = "{\"action\": \"version\", \"version\":\"" STR(VERSION) "\", \"url\":";
	response += json::Encoder::encode(link.c_str());
	response += "}";

	return strdup(response.c_str());
}

extern "C" char * AFRsearch (const char *text) {
	WebAddonHelper h;
	std::vector<UrlStruct> result;
	h.searchUrls (MZNC_utf8towstr(text), result);
	std::string response = " [";
	bool first = true;
	for ( std::vector<UrlStruct>::iterator it = result.begin(); it != result.end (); it++)
	{
		UrlStruct s = *it;
		if (first) first = false;
		else response += ",";
		response += "{\"url\":";
		response += json::Encoder::encode (MZNC_wstrtoutf8(s.url.c_str()).c_str());
		response += ",\"name\":";
		response += json::Encoder::encode (MZNC_wstrtoutf8(s.description.c_str()).c_str());
		response += "}";
	}
	response += "]";

	return strdup (response.c_str());
}

extern "C" void Test (const char *id) {
//	MessageBox (NULL, id, "Test", MB_OK);
}


#ifdef WIN32
extern "C" BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD dwReason,
		LPVOID lpvReserved) {

	if (dwReason == DLL_PROCESS_ATTACH) {
		hMazingerInstance = hinstDLL;
	}
	return TRUE;
}
#endif
