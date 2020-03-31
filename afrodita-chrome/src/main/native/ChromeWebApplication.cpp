/*
 * ExplorerWebApplication.cpp
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#include "AfroditaC.h"
#include "ChromeWebApplication.h"
#include "ChromeElement.h"
#include <string.h>
#include <stdio.h>
#include <vector>
#include "json/JsonMap.h"
#include "json/JsonVector.h"
#include "json/JsonValue.h"

#include "CommunicationManager.h"
#include <MazingerInternal.h>

#include "ChromeWidget.h"

#ifndef WIN32
#ifdef USE_QT
#include <QtWidgets/QApplication>
#else

class PopupMenu {
public:
	std::string title;
	std::vector<std::string> optionId;
	std::vector<std::string> names;
	AbstractWebElement *element;
	mazinger_chrome::ChromeWebApplication *app;
	std::string eventId;
	std::string selectedOption;
	std::vector<PopupMenu*> others;
};

static PopupMenu* currentMenu;
#endif
#endif


namespace mazinger_chrome
{


ChromeWebApplication::~ChromeWebApplication() {
	if (pageData != NULL)
		delete pageData;
	webPage->release();
	if (threadStatus != NULL)
		threadStatus->release();
}


void ChromeWebApplication::getUrl(std::string & value)
{
	value = url;
}



void ChromeWebApplication::getTitle(std::string & value)
{
	bool error;
	const char * msg [] = {"action","getTitle", "pageId", threadStatus->pageId.c_str(), NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		value = response->value;
		delete response;
	}
}



void ChromeWebApplication::getContent(std::string & value)
{
	value.assign("<not supported>");
}


AbstractWebElement *ChromeWebApplication::getDocumentElement()
{
	bool error;
	const char * msg [] = {"action","getDocumentElement", "pageId", threadStatus->pageId.c_str(), NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		ChromeElement* result = new ChromeElement(this, response->value.c_str());
		delete response;
		return result;
	}
	else
		return NULL;
}



void ChromeWebApplication::getElementsByTagName(const char*tag, std::vector<AbstractWebElement*> & elements)
{
	const char * msg [] = {"action","getElementsByTagName", "pageId", threadStatus->pageId.c_str(), "tagName", tag, NULL};
	generateCollection(msg, elements);
}



void ChromeWebApplication::getImages(std::vector<AbstractWebElement*> & elements)
{
	const char * msg [] = {"action","getImages", "pageId", threadStatus->pageId.c_str(), NULL};
	generateCollection(msg, elements);
}




void ChromeWebApplication::getLinks(std::vector<AbstractWebElement*> & elements)
{
	const char * msg [] = {"action","getLinks", "pageId", threadStatus->pageId.c_str(), NULL};
	generateCollection(msg, elements);
}



void ChromeWebApplication::getDomain(std::string & value)
{
	bool error;
	const char * msg [] = {"action","getDomain", "pageId", threadStatus->pageId.c_str(), NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		value = response->value;
		delete response;
	}
}

std::string ChromeWebApplication::toString() {
	return std::string("ChromeWebApplication ")+threadStatus->pageId;
}

AbstractWebElement* ChromeWebApplication::getElementBySoffidId(const char* id) {
	return new ChromeElement (this, id);
}

bool ChromeWebApplication::equals(ChromeWebApplication* app) {
	return (app != NULL && app->threadStatus->pageId == this->threadStatus->pageId);
}

void ChromeWebApplication::generateCollection(const char* msg[],
		std::vector<AbstractWebElement*>& elements) {
	elements.clear ();
	bool error;
	json::JsonVector* response =
			dynamic_cast<json::JsonVector*>(CommunicationManager::getInstance()->call(
					error, msg));
	if (response != NULL && !error) {
		for (std::vector<json::JsonAbstractObject*>::iterator it =
				response->objects.begin(); it != response->objects.end();
				it++) {
			json::JsonValue *v = dynamic_cast<json::JsonValue*>(*it);
			if (v != NULL) {
				ChromeElement *e = new ChromeElement(this, v->value.c_str());
				elements.push_back(e);
			}
		}
		delete response;
	}
}

void ChromeWebApplication::getAnchors(std::vector<AbstractWebElement*> & elements)
{
	const char * msg [] = {"action","getAnchors", "pageId", threadStatus->pageId.c_str(), NULL};
	generateCollection(msg, elements);
}



void ChromeWebApplication::getCookie(std::string & value)
{
	bool error;
	const char * msg [] = {"action","getCookie", "pageId", threadStatus->pageId.c_str(), NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		value = response->value;
		delete response;
	}
}



AbstractWebElement *ChromeWebApplication::getElementById(const char *id)
{
	bool error;
	const char * msg [] = {"action","getElementById", "pageId", threadStatus->pageId.c_str(), "elementId", id, NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error && response->value.size() > 0)
	{
		ChromeElement* result = new ChromeElement(this, response->value.c_str());
		delete response;
		return result;
	}
	else
		return NULL;
}



void ChromeWebApplication::getForms(std::vector<AbstractWebElement*> & elements)
{
	const char * msg [] = {"action","getForms", "pageId", threadStatus->pageId.c_str(), NULL};
	generateCollection(msg, elements);
}



void ChromeWebApplication::write(const char *text)
{
	bool error;
	const char * msg [] = {"action","write", "pageId", threadStatus->pageId.c_str(), "text", text, NULL};
	json::JsonAbstractObject * response = CommunicationManager::getInstance()->call(error,msg);

	if (response != NULL)
		delete response;
}

void ChromeWebApplication::writeln(const char *text)
{
	bool error;
	const char * msg [] = {"action","writeln", "pageId", threadStatus->pageId.c_str(), "text", text, NULL};
	json::JsonAbstractObject * response = CommunicationManager::getInstance()->call(error,msg);

	if (response != NULL)
		delete response;
}

void ChromeWebApplication::execute(const char *script)
{
	bool error;
	const char * msg [] = {"action","execute", "pageId", threadStatus->pageId.c_str(), "text", script, NULL};
	json::JsonAbstractObject * response = CommunicationManager::getInstance()->call(error,msg);

	if (response != NULL)
		delete response;
}




ChromeWebApplication::ChromeWebApplication(ThreadStatus *threadStatus) {
	this->threadStatus = threadStatus;
	this->threadStatus->lock();
	title = threadStatus->title;
	url = threadStatus->url;
	pageData = threadStatus -> pageData;
	this->webPage = new SmartWebPage;
}


AbstractWebElement* ChromeWebApplication::createElement(const char* tagName) {
	bool error;
	AbstractWebElement * result = NULL;
	const char * msg [] = {"action","createElement", "pageId", threadStatus->pageId.c_str(), "tagName", tagName, NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL )
	{
		if (! error  && !response->value.empty())
			result = new ChromeElement(this, response->value.c_str());
		delete response;
	}
	return result;
}

void ChromeWebApplication::alert(const char* str) {
	bool error;
	const char * msg [] = {"action","alert", "pageId", threadStatus->pageId.c_str(), "text", str, NULL};
	json::JsonAbstractObject * response = CommunicationManager::getInstance()->call(error,msg);

	if (response != NULL)
		delete response;
}

void ChromeWebApplication::subscribe(const char* eventName,
		WebListener* listener) {
}

void ChromeWebApplication::unSubscribe(const char* eventName,
		WebListener* listener) {
}


bool ChromeWebApplication::supportsPageData() {
	return true;
}

PageData* ChromeWebApplication::getPageData() {
	return pageData;
}


void ChromeWebApplication::selectAction (const char * title,
		std::vector<std::string> &optionId,
		std::vector<std::string> &names,
		AbstractWebElement *element,
		WebListener *listener)
{
#ifdef WIN32
	AbstractWebApplication::selectAction(title, optionId, names,
			element, listener);
#else

#ifdef USE_QT
	char *argv [2] = {"afrodita-chrome", NULL};
	int argc = 1;
	ChromeWidget w;

	std::string r = w.selectAction(title, optionId, names);

	if (!r.empty())
	{
		listener->onEvent("selectedAction", this, element, r.c_str());
	}
#else
	MZNSendDebugMessage("Creating menu structure");
	PopupMenu *m = new PopupMenu();

	this->lock();
	listener->lock();
	if (element != NULL)
		element->lock();

	m->app = this;
	m->element = element;
	m->title = title;
	m->eventId = CommunicationManager::getInstance()->registerListener(this, "selectAction", listener);
	for (int i = 0; i < optionId.size(); i ++)
		m->optionId.push_back(optionId[i]);
	for (int i = 0; i < names.size() ; i++)
		m->names.push_back(MZNC_utf8tostr(names[i].c_str()).c_str());

	currentMenu = m;
	gdk_threads_add_idle(menuPopupHandler, m);
#endif
#endif

#if 0
	std::string listenerId = CommunicationManager::getInstance()
		->registerListener(this, "selectedAction", listener);

	bool error;

	ChromeElement *chromeElement = dynamic_cast<ChromeElement*> (element);

	json::JsonMap * m = new json::JsonMap();
	m->setObject("action", new json::JsonValue("selectAction"));
	m->setObject("title", new json::JsonValue(title));
	m->setObject("pageId", new json::JsonValue(threadStatus->pageId.c_str()));
	m->setObject("listener", new json::JsonValue(listenerId.c_str()));
	if (chromeElement != NULL)
	{
		m->setObject("element", new json::JsonValue(chromeElement->getExternalId().c_str()));
	}

	json::JsonVector *v = new json::JsonVector();
	for (int i = 0; i < names.size() && i < optionId.size(); i++)
	{
		json::JsonMap *m2 = new json::JsonMap();
		m2->setObject ("id",  new json::JsonValue(optionId[i].c_str()));
		m2->setObject ("name", new json::JsonValue(names[i].c_str()));
		v->objects.push_back(m2);
	}
	m->setObject("options", v);

	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error, m));

	if (response != NULL)
	{
		delete response;
	}

	delete m;
#endif
}

void ChromeWebApplication::lock() {
	LockableObject::lock();
}

void ChromeWebApplication::release() {
	LockableObject::release();
}


void ChromeWebApplication::releaseWebPage() {
	webPage->release();
	webPage = new SmartWebPage();
}
}


#ifndef WIN32
#ifndef USE_QT

#include "CommunicationManager.h"

static gboolean onActivate (GtkWidget *dialog, gpointer userdata) {
	gtk_dialog_response(GTK_DIALOG(dialog), 0);
	return true;
}

static gboolean onSelect (GtkWidget *widget, gpointer userdata) {
	GtkWidget *dialog = gtk_widget_get_toplevel(widget);

	PopupMenu *ld = (PopupMenu*) userdata;
	if (ld != NULL &&
			! ld->selectedOption.empty())
	{
		MZNSendDebugMessage("Selected %s", ld->selectedOption.c_str());

		mazinger_chrome::CommunicationManager::getInstance()
			->sendEvent(ld->eventId.c_str(), ld->app->threadStatus->pageId.c_str(),
					NULL, ld->selectedOption.c_str());
		gtk_dialog_response(GTK_DIALOG(dialog), 1);
	}
	return true;
}

static gboolean releaseMenuObjects (PopupMenu *ld)
{
	if (ld != NULL)
	{

		if (ld->element != NULL)
		{
			ld->element->sanityCheck();
			ld->element->release();
		}
		mazinger_chrome::CommunicationManager::getInstance()->unregisterListener(ld->app, ld->eventId.c_str());
		if (ld->app != NULL)
			ld->app->release();
		for (int i = 0 ; i < ld->others.size(); i++)
		{
			delete ld->others[i];
		}
		delete ld;
	}
	return false;
}

gboolean menuPopupHandler (gpointer dummy) {
	PopupMenu *masterData  = currentMenu;

//	GtkWidget* signalWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
//	gtk_window_set_position(GTK_WINDOW(signalWindow), GTK_WIN_POS_MOUSE);

	GtkWidget *dialog = gtk_dialog_new();
	GdkWindow * w = gtk_widget_get_parent_window ( dialog );

	gdk_window_set_events ( w, GDK_FOCUS_CHANGE_MASK );
	gtk_window_set_title(GTK_WINDOW(dialog), masterData->title.c_str());

	GtkWidget * content = gtk_dialog_get_content_area(
			GTK_DIALOG(dialog));

	for (int i = 0; i < masterData->optionId.size() && i < masterData->names.size(); i++)
	{
		PopupMenu *l = new PopupMenu;
		l->eventId = masterData->eventId;
		l->selectedOption = masterData->optionId[i];
		l->app = masterData->app;
		l->element = masterData->element;
		masterData->others.push_back(l);


		if ( masterData->optionId[i].empty() )
		{
			GtkWidget * child = gtk_label_new(masterData->names[i].c_str());
			gtk_widget_show(child);
			gtk_container_add ( GTK_CONTAINER( content ), child);

		}
		else
		{
			GtkWidget * child = gtk_link_button_new_with_label(masterData->names[i].c_str(), masterData->names[i].c_str());
			gtk_widget_show(child);
			gtk_container_add ( GTK_CONTAINER( content ), child);

			g_signal_connect(child, "activate-link", G_CALLBACK(onSelect), l);
		}
	}

	MZNSendDebugMessageA("Show menu");

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), -100);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	g_signal_connect(dialog, "focus-out-event", G_CALLBACK(onActivate), NULL);
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	MZNSendDebugMessageA("Closed popup %d", result);

	releaseMenuObjects(masterData);

	gtk_widget_hide(dialog);

	return FALSE;
}


#endif
#endif
