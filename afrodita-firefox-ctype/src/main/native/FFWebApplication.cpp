/*
 * ExplorerWebApplication.cpp
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#include "AfroditaF.h"
#include "FFWebApplication.h"
#include "FFElement.h"
#include "EventHandler.h"
#include <SmartWebPage.h>

#include <stdio.h>

FFWebApplication::~FFWebApplication() {
	if (pageData != NULL)
		delete pageData;
	pageData = NULL;

	if (page != NULL)
		page->release();
	page = NULL;
}


void FFWebApplication::getUrl(std::string & value)
{
	value.clear ();
	if ( AfroditaHandler::handler.getUrlHandler != NULL)
	{
		const char *v  = AfroditaHandler::handler.getUrlHandler(docId);
		if (v != NULL)
			value = v;
	}
}



void FFWebApplication::getTitle(std::string & value)
{
	value.clear ();
	if ( AfroditaHandler::handler.getTitleHandler != NULL)
	{
		const char *v  = AfroditaHandler::handler.getTitleHandler(docId);
		if (v != NULL)
			value = v;
	}
}



void FFWebApplication::getContent(std::string & value)
{
	value.assign("<not supported>");
}

FFWebApplication::FFWebApplication(long docId)
{
	this->docId = docId;
	this->pageData = NULL;
	this->page = NULL;
}

AbstractWebElement *FFWebApplication::getDocumentElement()
{
	if (AfroditaHandler::handler.getDocumentElementHandler != NULL) {
		long id = AfroditaHandler::handler.getDocumentElementHandler (docId);
		return new FFElement (this, id);
	} else {
		return NULL;
	}
}



void FFWebApplication::getElementsByTagName(const char*tag, std::vector<AbstractWebElement*> & elements)
{
	elements.clear();
	if (AfroditaHandler::handler.getElementsByTagNameHandler != NULL) {
		long* id = AfroditaHandler::handler.getElementsByTagNameHandler (docId, tag);
		for (int i = 0; id != NULL && id[i] != 0; i++) {
			FFElement *element = new FFElement(this, id[i]);
			elements.push_back(element);
		}
	}
}



void FFWebApplication::getImages(std::vector<AbstractWebElement*> & elements)
{
	elements.clear();
	if (AfroditaHandler::handler.getImagesHandler != NULL) {
		long* id = AfroditaHandler::handler.getImagesHandler (docId);
		for (int i = 0; id != NULL && id[i] != 0; i++) {
			FFElement *element = new FFElement(this, id[i]);
			elements.push_back(element);
		}
	}
}




void FFWebApplication::getLinks(std::vector<AbstractWebElement*> & elements)
{
	elements.clear();
	if (AfroditaHandler::handler.getLinksHandler != NULL) {
		long* id = AfroditaHandler::handler.getLinksHandler (docId);
		for (int i = 0; id != NULL && id[i] != 0; i++) {
			FFElement *element = new FFElement(this, id[i]);
			elements.push_back(element);
		}
	}
}



void FFWebApplication::getDomain(std::string & value)
{
	value.clear ();
	if ( AfroditaHandler::handler.getDomainHandler != NULL)
	{
		const char *v  = AfroditaHandler::handler.getDomainHandler(docId);
		if (v != NULL)
			value = v;
	}
}



void FFWebApplication::getAnchors(std::vector<AbstractWebElement*> & elements)
{
	elements.clear();
	if (AfroditaHandler::handler.getAnchorsHandler != NULL) {
		long* id = AfroditaHandler::handler.getAnchorsHandler (docId);
		for (int i = 0; id != NULL && id[i] != 0; i++) {
			FFElement *element = new FFElement(this, id[i]);
			elements.push_back(element);
		}
	}
}



void FFWebApplication::getCookie(std::string & value)
{
	value.clear ();
	if ( AfroditaHandler::handler.getCookieHandler != NULL)
	{
		const char *v  = AfroditaHandler::handler.getCookieHandler(docId);
		if (v != NULL)
			value = v;
	}
}



AbstractWebElement *FFWebApplication::getElementById(const char *id)
{
	if (AfroditaHandler::handler.getElementByIdHandler != NULL) {
		long internalId = AfroditaHandler::handler.getElementByIdHandler (docId, id);
		if (internalId == 0)
			return NULL;
		else
			return new FFElement(this, internalId);
	}
	else
		return NULL;
}



void FFWebApplication::getForms(std::vector<AbstractWebElement*> & elements)
{
	elements.clear();
	if (AfroditaHandler::handler.getFormsHandler != NULL) {
		long* id = AfroditaHandler::handler.getFormsHandler (docId);
		for (int i = 0; id != NULL && id[i] != 0; i++) {
			FFElement *element = new FFElement(this, id[i]);
			elements.push_back(element);
		}
	}
}



void FFWebApplication::write(const char *str)
{
	if (AfroditaHandler::handler.writeHandler != NULL) {
		AfroditaHandler::handler.writeHandler(docId, str);
	}
}

void FFWebApplication::writeln(const char *str)
{
	if (AfroditaHandler::handler.writeLnHandler != NULL) {
		AfroditaHandler::handler.writeLnHandler(docId, str);
	}
}

AbstractWebElement* FFWebApplication::createElement(const char* tagName) {
	if (AfroditaHandler::handler.createElementHandler != NULL) {
		long internalId = AfroditaHandler::handler.createElementHandler(docId, tagName);
		if (internalId == 0)
			return NULL;
		else
			return new FFElement(this, internalId);
	}
	else
		return NULL;
}

SmartWebPage* FFWebApplication::getWebPage() {
	return page;
}

void FFWebApplication::alert(const char* str) {
	if (AfroditaHandler::handler.alertHandler != NULL)
		AfroditaHandler::handler.alertHandler(docId, str);
}

void FFWebApplication::subscribe(const char* eventName, WebListener* listener) {
	EventHandler::getInstance()->registerEvent(listener, this, eventName);
}

void FFWebApplication::unSubscribe(const char* eventName,
		WebListener* listener) {
	EventHandler::getInstance()->unregisterEvent(listener, this, eventName);
}

std::string FFWebApplication::toString() {
	char ach[100];
	sprintf (ach, "FFWebApplication %d", docId);
	return std::string(ach);

}

bool FFWebApplication::supportsPageData() {
	return true;
}

PageData* FFWebApplication::getPageData() {
	return pageData;
}

AbstractWebElement* FFWebApplication::getElementBySoffidId(const char* id) {
	long elementId = 0;
	sscanf (id, " %ld", &elementId);
	return new FFElement (this, elementId);
}


#ifndef WIN32

#include <gtk/gtk.h>

class ListenerData {
public:
	WebListener *listener;
	FFWebApplication *app;
	AbstractWebElement *element;
	std::string id;
	std::vector<ListenerData*> v;
};

static gboolean onSelectGtkMenu (GtkWidget *menuitem, gpointer userdata) {

	MZNSendDebugMessage("Selected");
	ListenerData *ld = (ListenerData*) userdata;
	if (ld != NULL &&
			ld->listener != NULL &&
			! ld->id.empty())
	{
		MZNSendDebugMessage("Selected %s", ld->id.c_str());
		ld->listener->onEvent("selectAction", ld->app, ld->element, ld->id.c_str());
	}
	return false;
}

static gboolean releaseMenuObjects (GtkWidget *widget,
               gpointer   user_data)
{
	MZNSendDebugMessage("Cleaning menu data");
	ListenerData *ld = (ListenerData*) user_data;
	if (ld != NULL)
	{
		MZNSendDebugMessage("Cleaning menu data q %p", ld);

		if (ld->element != NULL)
		{
			MZNSendDebugMessage("Cleaning menu data q1.");
			ld->element->sanityCheck();
			MZNSendDebugMessage("Cleaning menu data q1q");
			ld->element->release();
		}
		MZNSendDebugMessage("Cleaning menu data q1");
		if (ld->listener != NULL)
			ld->listener->release();
		MZNSendDebugMessage("Cleaning menu data q2");
		if (ld->app != NULL)
			ld->app->release();
		MZNSendDebugMessage("Cleaning menu data B");
		for (int i = 0 ; i < ld->v.size(); i++)
		{
			MZNSendDebugMessage("Cleaning menu data %d", i);
			delete ld->v[i];
		}
		MZNSendDebugMessage("Cleaning menu data END");
		delete ld;
	}
	MZNSendDebugMessage("Cleaned up menu data");
	return false;
}

static gboolean displayGtkMenu (const char * title,
		std::vector<std::string> &optionId,
		std::vector<std::string> &names,
		FFWebApplication *app,
		AbstractWebElement *element,
		WebListener *listener) {

	gdk_threads_enter();

	GtkWidget *menu, *menuitem;
	menu = gtk_menu_new();

	menuitem = gtk_menu_item_new_with_label(title);


	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_set_sensitive(menuitem, false);


	app->lock();
	listener->lock();
	if (element != NULL)
		element->lock();

	ListenerData *masterData = new ListenerData;

	for (int i = 0; i < optionId.size() && i < names.size(); i++)
	{
		ListenerData *l = new ListenerData;
		l->listener = listener;
		l->id = optionId[i];
		l->app = app;
		l->element = element;
		masterData->v.push_back(l);
		menuitem = gtk_menu_item_new_with_label(names[i].c_str());
		MZNSendDebugMessageA("Created option %s", names[i].c_str());
		if ( optionId[i].empty() )
		{
			gtk_widget_set_sensitive(menuitem, false);
		}
		else
		{
			MZNSendDebugMessageA("Connecting id %s", optionId[i].c_str());
			g_signal_connect(menuitem, "activate",
					G_CALLBACK(onSelectGtkMenu), l);
		}
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}

	masterData->app = app;
	masterData->element = element;
	masterData->listener = listener;

	gtk_widget_show_all(menu);
	MZNSendDebugMessage("Setting menu data on %p", masterData);

	g_signal_connect(menu, "destroy",
			G_CALLBACK(releaseMenuObjects), masterData);

	/* Note: event can be NULL here when called from view_onPopupMenu;
	 *  gdk_event_get_time() accepts a NULL argument */
	gdk_threads_leave();

	MZNSendDebugMessageA("Opening popup");
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
	MZNSendDebugMessageA("Closed popup");

//	listener->release();
//	element->release();


	return TRUE;
}

#endif

void FFWebApplication::selectAction(const char* title,
		std::vector<std::string>& optionId, std::vector<std::string>& names,
		AbstractWebElement* element, WebListener* listener) {
#ifdef WIN32
	AbstractWebApplication::selectAction(title, optionId, names,
			element, listener);
#else
	displayGtkMenu(title,
			optionId,
			names,
			this,
			element,
			listener);
#endif

}
