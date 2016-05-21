/*
 * SmartForm.cpp
 *
 *  Created on: 31/08/2015
 *      Author: gbuades
 */

#include "SmartForm.h"
#include <MazingerInternal.h>
#include <SecretStore.h>
#include <string.h>
#include <ScriptDialog.h>
#include <SeyconServer.h>
#include <WebListener.h>

#include <SmartWebPage.h>

#include <img_password.h>
#include <img_save.h>
#include <img_generate.h>
#include <img_unlock.h>
#include <img_user.h>
#include <img_link.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

class OnChangeListener: public WebListener
{
public:
	virtual std::string toString () { return std::string("OnChangeListener");}
	SmartForm *form;
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component);

};

class OnClickListener: public WebListener
{
public:
	virtual std::string toString () { return std::string("OnClickListener");}
	SmartForm *form;
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component);

};

class OnBeforeUnloadListener: public WebListener
{
public:
	virtual std::string toString () { return std::string("OnBeforeUnloadListener");}
	SmartForm *form;
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component);

};

class OnHiddenElementFocusListener: public WebListener
{
public:
	virtual std::string toString () { return std::string("OnHiddenElementFocusListener");}
	SmartForm *form;
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component);

};


void OnChangeListener::onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component) {
	if (MZNC_waitMutex())
	{
		component->sanityCheck();
		app->sanityCheck();
		if (form != NULL)
		{
			form->sanityCheck();
			form -> onChange (component);
		}
		MZNC_endMutex();
	}

}

void OnClickListener::onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component) {
	if (MZNC_waitMutex())
	{
		std::string managed = "";
		std::string tag = "";
		component->getTagName(tag);
//		MZNSendDebugMessageA("On click event %s - %s", tag.c_str(), managed.c_str() );

		component->sanityCheck();
		app->sanityCheck();
		if (form != NULL)
		{
			form->sanityCheck();
			std::string id;
			component->getAttribute("_soffid_handler", id);
			if (id == "true")
				form -> onClickImage(component);
			else
			{
				component->getAttribute("_soffid_account", id);
				if (!id.empty())
					form -> onClickAccount(component);
				else
				{
					component->getAttribute("_soffid_modal", id);
					if (id == "true")
						form -> onClickModal(component);
					else
					{
						component->getAttribute("_soffid_level", id);
						if (! id.empty())
							form -> onClickLevel(component);
						else
						{
							component->getAttribute("_soffid_save", id);
							if (! id.empty())
								form -> onClickSave(component);
						}
					}
				}
			}
		}
//		MZNSendDebugMessageA("END On click event %s - %s", tag.c_str(), managed.c_str() );
		MZNC_endMutex();
	}
}

void OnBeforeUnloadListener::onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component) {
	if (MZNC_waitMutex())
	{
		app->sanityCheck();
		form->sanityCheck();
		form->onBeforeUnload();
		MZNC_endMutex();
	}
}

void OnHiddenElementFocusListener::onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component) {
	if (MZNC_waitMutex())
	{
		component->sanityCheck();
		app->sanityCheck();
		if (form != NULL)
		{
			std::string pending ;
			component->getProperty("soffidOnFocusTrigger", pending);
			if (pending == "true")
			{
				component->setProperty("soffidOnFocusTrigger", "false");
				form->sanityCheck();
				form->reparse();
			}
		}
		MZNC_endMutex();
	}

}
SmartForm::SmartForm(SmartWebPage *page) {
	this->page = page;
	this->app = NULL;
	this->element = NULL;
	this->onChangeListener = NULL;
	this->onClickListener = NULL;
	this->onBeforeUnloadListener = NULL;
	this->onHiddenElementFocusListener = new OnHiddenElementFocusListener();
	this->onHiddenElementFocusListener->form = this;
	status = SF_STATUS_NEW;
	char ach[100];
	sprintf (ach, "_soffid_%ld_id_", (long) (long long) this);
	stylePrefix = ach;
	numPasswords = 0;
	parsed = false;
}

SmartForm::~SmartForm() {
	if (this->app != NULL)
		this->app->release();
	if (this->element != NULL)
		this->element->release();
	if (this->onChangeListener != NULL)
		this->onChangeListener->release();
	if (this->onClickListener != NULL)
		this->onClickListener->release();
	if (this->onBeforeUnloadListener != NULL)
		this->onBeforeUnloadListener->release();
	if (this->onHiddenElementFocusListener != NULL)
		this->onHiddenElementFocusListener->release();
}


static long getIntProperty (AbstractWebElement *element, const char* property)
{
	std::string s;
	element->getProperty(property, s);
	long l = 0;
	sscanf (s.c_str(), " %ld", &l);
	return l;
}


void SmartForm::findInputs (AbstractWebApplication* app, AbstractWebElement *element, std::vector<AbstractWebElement*> &inputs,
		bool first, bool visible, std::string indent)
{
	std::string tagname;
	element->getTagName(tagname);
	if (visible && element->getComputedStyle("display")  == "none")
			visible = false;
	if (visible && element->getComputedStyle("visibility")  == "hidden")
			visible = false;
	if (strcasecmp (tagname.c_str(), "input") == 0)
	{
		inputs.push_back(element);
		element->lock();
		if (!visible)
		{
			std::string soft;
			element->getProperty("soffidOnFocusTrigger", soft);
			if (soft != "true")
			{
				element->subscribe("focus", onHiddenElementFocusListener);
				element->setProperty("soffidOnFocusTrigger", "true");
			}
		}
	}
	else if (! first && strcasecmp (tagname.c_str(), "form") == 0)
	{
		// Do not go inside nested forms
	}
	else
	{
		std::vector<AbstractWebElement*> children;
		element->getChildren(children);
		for (std::vector<AbstractWebElement*>::iterator it = children.begin(); it != children.end(); it++)
		{
			AbstractWebElement *child = *it;
			std::string indent2 = indent;
			indent2 += "   ";
			findInputs (app, child, inputs, false, visible, indent2);
			child->release();
		}
	}
}

void SmartForm::findRootInputs (AbstractWebApplication* app, std::vector<AbstractWebElement*> &inputs)
{
	std::vector<AbstractWebElement*> inputs2;
	app->sanityCheck();
//	MZNSendDebugMessageA("APP = %s", app->toString().c_str());
	app->getElementsByTagName("input", inputs2 );
//	MZNSendDebugMessageA("Got %d elements", inputs2.size());
	for (std::vector<AbstractWebElement*>::iterator it = inputs2.begin () ; it!= inputs2.end(); it++)
	{
		AbstractWebElement *element = *it;
		bool visible = true;
		bool omit = false;
		AbstractWebElement *loopElement = element;
		loopElement->lock();
		do
		{
			std::string tagname;
			loopElement->getTagName(tagname);
			if (strcasecmp (tagname.c_str(), "form") == 0)
			{
				omit = true;
				loopElement->release();
				break;
			}
			else if (loopElement->getComputedStyle("display")  == "none" ||
					loopElement->getComputedStyle("visibility")  == "hidden")
			{
				visible = false;
			}
			AbstractWebElement *parent = loopElement->getParent();
			loopElement->release();
			loopElement = parent;
		} while (loopElement != NULL);
		if (!omit)
		{
			inputs.push_back(element);
			if( ! visible)
			{
				std::string soft;
				element->getProperty("soffidOnFocusTrigger", soft);
				if (soft != "true")
				{
					element->subscribe("focus", onHiddenElementFocusListener);
					element->setProperty("soffidOnFocusTrigger", "true");
				}
			}
		}
	}
	inputs2.clear();
}

static void setDisplayStyle (AbstractWebElement *element, const char* display)
{
	std::string style;
	element->getAttribute("style", style);
//	MZNSendDebugMessageA("Style was: %s", style.c_str());
	size_t i = style.find ("; display:");
	if (i != style.npos)
	{
		size_t j = style.find(";", i+1);
		if (j == style.npos)
		{
			style = style.substr(0, i);
		} else {
			style = style.substr(0, i) + style.substr(j);
		}
	}
	style += "; display: ";
	style += display;
//	MZNSendDebugMessageA("Style is: %s", style.c_str());
	element->setAttribute("style", style.c_str());

}


void SmartForm::updateIcon (InputDescriptor *input)
{
	std::string value;
	input->input->getProperty("value", value);

	if (status == SF_STATUS_LOCKED)
	{
		if ( value.empty())
		{
			input->status = IS_EMPTY;
			input->input->removeAttribute("readonly");
		}
		else
		{
			input->status = IS_LOCKED;
			input->input->setAttribute("readonly", "");
		}
	}
	else
	{
		input->input->removeAttribute("readonly");
		if (status == SF_STATUS_NEW)
			input -> status = value.empty() ? IS_EMPTY : IS_MODIFIED;
		else if (status == SF_STATUS_SELECT )
			input -> status = IS_SELECT;
		else if (status == SF_STATUS_GENERATING)
		{
			if ( input -> type == IT_NEW_PASSWORD)
			{
				input->status = IS_EMPTY;
			} else {
				input->input->setAttribute("readonly", "");
				input->status = IS_LOCKED;
			}
		}
		else if (status == SF_STATUS_MODIFYING)
		{
			std::string v;
			input->input->getProperty("value", v);
			if (v.empty() && page->accounts.size() == 0)
				input->status = IS_EMPTY;
			else if (v.empty() && page->accounts.size() > 0)
				input->status = IS_SELECT;
			else
				input->status = IS_MODIFIED;
		}
	}


	addIcon(input);


	if (input->img != NULL)
	{
		input->input->setProperty("soffidManaged", "true");
	}

	if (input->img == NULL)
	{
		// Nothing to do yet
	}
	else if (input->status == IS_EMPTY) // Unlocked state
	{
		if (input->type == IT_GENERAL)
			setDisplayStyle(input->img, "none");
		else
		{
			setDisplayStyle(input->img, "block");
			input->img->setAttribute("src", _img_resource_generate);
		}
	}
	else if (input->status == IS_LOCKED)
	{
		setDisplayStyle(input->img, "block");
		input->img->setAttribute("src", _img_resource_unlock);
	}
	else if (input->status == IS_SELECT)
	{
		setDisplayStyle(input->img, "block");
		if (input->type == IT_PASSWORD)
		{
			if (status == SF_STATUS_MODIFYING || page->accounts.size() == 0)
				input->img->setAttribute("src", _img_resource_generate);
			else
				input->img->setAttribute("src", _img_resource_password);
		}
		else if (input->type == IT_NEW_PASSWORD)
		{
			input->img->setAttribute("src", _img_resource_generate);
		}
		else
		{
			input->img->setAttribute("src", _img_resource_user);
		}
	}
	else // if (input->status == IS_MODIFIED)
	{
		setDisplayStyle(input->img, "block");
		input->img->setAttribute("src", _img_resource_save);
	}
}

void SmartForm::removeIcon (InputDescriptor *input)
{
	if (input->img != NULL)
	{
		AbstractWebElement *parent = input->img->getParent();
		if (parent != NULL)
		{
			parent->removeChild(input->img);
			parent->release();
			input->img->release();
			input->img = NULL;
		}
	}
	if (input->input != NULL)
		input->input->removeAttribute("readOnly");
}

void SmartForm::releaseElements ()
{
	for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		InputDescriptor *i = *it;
		if (i->img != NULL)
			i->img->release();
		if (i->input != NULL)
			i->input->release();
		delete i;
	}
	inputs.clear();
	numPasswords = 0;
}

static void getPosition (AbstractWebElement *input, long &left, long &top, long &width, long &height,
		long &width2, long &height2, bool global)
{
	height = getIntProperty (input, "offsetHeight");
	width = getIntProperty (input, "offsetWidth");
	left = getIntProperty (input, "offsetLeft");
	top= getIntProperty (input, "offsetTop");

	height2 = getIntProperty (input, "clientHeight"); // Without border
	width2 = getIntProperty (input, "clientWidth"); // Without border

	AbstractWebElement *parentOffset = input->getOffsetParent();
	while (parentOffset != NULL)
	{
		AbstractWebElement *pp = NULL;
		std::string tag;
		parentOffset->getTagName(tag);
		std::string position = parentOffset->getComputedStyle("position");
		if (position == "static" || global)
		{
			left += getIntProperty(parentOffset, "offsetLeft");
			top += getIntProperty (parentOffset, "offsetTop");
			pp = parentOffset->getOffsetParent();
		}
		parentOffset->release();
		parentOffset = pp;
	}

}


void SmartForm::addIcon (InputDescriptor *descriptor)
{
	AbstractWebElement *input = descriptor->input;
	if ( descriptor->img == NULL)
	{
		// Calculate position & size
		long left, top, width, height, width2, height2;
		getPosition (input, left, top, width, height, width2, height2, false);

		int borderH = (height - height2)/2;
		int borderW = (width - width2)/2;
		if (height <= 0) height = 20L;

		if (width > height * 3)
		{
			AbstractWebElement *img = app->createElement("img");
			if (img != NULL)
			{
				descriptor->img = img;
				char achStyle[200];
				int zIndex = 10;
				std::string zIndexStr = input->getComputedStyle("zIndex");
				if (zIndexStr != "")
				{
					sscanf (zIndexStr.c_str(), "%d", &zIndex);
					zIndex ++;
				} else {
					std::string zIndexStr = input->getComputedStyle("z-index");
					if (zIndexStr != "")
					{
						sscanf (zIndexStr.c_str(), "%d", &zIndex);
						zIndex ++;
					}
				}
				if (input->getComputedStyle("position") == "static" && input->getComputedStyle("float").size() == 0)
				{
					sprintf (achStyle, "margin-left: %ldpx; margin-top: %ldpx; "
							"position: absolute; height: %ldpx; width:%ldpx; "
							"z-index:%d",
							(long) (width-height2-4L-borderW), 1L + borderH, (long) height2 - 2L, (long) height2 - 2L,
							zIndex);
				}
				else
				{
					sprintf (achStyle, "left:%ldpx; top:%ldpx; margin-left: %ldpx; margin-top: %ldpx; "
							"position: absolute; height: %ldpx; width:%ldpx; "
							"z-index:%d",
							(long) left, (long) top,
							(long) (width-height2-4L-borderW), 1L + borderH, (long) height2 - 2L, (long) height2 - 2L,
							zIndex);
				}
				img->setAttribute("style", achStyle);
				char ach[50];
				sprintf (ach, "%ldpx", height);
				img->setProperty("width", ach);
				img->setProperty("height", ach);
				img->setAttribute("_soffid_handler", "true");
				img->setAttribute("_soffid_element", "true");
				std::string v;
				input->getProperty("value", v);
				AbstractWebElement *parent = input->getParent();
				if (parent != NULL)
				{
					parent->insertBefore(img, input);
					if (onChangeListener == NULL)
					{
						onChangeListener = new OnChangeListener();
						onChangeListener->form = this;
					}
					input->subscribe("input", onChangeListener);
					if (onClickListener == NULL)
					{
						onClickListener = new OnClickListener();
						onClickListener->form = this;
					}
					img->subscribe("click", onClickListener);
					parent->release();
				}
			}
		}
	}
}

bool SmartForm::addNoDuplicate (AbstractWebElement* input)
{
	for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		InputDescriptor *id = *it;
		if (id != NULL && id->input != NULL && id->input->equals(input))
		{
			return false;
		}
	}
	std::string type;
	input -> getAttribute("type", type);
	if (strcasecmp ( type.c_str(), "password") == 0)
	{
		input->lock();

		InputDescriptor *descriptor = new InputDescriptor();
		descriptor->img = NULL;
		descriptor->input = input;
		if (numPasswords == 0)
		{
			descriptor->type = IT_PASSWORD;
			numPasswords ++;
		}
		else if (numPasswords == 1)
		{
			descriptor->type = IT_NEW_PASSWORD;
			for (std::vector<InputDescriptor*>::iterator it2 = inputs.begin(); it2 != inputs.end(); it2++)
			{
				if ((*it2)->type == IT_PASSWORD)
					(*it2)->type = IT_NEW_PASSWORD;
			}
			numPasswords ++;
		}
		else if (numPasswords == 2)
		{
			descriptor->type = IT_NEW_PASSWORD;
			for (std::vector<InputDescriptor*>::iterator it2 = inputs.begin(); it2 != inputs.end(); it2++)
			{
				(*it2)->type = IT_PASSWORD;
				break;
			}
			numPasswords ++;
		}
		else {
			descriptor->type = IT_GENERAL;
		}
		descriptor->status = IS_EMPTY;
		inputs.push_back(descriptor);
	}
    // if (type == undefined || type.toLowerCase() == "text" || type.toLowerCase() == "input"  || type.toLowerCase() == "email" || type == "")
	else if (strcasecmp ( type.c_str(), "") == 0 ||
			strcasecmp ( type.c_str(), "text") == 0 ||
			strcasecmp ( type.c_str(), "input") == 0 ||
			strcasecmp ( type.c_str(), "email") == 0)
	{
		input->lock();
		InputDescriptor *descriptor = new InputDescriptor();
		descriptor->img = NULL;
		descriptor->input = input;
		descriptor->type = IT_GENERAL;
		descriptor->status = IS_EMPTY;
		inputs.push_back(descriptor);
	}
	else if (strcasecmp ( type.c_str(), "submit") == 0)
	{
		input->lock();
		submits.push_back(input);
	}

	return true;
}

bool SmartForm::checkAnyPassword (std::vector<AbstractWebElement*> &elements)
{
	bool anyPassword = false;
	int nonPassword = 0;
	int password = 0;
//	MZNSendDebugMessage("Analyzing form =============================");
	for (std::vector<AbstractWebElement*>::iterator it = elements.begin(); it != elements.end(); it++)
	{
		AbstractWebElement *input = *it;

		input->sanityCheck();

		std::string type;
		input -> getAttribute("type", type);
		if (strcasecmp ( type.c_str(), "password") == 0)
		{
			MZNSendDebugMessage("Found password element %s", input->toString().c_str());
			if (input->isVisible())
			{
				MZNSendDebugMessage("Found password element %s is visible (%s / %s)",
						input->toString().c_str(),
						input->getComputedStyle("display").c_str(),
						input->getComputedStyle("visibility").c_str());
				anyPassword = true;
				password ++;
			} else {
				// Test for any account on this URL
				MZNSendDebugMessage("Found password element %s is not visible", input->toString().c_str());
				for ( std::vector<AccountStruct>::iterator it = page->accounts.begin(); it != page->accounts.end(); it++)
				{
					AccountStruct as = *it;
					if (page->url == page->getAccountURL(as))
					{
						anyPassword = true;
						password ++;
						break;
					}
				}
			}
		} else if (strcasecmp ( type.c_str(), "date") == 0 ||
				strcasecmp ( type.c_str(), "") == 0 ||
				strcasecmp ( type.c_str(), "datetime") == 0 ||
				strcasecmp ( type.c_str(), "datetime-local") == 0 ||
				strcasecmp ( type.c_str(), "email") == 0 ||
				strcasecmp ( type.c_str(), "month") == 0 ||
				strcasecmp ( type.c_str(), "number") == 0 ||
				strcasecmp ( type.c_str(), "search") == 0 ||
				strcasecmp ( type.c_str(), "tel") == 0 ||
				strcasecmp ( type.c_str(), "text") == 0 ||
				strcasecmp ( type.c_str(), "time") == 0 ||
				strcasecmp ( type.c_str(), "url") == 0 ||
				strcasecmp ( type.c_str(), "week") == 0 ) {
			if (input->isVisible())
			{
//				std::string id;
//				std::string name;
//				input->getAttribute("name", name);
//				input->getAttribute("id", id);
//				MZNSendDebugMessage("Found input element %s (type %s) (name %s) (id %s)",
//						input->toString().c_str(), type.c_str(),
//						name.c_str(), id.c_str());
				nonPassword ++;
			}
		}
	}

	// Do nothing
	if (! anyPassword || nonPassword > 50)
	{
		if (! anyPassword)
		{
			MZNSendDebugMessage("* No password input found");
		}
		else
		{
			MZNSendDebugMessage("* More than 50 inputs found");
		}
		for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
		{
			removeIcon (*it);
		}
		releaseElements();
		for (std::vector<AbstractWebElement*>::iterator it = elements.begin(); it != elements.end(); it++)
		{
			AbstractWebElement *input = *it;

			input->sanityCheck();

			input->release();
		}
		return false;
	}
	else
	{
		MZNSendDebugMessage("* Found %d password inputs and %d non password inputs", password, nonPassword);
		return true;
	}
}

void SmartForm::parse(AbstractWebApplication *app, AbstractWebElement *formRoot) {
	std::vector<AbstractWebElement*> elements;
	if (this->element != NULL)
		this->element->release();
	this->element = formRoot;
	if (this->element != NULL)
		this->element -> lock();
	if (this->app != NULL)
		this->app->release();
	this->app = app;
	this->app->lock();

	if (formRoot == NULL)
	{
		MZNSendDebugMessage("Parsing formless inputs");
		findRootInputs(app, elements);
		MZNSendDebugMessage("Parsed formless inputs");
	}
	else
	{
		MZNSendDebugMessage("Parsing inputs on %s", formRoot->toString().c_str());
		findInputs(app, formRoot, elements, true, formRoot->isVisible(), std::string());
	}

	releaseElements();


	if (! checkAnyPassword(elements))
		return;

	parsed = true;

	if (page->accounts.size () == 0)
		status = SF_STATUS_NEW;
	else
		status = SF_STATUS_SELECT;


	for (std::vector<AbstractWebElement*>::iterator it = elements.begin(); it != elements.end(); it++)
	{
		AbstractWebElement *input = *it;

		if (input->isVisible())
		{
			input->sanityCheck();

			addNoDuplicate(input);

			input->release();
		}
	}

	for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		updateIcon (*it);
	}

	if (onBeforeUnloadListener == NULL)
	{
		onBeforeUnloadListener = new OnBeforeUnloadListener();
		onBeforeUnloadListener->form = this;
//		app->subscribe("beforeunload", onBeforeUnloadListener);
	}


	if (page->accounts.size() == 1)
	{
		AccountStruct as = page->accounts[0];
		fetchAttributes(as, NULL);
	}
}

void SmartForm::reparse() {
	std::vector<AbstractWebElement*> elements;
	if (this->element == NULL)
	{
		MZNSendDebugMessageA("Reparsing root element ...");
		findRootInputs(app, elements);
	}
	else
		findInputs(app, this->element, elements, true, true, std::string());
	bool newItem = false;

	if (! checkAnyPassword(elements))
		return;

	for (std::vector<AbstractWebElement*>::iterator it = elements.begin(); it != elements.end(); it++)
	{
		AbstractWebElement *input = *it;
		input->sanityCheck();
		if (addNoDuplicate (input))
			newItem = true;
		input->release();
	}

	if (numPasswords > 0)
	{
		for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
		{
			updateIcon (*it);
		}
	}

	if (page->accounts.size() == 1 && status == SF_STATUS_NEW)
	{
		AccountStruct as = page->accounts[0];
		fetchAttributes(as, NULL);
	} else if (page->accounts.size() > 1  && status == SF_STATUS_NEW)
	{
		changeStatus(SF_STATUS_SELECT);
	} else if (newItem && status == SF_STATUS_LOCKED)
	{
		MZNSendDebugMessageA("New item added on locked form");
		fetchAttributes(currentAccount, NULL);
	}
}


InputDescriptor* SmartForm::findInputDescriptor (AbstractWebElement *element)
{
	for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		InputDescriptor *d = *it;
		if ((d->input != NULL && d->input->equals(element)) ||
				(d->img != NULL && d->img->equals(element)))
			return d;
	}
	return NULL;
}

void SmartForm::onChange(AbstractWebElement* element) {
	InputDescriptor *input = findInputDescriptor(element);
	std::string value;

	if (input == NULL)
		return;

	page->sanityCheck();
	element->sanityCheck();
	element->getProperty("value", value);
	if (this->status == SF_STATUS_NEW)
	{
		if (!value.empty())
		{
			changeStatus (SF_STATUS_MODIFYING);
		}
	}
	else if (this->status == SF_STATUS_SELECT)
	{
		if (!value.empty())
		{
			changeStatus (SF_STATUS_MODIFYING);
		}
	}
	else if (this->status == SF_STATUS_GENERATING)
	{
		if (!value.empty())
		{
			changeStatus (SF_STATUS_MODIFYING);
		}
	}
	else if (status == SF_STATUS_MODIFYING)
	{
		if (input->type == IT_NEW_PASSWORD)
		{
			if (value.empty())
				changeStatus (SF_STATUS_GENERATING);
		}
		else
		{
			input->status = IS_MODIFIED;
			updateIcon(input);
		}
	}
}

static std::string toLowerCase (const char* src)
{
	std::wstring wstr = MZNC_strtowstr(src);
	std::wstring result;
	for (int i = 0; i< wstr.length(); i++)
	{
		wchar_t w = wstr[i];
		wchar_t w2 = tolower(w);
		result.append(&w2, 1);
	}
	return MZNC_wstrtostr(result.c_str());
}

void SmartForm::onClickImage(AbstractWebElement* element) {
	page->sanityCheck();
	element->sanityCheck();
	std::string type, value, tag;
	InputDescriptor *descr = findInputDescriptor(element);
	if (descr == NULL)
		return;

	if (descr->status == IS_LOCKED)
	{
		changeStatus(SF_STATUS_SELECT);
	}
	else if (descr->type == IT_PASSWORD && descr->status == IS_SELECT)
	{
		if (status == SF_STATUS_MODIFYING || page->accounts.size() == 0)
			createGenerateModal(element);
		else
			createModal(element);
	}
	else if (descr->type == IT_PASSWORD && descr->status == IS_EMPTY)
	{
		createGenerateModal(element);
	}
	else if (descr->type == IT_NEW_PASSWORD && (descr->status == IS_EMPTY || descr->status == IS_SELECT))
	{
		createGenerateModal(element);
	}
	else if (descr->status == IS_SELECT)
	{
		if (page->accounts.size() > 0)
		{
			createModal(element);
		}
	}
	else if (descr->status == IS_MODIFIED)
	{
		if (currentAccount.account.empty() && page->accounts.size() == 0)
			save ();
		else //if (detectAttributeChange())
			createSaveModal(element);
	}
	else if (this->status == SF_STATUS_GENERATING)
	{
	}
}

void SmartForm::save ()
{
	MZNSendDebugMessageA("Sending password and attributes to server");
	std::map<std::string,std::string> attributes;
	std::string msg;
	std::string description;
	for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		if ((*it)->type == IT_GENERAL)
		{
			AbstractWebElement *element = (*it)->input;
			element->sanityCheck();
			std::string value;
			std::string name;
			std::string id;
			element->getProperty("value", value);
			element->getAttribute("name", name);
			element->getAttribute("id", id);
			if (name.empty())
				name = id;
			if (name.empty())
				element->getAttribute("data-bind", name);

			if (!name.empty())
			{
				if (attributes.find(name) != attributes.end() ||
						!value.empty())
					attributes[name] = value;
				description.append (value).append(" ");
			}
		}
	}

	if (currentAccount.account.empty())
	{
		std::string msg;
		if (description.length() > 100)
			description = description.substr(0, 100);
		if ( ! page->createAccount(description.c_str(), msg, currentAccount) )
		{
			this->app->alert(MZNC_strtoutf8(msg.c_str()).c_str());
			return;
		}
	}
	for (std::vector<InputDescriptor*>::iterator it = inputs.begin (); it != inputs.end(); it++)
	{
		InputDescriptor *input = *it;
		if (input->type == IT_PASSWORD)
		{
			std::string value;
			input->input->getProperty("value", value);
			if (!value.empty () && !this->page->updatePassword (currentAccount, value, msg))
			{
				this->app->alert(MZNC_strtoutf8(msg.c_str()).c_str());
				return;
			}
		}
	}

	if (this->page->updateAttributes (currentAccount, attributes, msg))
		changeStatus(SF_STATUS_LOCKED);
	else
		this->app->alert(MZNC_strtoutf8(msg.c_str()).c_str());

}


void SmartForm::onClickModal(AbstractWebElement* element) {
	page->sanityCheck();
	std::string id = stylePrefix+"_id";
	AbstractWebElement *masterDiv = app->getElementById(id.c_str());
	if (masterDiv != NULL)
	{
		AbstractWebElement *parent = masterDiv->getParent();
		if (parent != NULL)
		{
			parent->removeChild(masterDiv);
			parent->release();
		}
		masterDiv->release();
	}
	id =  (stylePrefix+"_modal");
	AbstractWebElement *modal = app->getElementById(id.c_str());
	if (modal != NULL)
	{
		AbstractWebElement *parent = modal->getParent();
		if (parent != NULL)
		{
			parent->removeChild(modal);
			parent->release();
		}
		modal->release();
	}
}

void SmartForm::onClickLevel(AbstractWebElement* element) {
	std::string level;
	element->getAttribute("_soffid_level", level);
	if (! level.empty())
	{
		std::string type, value, tag;
		std::string id = stylePrefix+"_id";
		AbstractWebElement *masterDiv = app->getElementById(id.c_str());
		if (masterDiv != NULL)
		{
			InputDescriptor *descriptor = inputs[currentModalInput];
			if (descriptor != NULL)
			{

				int length;
				bool mays = false;
				bool mins = false;
				bool numbers = false;
				bool simbols = false;
				if (level == "high")
				{
					length = 24;
					mays = mins = numbers = simbols = true;
				}
				else if (level == "medium")
				{
					length = 16;
					mays = mins = numbers = true;
				}
				else
				{
					length = 8;
					mins = numbers = true;
				}
				std::string newPass;
				static bool initialized = false;
				if ( ! initialized)
				{
					time_t t;
					time(&t);
					srand (t);
					initialized = true;
				}
				while (newPass.length() != length)
				{
					int ch = rand() % (26 + 26 + 10 + 15);
					if (ch < 15 && simbols)
						newPass.push_back( '!' + ch  ) ;
					if (ch >= 15 && ch < 25 && numbers)
						newPass.push_back( '0' - 15 + ch  ) ;
					if (ch >= 25 && ch < 51 && mays)
						newPass.push_back( 'A' - 25 + ch  ) ;
					if (ch >= 51 && ch < 77 && mins)
						newPass.push_back( 'a' - 51 + ch  ) ;
				}
				bool found = false;
				for ( std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
				{
					InputDescriptor *id = *it;
					if (id -> type == descriptor -> type)
					{
						id -> input -> focus();
						id -> input -> setProperty("value", newPass.c_str());
					}
				}
			}
		}
		onClickModal(NULL);
		changeStatus(SF_STATUS_MODIFYING);
	}
}

void SmartForm::onClickSave(AbstractWebElement* element) {
	std::string action;
	element->getAttribute("_soffid_save", action);
	currentAccount.id = "";
	currentAccount.friendlyName = L"";
	currentAccount.system = L"";
	currentAccount.account = L"";
	for (std::vector<AccountStruct>::iterator it = page->accounts.begin(); it != page->accounts.end(); it++)
	{
		if (action == it->id)
		{
			currentAccount = *it;
		}
	}

	onClickModal(element);
	save ();

}


void SmartForm::onClickAccount(AbstractWebElement* element) {
	AbstractWebElement *input = NULL;
	std::string id;
	AccountStruct as;

	page->sanityCheck();
	element->sanityCheck();

	if (currentModalInput >= 0 && currentModalInput < inputs.size())
	{
		InputDescriptor *descriptor = inputs[currentModalInput];
		if (descriptor != NULL && descriptor->input != NULL)
			input = descriptor->input;
	}

	element->getAttribute("_soffid_account", id);
	page->getAccountStruct(id.c_str(), as);

	onClickModal (element);

	fetchAttributes (as, input);
}

void SmartForm::fetchAttributes(AccountStruct &as, AbstractWebElement *selectedElement)
{
	currentAccount = as;
	std::map<std::string,std::string> attributes;

	page->fetchAttributes(app, as, attributes);

	bool first = true;
	for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		InputDescriptor *descr = *it;
		AbstractWebElement *e = descr->input;
		e->sanityCheck();
		if (descr->type == IT_GENERAL && e->isVisible())
		{
			std::string currentValue;
			std::string name;
			std::string id;
			e->getAttribute("name", name);
			if (name.empty())
				e->getAttribute("id", name);
			if (name.empty())
				e->getAttribute("data-bind", name);
			e->getProperty("value", currentValue);
			if (attributes.find(name) != attributes.end())
			{
				std::string v2 = MZNC_strtoutf8(attributes[name].c_str());
				if (v2 != currentValue)
				{
					e->focus();
					e->setProperty("value", v2.c_str());
//					MZNSendDebugMessageA("Setting value of %s=%s", name.c_str(), v2.c_str());
				}
			}
			else if (first && (selectedElement == NULL || selectedElement->equals(e)))
			{
				first = false;
				std::string ssoSystem;
				SeyconCommon::readProperty("AutoSSOSystem", ssoSystem);
				if (currentAccount.system != MZNC_strtowstr(ssoSystem.c_str()))
				{
					std::string value = MZNC_wstrtoutf8(currentAccount.account.c_str());
					if (value != currentValue)
					{
						e->focus();
						e->setProperty("value", value.c_str());
//						MZNSendDebugMessageA("Setting value of %s=%s", name.c_str(), value.c_str());
					}
				}
				else
				{
					for (std::map<std::string,std::string>::iterator it = attributes.begin(); it != attributes.end(); it++)
					{
						std::string name = toLowerCase(it->first.c_str());
						if ( name.find("user") != name.npos || name.find("login") != name.npos)
						{
							std::string value = MZNC_strtoutf8(it->second.c_str());
							if (value != currentValue)
							{
								e->focus();
								e->setProperty("value", value.c_str());
//								MZNSendDebugMessageA("Setting value of %s=%s", name.c_str(), value.c_str());
							}
						}
					}
				}
			} else {
//				MZNSendDebugMessageA("Cannot find value for %s", name.c_str());
			}
		}
		else if (e->isVisible() &&
					(descr->type == IT_PASSWORD ||
					 descr->type == IT_NEW_PASSWORD && selectedElement != NULL && e->equals(selectedElement) ))
		{
			SecretStore s(MZNC_getUserName());
			std::wstring secret = L"pass.";
			secret += as.system;
			secret += L".";
			secret += as.account;
			wchar_t *pass = s.getSecret(secret.c_str());
			if (pass != NULL)
			{
				e->focus();
				e->setProperty("value", MZNC_wstrtoutf8(pass).c_str());
				s.freeSecret(pass);
			}
			wchar_t *sessionKey = s.getSecret(L"sessionKey");
			wchar_t *user = s.getSecret(L"user");
			SeyconService ss;
			SeyconResponse *response = ss.sendUrlMessage(L"/auditPassword?user=%ls&key=%ls&system=%ls&account=%ls&application=%ls",
					user, sessionKey, as.system.c_str(), as.account.c_str(),
					ss.escapeString(MZNC_getCommandLine()).c_str());
			if (response != NULL)
				delete response;
			if (descr->type == IT_NEW_PASSWORD)
			{
				descr->type = IT_PASSWORD;
				updateIcon(descr);
			}
		}
	}

	if (numPasswords < 2)
		changeStatus(SF_STATUS_LOCKED);
	else
		changeStatus(SF_STATUS_GENERATING);
}

bool SmartForm::detectAttributeChange()
{
	return true;

	std::map<std::string,std::string> attributes;
	page->fetchAttributes(app, currentAccount, attributes);

	for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		InputDescriptor *descr = *it;
		AbstractWebElement *e = descr->input;
		e->sanityCheck();
		std::string name;
		std::string id;
		e->getAttribute("name", name);
		if (name.empty())
			e->getAttribute("id", name);
		if (name.empty())
			e->getAttribute("data-bind", name);
		std::string value;
		if (attributes.find(name) != attributes.end())
		{
			std::string value = MZNC_strtoutf8(attributes[name].c_str());
		}
		std::string currentValue;
		e->getProperty("value", currentValue);
		if (value != currentValue)
			return true;
	}
	return false;
}

void SmartForm::changeStatus (int status)
{
	this->status = status;
//	MZNSendDebugMessage("Updating icons (status = %d)", status);

	for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		InputDescriptor *id = *it;
		updateIcon (id);
	}
}

void SmartForm::onBeforeUnload ()
{
}

static const char *STYLE_MASTER = "z-index: 1000000001; text-align:left; border: solid white 2px; "
		"display: inline-block; left:%ldpx; top: %ldpx; position: fixed; min-width: 200px; "
		"max-height: 10em; overflow-y: auto; "
		"box-shadow: 10px 10px 4px #888888; background-color: white; "
		"font-family: Verdana, sans-serif; font-size: 1em; ";
static const char *STYLE_MODAL = "position: fixed;"
	   	   "top:0; right: 0 ; bottom: 0; left:0;"
	   	   "background:black;"
	   	   "z-index: 1000000000;"
	   	   "opacity: 0.3;"
	   	   "pointer-events: auto;"
	   	   "filter: progid:DXImageTransform.Microsoft.Alpha(Opacity=30);"
           "-ms-filter: progid:DXImageTransform.Microsoft.Alpha(Opacity=30);";
static const char *STYLE_SELECTOR_HOVER = "_selector:hover {background-color: #a6d100;}\n";
static const char *STYLE_SELECTOR = "_selector { "
		"background-color: #5f7993; "
		"color: white; "
		"cursor: pointer; "
		"padding-left:5px; "
		"padding-right: 5px;"
  	  	"padding-top:3px; "
  	  	"padding-bottom: 3px;}\n";
static const char *STYLE_HEADER= "_header {"
	   	"background-color: #17b5c8; "
		"color: black; "
	   	"padding-left:5px; "
	   	"padding-right: 5px;"
	   	"padding-top:3px; "
	   	"padding-bottom: 3px;}\n";

static AbstractWebElement* getBody (AbstractWebApplication *app) {
	AbstractWebElement *body = NULL;
	std::vector<AbstractWebElement *> bodies;
	app->getElementsByTagName("body", bodies);
	for (std::vector<AbstractWebElement*>::iterator it = bodies.begin(); it != bodies.end (); it++)
	{
		if (body == NULL)
			body = *it;
		else
			(*it)->release();
	}
	if (body == NULL)
		body = app->getDocumentElement();
	return body;
}

void SmartForm::createStyle () {

	if( app == NULL)
		return;

	std::string styleId = std::string(stylePrefix)+"_style";

	AbstractWebElement *style = app->getElementById(styleId.c_str());
	if (style == NULL)
	{
		style = app->createElement("style");
		style->setAttribute("type", "text/css");
		std::string styleText = (std::string()+
				"." + stylePrefix+ STYLE_SELECTOR_HOVER +
				"." + stylePrefix+ STYLE_SELECTOR +
				"." + stylePrefix+ STYLE_HEADER);
		style->setTextContent(styleText.c_str());
		style->setAttribute("id", styleId.c_str());

		std::vector<AbstractWebElement*> heads;
		app->getElementsByTagName("head", heads);
		AbstractWebElement * head = NULL;
		if (!heads.empty())
		{
			head = *heads.begin();
			head->lock();
			for (std::vector<AbstractWebElement*>:: iterator it = heads.begin(); it != heads.end(); it++)
			{
				(*it)->release();
			}
		} else {
			head = app->createElement("head");
			AbstractWebElement *root = app->getDocumentElement();
			if (root != NULL)
			{
				std::vector<AbstractWebElement*> children;
				root->getChildren(children);
				if (children.empty())
					root->appendChild(head);
				else
					root->appendChild(*(children.begin()));
				for (std::vector<AbstractWebElement*>:: iterator it = children.begin(); it != heads.end(); it++)
					(*it)->release();
				root-> release();
			}
		}
		head->appendChild(style);

		if (head != NULL)
			head->release();
	}
	if (style != NULL) style-> release();
}



AbstractWebElement* SmartForm::createModalDialog (AbstractWebElement *input)
{
	std::string id = stylePrefix+"_id";
	onClickModal(input);

	AbstractWebElement *masterDiv = app->createElement("div");
	if (masterDiv == NULL)
	{
		MZNSendDebugMessageA("Cannot create master div");
		return NULL;
	}

	masterDiv->setAttribute("_soffid_element", "true");

	masterDiv->setAttribute("_soffid_modal", "true");
	char ach[1000];

	long left = 0, top = 0, width = 0, height = 0, width2 = 0, height2 = 0;
	getPosition (input, left, top, width, height, width2, height2, true);

	AbstractWebElement *html = app->getDocumentElement();
	left -= getIntProperty(html, "scrollLeft");
	top -= getIntProperty(html, "scrollTop");

	int dheight = getIntProperty (html, "clientHeight"); // Without border
	int dwidth = getIntProperty (html, "clientWidth"); // Without border

	html->release();

	AbstractWebElement *body = getBody(app);
	left -= getIntProperty(body, "scrollLeft");
	top -= getIntProperty(body, "scrollTop");

	// Adjust to visible space
	int x = left+width-200+2;
	MZNSendDebugMessage("x=%d dwidth=%d", x, dwidth);
	if (x+200 > dwidth) x = dwidth - 200;
	if (x < 0) x = 0;
	MZNSendDebugMessage("x=%d dwidth=%d", x, dwidth);

	int y = top+height;
	MZNSendDebugMessage("y=%d dheight=%d", y, dheight);
	if (y > dheight - 10) y = dheight - 10;
	if (y < 0) y = 0;
	MZNSendDebugMessage("y=%d dheight=%d", y, dheight);

	sprintf (ach, STYLE_MASTER, x, y);

	masterDiv->setAttribute("style", ach);
	masterDiv->setAttribute("id", id.c_str());
	body->appendChild(masterDiv);

	createStyle();

	AbstractWebElement *modal = app->createElement("div");
	if (modal != NULL)
	{
		std::string id = stylePrefix+"_modal";
		modal->setAttribute("_soffid_element", "true");
		modal->setAttribute("style", STYLE_MODAL);
		modal->setAttribute("id", id.c_str());
		modal->setAttribute("_soffid_modal", "true");
		masterDiv->appendChild(modal);
		modal->subscribe("click", onClickListener);
		body->appendChild(modal);
		modal->release();
	}

	body->release();

	currentModalInput = 0;
	for (unsigned int i = 0; i < inputs.size(); i++)
	{
		if (inputs[i]->img != NULL && inputs[i]->img->equals(input) ||
				inputs[i]->input != NULL && inputs[i]->input->equals(input))
		{
			currentModalInput = i;
		}
	}

	return masterDiv;

}

void SmartForm::createModal(AbstractWebElement *img)
{
	std::string link;

	SeyconCommon::readProperty("AutoSSOURL", link);

	AbstractWebApplication *app = img->getApplication();
	AbstractWebElement *parent = img->getParent();
	if (parent != NULL)
	{
		AbstractWebElement *input = img->getNextSibling();
		if ( input == NULL)
		{
			MZNSendDebugMessageA("Cannot find input element");
			return;
		}

		AbstractWebElement *masterDiv = createModalDialog(input);
		if (masterDiv == NULL)
			return;
		AbstractWebElement *user = app->createElement("div");
		if (user == NULL)
			return;
		user->setTextContent("Select account");
		user->setAttribute("class", (stylePrefix+ "_header").c_str());
		masterDiv->appendChild(user);
		user->release();
		for (std::vector<AccountStruct>::iterator it = page->accounts.begin(); it != page->accounts.end(); it++)
		{
			AccountStruct as = *it;
			std::string friendlyName = MZNC_wstrtoutf8(as.friendlyName.c_str());
			AbstractWebElement *user = app->createElement("div");
			if (user == NULL) return;
			user->setAttribute("class", (stylePrefix+ "_selector").c_str());
			user->setAttribute("_soffid_account", as.id.c_str());
			user->setTextContent(friendlyName.c_str());
			masterDiv->appendChild(user);
			if (! link.empty())
			{
				AbstractWebElement *linkAnchor = app->createElement("a");
				linkAnchor->setAttribute("target", "_blank");
				std::string url = link;
				url += "/selfservice/index.zul?target=sharedAccounts/sharedAccounts.zul?account=";
				url += SeyconCommon::urlEncode(as.account.c_str());
				url += "&system=";
				url += SeyconCommon::urlEncode(as.system.c_str());
				linkAnchor->setAttribute("href", url.c_str());
				linkAnchor->setAttribute("style", "float: right;");
				user->appendChild(linkAnchor);
				AbstractWebElement *linkImg = app->createElement("img");
				linkImg->setAttribute("src", _img_resource_link);
				linkAnchor->appendChild(linkImg);
				linkImg->release();
				linkAnchor->release();
			}
			user->subscribe("click", onClickListener);
			user->release();
		}

		masterDiv->release();
		input->release();
		parent->release();
	}
}

void SmartForm::createGenerateModal(AbstractWebElement *img)
{
	AbstractWebApplication *app = img->getApplication();
	AbstractWebElement *parent = img->getParent();
	if (parent != NULL)
	{
		std::string id = stylePrefix+"_id";
		AbstractWebElement *input = img->getNextSibling();
		if (input == NULL)
			return;

		AbstractWebElement *masterDiv = createModalDialog(input);
		if (masterDiv == NULL)
			return;

		AbstractWebElement *user = app->createElement("div");
		if (user == NULL) return;

		user->setAttribute("class", (stylePrefix+ "_header").c_str());
		user->setTextContent("Generate new password");
		masterDiv->appendChild(user);
		user->release();
		static const char *levels[] = {"high", "medium", "low"} ;
		static const char *levelDescription[] = {"High security", "Medium security", "Low security"} ;
		for (int i= 0; i < 3; i++)
		{
			AbstractWebElement *user = app->createElement("div");
			if (user == NULL) return;
			user->setAttribute("class", (stylePrefix+ "_selector").c_str());
			user->setAttribute("_soffid_level", levels[i]);
			user->setTextContent(levelDescription[i]);
			masterDiv->appendChild(user);
			user->subscribe("click", onClickListener);
			user->release();
		}

		// ---------------- Save current account
		if (!currentAccount.id.empty())
		{
			user = app->createElement("div");
			if (user == NULL) return;
			user->setAttribute("class", (stylePrefix+ "_selector").c_str());
			user->setAttribute("_soffid_account", currentAccount.id.c_str());
			user->setTextContent(MZNC_wstrtoutf8( (std::wstring(L"Get password for ")+currentAccount.friendlyName).c_str() ).c_str());
			masterDiv->appendChild(user);
			user->subscribe("click", onClickListener);
			user->release();
		}

		masterDiv->release();
		parent->release();
		input->release();
	}
}



void SmartForm::createSaveModal(AbstractWebElement *img)
{
	MZNSendDebugMessageA("Creating save modal");
	AbstractWebApplication *app = img->getApplication();
	AbstractWebElement *parent = img->getParent();
	if (parent != NULL)
	{
		std::string id = stylePrefix+"_id";
		AbstractWebElement *input = img->getNextSibling();
		if (input == NULL)
		{
			MZNSendDebugMessageA("Unable to find input element");
			return;
		}

		AbstractWebElement *masterDiv = createModalDialog(input);
		if (masterDiv == NULL)
		{
			MZNSendDebugMessageA("ERROR: No Master div created");
			return;
		}

		AbstractWebElement *user = app->createElement("div");
		if (user == NULL) {
			MZNSendDebugMessageA("Unable to create user element");
			return;
		}
		user->setAttribute("class", (stylePrefix+ "_header").c_str());
		user->setTextContent("Save identity");
		masterDiv->appendChild(user);
		user->release();

		// ---------------- Save current account
		if (!currentAccount.id.empty())
		{
			user = app->createElement("div");
			if (user == NULL) return;
			user->setAttribute("class", (stylePrefix+ "_selector").c_str());
			user->setAttribute("_soffid_save", currentAccount.id.c_str());
			user->setTextContent(MZNC_wstrtoutf8( (std::wstring(L"Update identity ")+currentAccount.friendlyName).c_str() ).c_str());
			masterDiv->appendChild(user);
			user->subscribe("click", onClickListener);
			user->release();
		}

		// ---------------- Create new account
		user = app->createElement("div");
		if (user == NULL) return;
		user->setAttribute("class", (stylePrefix+ "_selector").c_str());
		user->setAttribute("_soffid_save", "-");
		user->setTextContent("As new identity");
		masterDiv->appendChild(user);
		user->subscribe("click", onClickListener);
		user->release();

		if (currentAccount.id.empty())
		{
			for (std::vector<AccountStruct>::iterator it = page->accounts.begin(); it != page->accounts.end(); it++)
			{
				AccountStruct s = *it;
				user = app->createElement("div");
				if (user == NULL) return;
				user->setAttribute("class", (stylePrefix+ "_selector").c_str());
				user->setAttribute("_soffid_save", s.id.c_str());
				user->setTextContent(MZNC_wstrtoutf8( (std::wstring(L"Update identity ")+s.friendlyName).c_str() ).c_str());
				masterDiv->appendChild(user);
				user->subscribe("click", onClickListener);
				user->release();
			}
		}


		masterDiv->release();
		parent->release();
		input->release();
	}
}

std::string SmartForm::toString() {
	std::string s = "SmartForm ";
	if (element != NULL)
	{
		element->sanityCheck();
		s += element -> toString();
	} else {
		s += "Root";
	}
	return s;
}
