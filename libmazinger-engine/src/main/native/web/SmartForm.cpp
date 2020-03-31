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
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component, const char *data);

};

class OnClickListener: public WebListener
{
public:
	virtual std::string toString () { return std::string("OnClickListener");}
	SmartForm *form;
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component, const char *data);

};

class OnBeforeUnloadListener: public WebListener
{
public:
	virtual std::string toString () { return std::string("OnBeforeUnloadListener");}
	SmartForm *form;
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component, const char *data);

};

class OnHiddenElementFocusListener: public WebListener
{
public:
	virtual std::string toString () { return std::string("OnHiddenElementFocusListener");}
	SmartForm *form;
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component, const char *data);

};


void OnChangeListener::onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component, const char *data) {
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

void OnClickListener::onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component, const char *data) {
	if (MZNC_waitMutex())
	{
		std::string managed = "";
		std::string tag = "";
		if (component != NULL)
		{
			component->getTagName(tag);
			component->sanityCheck();
			//		MZNSendDebugMessageA("On click event %s - %s", tag.c_str(), managed.c_str() );

		}
		app->sanityCheck();
		if (form != NULL)
		{
			form->sanityCheck();
			std::string id;
			if (data != NULL && data[0] != '\0')
			{
				if (data [0] == 'a')
					form -> onClickAccount(component, &data[1]);
				else if (data [0] == 'l')
					form -> onClickLevel(component, &data[1]);
				else
					form -> onClickSave(component, &data[1]);
			}
			else
			{
				component->getAttribute("_soffid_handler", id);
				if (id == "true")
					form -> onClickImage(component);
			}
		}
//		MZNSendDebugMessageA("END On click event %s - %s", tag.c_str(), managed.c_str() );
		MZNC_endMutex();
	}
}

void OnBeforeUnloadListener::onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component, const char *data) {
	if (MZNC_waitMutex())
	{
		app->sanityCheck();
		form->sanityCheck();
		form->onBeforeUnload();
		MZNC_endMutex();
	}
}

void OnHiddenElementFocusListener::onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component, const char *data) {
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
				form->reparse(NULL);
			}
			else
			{
				form->onFocus (component);
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
	this->lockedOnce = false;
	this->lastElement = NULL;
	status = SF_STATUS_NEW;
	char ach[100];
	sprintf (ach, "_soffid_%ld_id_", (long) (long long) this);
	stylePrefix = ach;
	numPasswords = 0;
	parsed = false;
	formlessMode = false;
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


void SmartForm::setFormless(){
	if (!formlessMode)
	{
		formlessMode = true;
		parsed = false;
	}
}

static long getIntProperty (AbstractWebElement *element, const char* property)
{
	std::string s;
	element->getProperty(property, s);
	long l = 0;
	sscanf (s.c_str(), " %ld", &l);
	return l;
}


static bool checkFormMembership (AbstractWebElement *loopElement, AbstractWebElement*form, std::map<std::string, bool> &visibility)
{
	bool result;
	std::string id = loopElement->toString();
	std::map<std::string, bool>::iterator it = visibility.find( id );
	if (it == visibility.end() )
	{
		std::string tagname;
		loopElement->getTagName(tagname);
		if (strcasecmp (tagname.c_str(), "form") == 0)
		{
			MZNSendDebugMessageA("Found form %s/%s", loopElement->toString().c_str(), (form == NULL ? "ROOT": form->toString().c_str()));
			result = form != NULL && form->equals(loopElement) ? true: false;
		}
		else
		{
			AbstractWebElement *parent = loopElement->getParent();
			if (parent == NULL)
			{
				MZNSendDebugMessageA("Found ROOT/%s", (form == NULL ? "ROOT": form->toString().c_str()));
				result = form == NULL ? true : false;
			}
			else
			{
				result = checkFormMembership(parent, form, visibility);
				parent->release();
			}
		}
		visibility[id] = result;
	}
	else
	{
		result = it->second;
	}
	return result;
}


void SmartForm::findInputs (AbstractWebApplication* app, AbstractWebElement *form, std::vector<InputDescriptor*> &inputs,
		bool first, bool visible, std::string indent)
{
	std::vector<AbstractWebElement*> inputs2;
	app->sanityCheck();
	std::map<std::string, bool> visibility;
	MZNSendDebugMessageA("Finding form inputs");
	app->getElementsByTagName("input", inputs2 );
	for (std::vector<AbstractWebElement*>::iterator it = inputs2.begin () ; it!= inputs2.end(); it++)
	{
		AbstractWebElement *element = *it;
		MZNSendDebugMessageA("Checking element %s", element->toString().c_str());
		bool member = formlessMode || checkFormMembership(element, form, visibility);
		MZNSendDebugMessage ("                 %s status = %d", element->toString().c_str(), (int) member);
		if (member)
		{
			bool visible = element->isVisible();
			MZNSendDebugMessageA("Adding element %s", element->toString().c_str());
			InputDescriptor *id = new InputDescriptor();
			id->hasInputData = false;
			id->input = element;
			inputs.push_back(id);
			if( ! visible || id->getClientHeight() < 8)
			{
				std::string soft;
				element->getProperty("soffidOnFocusTrigger", soft);
				if (soft != "true")
				{
					if (soft != "false")
						element->subscribe("focus", onHiddenElementFocusListener);
					element->setProperty("soffidOnFocusTrigger", "true");
				}
			}
			else
			{
				std::string soft;
				element->getProperty("soffidOnFocusTrigger", soft);
				if (soft == "true")
				{
					element->setProperty("soffidOnFocusTrigger", "false");
				}
			}
		} else {
			element->release();
		}
	}
	inputs2.clear();
}

void SmartForm::findInputs (std::vector<InputData> *inputDatae, std::vector<InputDescriptor*> &inputs)
{
	for (std::vector<InputData>::iterator it = inputDatae->begin();
			it != inputDatae->end();
			it ++)
	{
		InputData inputData = *it;

		InputDescriptor *id = new InputDescriptor();

		id->hasInputData = true;
		id->data = inputData;
		id->input = app->getElementBySoffidId(id->data.soffidId.c_str());
		if (id->input != NULL)
		{
			inputs.push_back(id);
			if( id->getDisplay() == "none" || id->getVisibility() == "hidden" || id->getClientHeight() < 8)
			{
				std::string soft;
				id->input->getProperty("soffidOnFocusTrigger", soft);
				if (soft != "true")
				{
					if (soft != "false")
						id->input->subscribe("focus", onHiddenElementFocusListener);
					id->input->setProperty("soffidOnFocusTrigger", "true");
				}
			}
			else
			{
				std::string soft;
				id->input->getProperty("soffidOnFocusTrigger", soft);
				if (soft == "true")
				{
					id->input->setProperty("soffidOnFocusTrigger", "false");
				}
			}
		}
	}
}

void SmartForm::findInputs (AbstractWebApplication* app, std::vector<InputDescriptor*> &inputs)
{
	std::vector<AbstractWebElement*> inputs2;
	app->sanityCheck();
	app->getElementsByTagName("input", inputs2 );
	MZNSendDebugMessage("Looking for input elements");

	std::map<std::string, bool> visibility;

	for (std::vector<AbstractWebElement*>::iterator it = inputs2.begin () ; it!= inputs2.end(); it++)
	{
		AbstractWebElement *element = *it;
		AbstractWebElement *loopElement = element;
		MZNSendDebugMessage("Looking element %s", element->toString().c_str());
		bool member = formlessMode || checkFormMembership(loopElement, NULL, visibility);
		MZNSendDebugMessage("                %s status = %d", element->toString().c_str(), (int) member);

		if (!member)
			loopElement->release();
		else
		{
			bool visible = loopElement->isVisible();
			MZNSendDebugMessageA("Adding element %s", element->toString().c_str());
			InputDescriptor *id = new InputDescriptor();
			id->hasInputData = false;
			id->input = element;
			inputs.push_back(id);
			if( !visible || id->getClientHeight() < 8)
			{
				std::string soft;
				element->getProperty("soffidOnFocusTrigger", soft);
				if (soft != "true")
				{
					if (soft != "false")
						element->subscribe("focus", onHiddenElementFocusListener);
					element->setProperty("soffidOnFocusTrigger", "true");
				}
			}
			else
			{
				std::string soft;
				element->getProperty("soffidOnFocusTrigger", soft);
				if (soft == "true")
				{
					element->setProperty("soffidOnFocusTrigger", "false");
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
	element->setAttribute("style", style.c_str());

}


void SmartForm::updateIcon (InputDescriptor *input)
{
	std::string value;
	input->input->getProperty("value", value);

	if (input->status == IS_IGNORED)
	{
//		MZNSendDebugMessage("Element %s is ignored",
//				input->input->toString().c_str());
		if (input->img != NULL)
		{
			input->img->setAttribute("display", "none");
		}
		return;
	}

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
		else if (page->accounts.empty())
		{
			setDisplayStyle(input->img, "none");
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
		delete i;
	}
	inputs.clear();
}

static void getPosition (AbstractWebElement *input, long &left, long &top, long &width, long &height,
		long &width2, long &height2, bool &rightAlign, bool global)
{
	height = getIntProperty (input, "offsetHeight");
	width = getIntProperty (input, "offsetWidth");
	left = getIntProperty (input, "offsetLeft");
	top= getIntProperty (input, "offsetTop");

	height2 = getIntProperty (input, "clientHeight"); // Without border
	width2 = getIntProperty (input, "clientWidth"); // Without border

	AbstractWebElement *parentOffset = input->getOffsetParent();
	std::string align = input->getComputedStyle("text-align");
	rightAlign = align == "right";
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

static void getPosition (InputDescriptor *descriptor, long &left, long &top, long &width, long &height,
		long &width2, long &height2, bool &rightAlign, bool global)
{
	height = descriptor->getOffsetHeight();
	width = descriptor->getOffsetWidth();
	left = descriptor->getOffsetLeft();
	top = descriptor->getOffsetTop();

	height2 = descriptor->getClientHeight();
	width2 = descriptor->getClientWidth();

	AbstractWebElement *parentOffset = descriptor->input->getOffsetParent();
	std::string align = descriptor->getTextAlign();
	rightAlign = align == "right";
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
//		MZNSendDebugMessageA("Creating icon for %s",
//				input->toString().c_str());
		AbstractWebElement *img = app->createElement("img");
		descriptor->img = img;
		if (onClickListener == NULL)
		{
			onClickListener = new OnClickListener();
			onClickListener->form = this;
		}
		if (img != NULL)
			img->subscribe("click", onClickListener);
		AbstractWebElement *parent = input->getParent();
		if (parent != NULL)
		{
			parent->insertBefore(img, input);
			parent->release();
		}
	}
	if (descriptor -> img != NULL)
	{
		// Calculate position & size
		std::string type;
		bool isNumber;

		long left, top, width, height, width2, height2;
		getPosition (descriptor, left, top, width, height, width2, height2, isNumber, false);
		input->getAttribute("type", type);

		int borderH = (height - height2)/2;
		int borderW = (width - width2)/2;
		if (height <= 0) height = 20L;

		AbstractWebElement *img = descriptor->img;
		char achStyle[200];
		int zIndex = 10;
		std::string zIndexStr = input->getComputedStyle("zIndex");
		if (zIndexStr != "")
		{
			sscanf (zIndexStr.c_str(), "%d", &zIndex);
			zIndex +=10;
		} else {
			std::string zIndexStr = input->getComputedStyle("z-index");
			if (zIndexStr != "")
			{
				sscanf (zIndexStr.c_str(), "%d", &zIndex);
				zIndex +=10;
			}
		}
		if (input->getComputedStyle("position") == "static" && input->getComputedStyle("float").size() == 0)
		{
			sprintf (achStyle, "margin-left: %ldpx; margin-top: %ldpx; "
					"position: absolute; height: %ldpx; width:%ldpx; "
					"z-index:%d",
					(long) (width <= height * 3 ? width + 4L:
							isNumber ? 4L + borderW: width-height2-4L-borderW),
						1L + borderH, (long) height2 - 2L, (long) height2 - 2L,
					zIndex);
		}
		else
		{
			sprintf (achStyle, "left:%ldpx; top:%ldpx; margin-left: %ldpx; margin-top: %ldpx; "
					"position: absolute; height: %ldpx; width:%ldpx; "
					"z-index:%d",
					(long) left, (long) top,
					(long) (width <= height * 3 ? width + 4L :
							isNumber ? 4L + borderW : width-height2-4L-borderW), 1L + borderH, (long) height2 - 2L, (long) height2 - 2L,
					zIndex);
		}
		std::string currentStyle;
		img->getAttribute("style", currentStyle);
		if (currentStyle != achStyle)
		{
			img->setAttribute("style", achStyle);
			char ach[50];
			sprintf (ach, "%ldpx", height);
			img->setProperty("width", ach);
			img->setProperty("height", ach);
			img->setAttribute("_soffid_handler", "true");
			img->setAttribute("_soffid_element", "true");
		}
	}
}

bool SmartForm::addNoDuplicate (InputDescriptor* descriptor)
{
	for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		InputDescriptor *id = *it;
		if (id != NULL && id->input != NULL && id->input->equals(descriptor->input))
		{
			id -> hasInputData = descriptor -> hasInputData;
			id -> data = descriptor -> data;
			if (id -> type == IT_PASSWORD || id -> type == IT_NEW_PASSWORD)
				numPasswords ++;
			return false;
		}
	}

	std::string type = descriptor->getType();

	std::string soffidType = descriptor->getInputType();
//	MZNSendDebugMessageA("Adding element soffid type = %s",
//			soffidType.c_str());
	if ((strcasecmp ( type.c_str(), "password") == 0 && soffidType.empty()) ||
			soffidType == "password" ||
			soffidType == "new_password")
	{
		if (numPasswords == 0 || soffidType == "password" )
		{
			descriptor->type = IT_PASSWORD;
			numPasswords ++;
		}
		else if (soffidType == "new_password" )
		{
			descriptor->type = IT_NEW_PASSWORD;
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

		descriptor->existingData = true;

		if (onChangeListener == NULL)
		{
			onChangeListener = new OnChangeListener();
			onChangeListener->form = this;
		}
		descriptor->input->subscribe("input", onChangeListener);

//		MZNSendDebugMessageA("   soffid type = %d", descriptor->type);

		inputs.push_back(descriptor);
		return true;
	}
	else if (strcasecmp ( type.c_str(), "submit") == 0 || soffidType == "ignore")
	{
//		submits.push_back(input);
		return false;
	}
    // if (type == undefined || type.toLowerCase() == "text" || type.toLowerCase() == "input"  || type.toLowerCase() == "email" || type == "")
	else if (strcasecmp ( type.c_str(), "") == 0 ||
			strcasecmp ( type.c_str(), "text") == 0 ||
			strcasecmp ( type.c_str(), "input") == 0 ||
			strcasecmp ( type.c_str(), "email") == 0 ||
			strcasecmp ( type.c_str(), "number") == 0 ||
			strcasecmp ( type.c_str(), "tel") == 0 ||
			soffidType == "general")
	{
		descriptor->img = NULL;
		descriptor->type = IT_GENERAL;

		std::string name;
		name = descriptor->getAttributeName();

		descriptor->existingData = page->isAnyAttributeNamed(name.c_str());
		descriptor->status = descriptor -> existingData ? IS_EMPTY: IS_IGNORED;

		if (onChangeListener == NULL)
		{
			onChangeListener = new OnChangeListener();
			onChangeListener->form = this;
		}
		descriptor->input->subscribe("input", onChangeListener);

		inputs.push_back(descriptor);
		return true;
	}
	else
		return false;
}

bool SmartForm::checkAnyPassword (std::vector<InputDescriptor*> &elements)
{
	bool anyPassword = false;
	int nonPassword = 0;
	int password = 0;
	for (std::vector<InputDescriptor*>::iterator it = elements.begin(); it != elements.end(); it++)
	{
		InputDescriptor *id = *it;

		id->input->sanityCheck();

		std::string type = id->getType();
		std::string soffidType = id->getInputType();
		if (strcasecmp ( type.c_str(), "password") == 0 ||
				soffidType == "password" ||
				soffidType == "new_password")
		{
			if (id->isVisible())
			{
				anyPassword = true;
				password ++;
			} else {
				// Test for any account on this URL
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
			if (id->isVisible())
			{
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
		for (std::vector<InputDescriptor*>::iterator it = elements.begin(); it != elements.end(); it++)
		{
			InputDescriptor *id = *it;
			delete id;
		}
		return false;
	}
	else
	{
		MZNSendDebugMessage("* Found %d password inputs and %d non password inputs", password, nonPassword);
		return true;
	}
}

void SmartForm::parse(AbstractWebApplication *app, AbstractWebElement *formRoot, std::vector<InputData> *inputDatae) {
	PageData *data = NULL;

	numPasswords = 0;

	std::vector<InputDescriptor*> elements;

	if (this->element != NULL)
		this->element->release();
	this->element = formRoot;
	if (this->element != NULL)
		this->element -> lock();
	if (this->app != NULL)
		this->app->release();
	this->app = app;
	this->app->lock();

	if (inputDatae != NULL)
	{
		findInputs(inputDatae, elements);
	}
	else if (formRoot == NULL)
	{
		findInputs(app, elements);
	}
	else
	{
		findInputs(app, formRoot, elements, true, formRoot->isVisible(), std::string());
	}

	releaseElements();


	if (! checkAnyPassword(elements))
	{
		return;
	}

	parsed = true;

	if (page->accounts.size () == 0)
		status = SF_STATUS_NEW;
	else
		status = SF_STATUS_SELECT;


	for (std::vector<InputDescriptor*>::iterator it = elements.begin(); it != elements.end(); it++)
	{
		InputDescriptor *input = *it;

		if (input->isVisible())
		{
			if (!addNoDuplicate(input))
				delete input;
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


	if (page->accounts.size() == 1 && ! lockedOnce)
	{
		AccountStruct as = page->accounts[0];

		SecretStore s (MZNC_getUserName());
		std::wstring secret = L"sso.";
		secret += as.system;
		secret += L".";
		secret += as.account;
		secret += L".URL";
		std::string originalURL = MZNC_wstrtostr(s.getSecret(secret.c_str()));
		if (originalURL == page->url)
		{
			fetchAttributes(as, NULL);
			lockedOnce = true;
		}
	}
}

void SmartForm::reparse(std::vector<InputData> *data) {
	std::vector<InputDescriptor*> elements;

	if (data != NULL)
	{
		findInputs(data, elements);
	}
	else if (this->element == NULL)
	{
		findInputs(app, elements);
	}
	else
	{
		findInputs(app, this->element, elements, true, true, std::string());
	}
	bool newItem = false;

	if (! checkAnyPassword(elements))
		return;

	numPasswords = 0;
	for (std::vector<InputDescriptor*>::iterator it = elements.begin(); it != elements.end(); it++)
	{
		InputDescriptor *input = *it;
		if (addNoDuplicate (input))
			newItem = true;
		else
			delete input;
	}

//	MZNSendDebugMessageA("* Password inputs found: %d", numPasswords);

	if (numPasswords > 0)
	{
		for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
		{
			updateIcon (*it);
		}
	}

	if (page->accounts.size() == 1 && status == SF_STATUS_NEW && ! lockedOnce)
	{
		AccountStruct as = page->accounts[0];

		SecretStore s (MZNC_getUserName());
		std::wstring secret = L"sso.";
		secret += as.system;
		secret += L".";
		secret += as.account;
		secret += L".URL";
		std::string originalURL = MZNC_wstrtostr(s.getSecret(secret.c_str()));
		if (originalURL == page->url)
		{
			fetchAttributes(as, NULL);
			lockedOnce = true;
		}
	}
	else if (status == SF_STATUS_NEW)
	{
		changeStatus(SF_STATUS_SELECT);
	}
	else if (newItem && status == SF_STATUS_LOCKED)
	{
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
//	MZNSendDebugMessageA("OnChange %s", element->toString().c_str());

	InputDescriptor *input = findInputDescriptor(element);
	std::string value;

	if (input == NULL || input->type == IT_IGNORE)
		return;

	page->sanityCheck();
	element->sanityCheck();
	element->getProperty("value", value);
	if (input->status == IS_IGNORED && !value.empty())
	{
		input->status = IS_MODIFIED;
	}

//	MZNSendDebugMessageA("OnChange %s value = %s selected = %s",
//			element->toString().c_str(),
//			value.c_str(),
//			input->assignedValue.c_str());

	if ( value == input->assignedValue &&
			input -> status == IS_LOCKED)
	{
//		MZNSendDebugMessageA("OnChange %s (3)", element->toString().c_str());
	}
	else if (this->status == SF_STATUS_NEW)
	{
//		MZNSendDebugMessageA("OnChange %s (4)", element->toString().c_str());
		if (!value.empty())
		{
			changeStatus (SF_STATUS_MODIFYING);
		}
	}
	else if (this->status == SF_STATUS_SELECT)
	{
//		MZNSendDebugMessageA("OnChange %s (5)", element->toString().c_str());
		if (!value.empty())
		{
			changeStatus (SF_STATUS_MODIFYING);
		}
	}
	else if (this->status == SF_STATUS_GENERATING)
	{
//		MZNSendDebugMessageA("OnChange %s (6)", element->toString().c_str());
		if (!value.empty())
		{
			changeStatus (SF_STATUS_MODIFYING);
		}
		else
		{

		}
	}
	else if (status == SF_STATUS_MODIFYING)
	{
//		MZNSendDebugMessageA("OnChange %s (7)", element->toString().c_str());
		if (input->type == IT_NEW_PASSWORD)
		{
			if (value.empty())
				changeStatus (SF_STATUS_GENERATING);
		}
		else
		{
			if (value.empty())
				input->status = IS_EMPTY;
			else
				input->status = IS_MODIFIED;
			if (lastElement != input)
				updateIcon(input);
		}
	}
	lastElement = input;
}

void SmartForm::onFocus(AbstractWebElement* element) {
	InputDescriptor *input = findInputDescriptor(element);
	std::string value;

	if (input == NULL)
		return;

	page->sanityCheck();
	element->sanityCheck();

	updateIcon(input);
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
	std::string description = page->accountDomain;
	int unnamed = 1;
	for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		if ((*it)->type == IT_GENERAL && (*it)->isVisible())
		{
			AbstractWebElement *element = (*it)->input;
			element->sanityCheck();
			std::string value;
			std::string name;
			std::string id;
			element->getProperty("value", value);

			name = (*it)->getAttributeName();
			if (name.empty())
			{
				char ach[10];
				sprintf (ach,"_%d", unnamed++);
				name = ach;
			}

			if (!name.empty())
			{
				if (attributes.find(name) != attributes.end() ||
						!value.empty())
					attributes[name] = value;
				description.append (" ").append(value);
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
	std::string passwordValue;
	for (std::vector<InputDescriptor*>::iterator it = inputs.begin (); it != inputs.end(); it++)
	{
		InputDescriptor *input = *it;
		if (input->isVisible() && (
				input->type == IT_PASSWORD ||
				input->type == IT_NEW_PASSWORD))
		{
			std::string value;
			input->input->getProperty("value", value);
			if (!value.empty ())
				passwordValue = value;
		}
		input->existingData = true;
	}

	if (!passwordValue.empty () && !this->page->updatePassword (currentAccount, passwordValue, msg))
	{
		this->app->alert(MZNC_strtoutf8(msg.c_str()).c_str());
		return;
	}

	if (this->page->updateAttributes (currentAccount, attributes, msg))
		changeStatus(SF_STATUS_LOCKED);
	else
		this->app->alert(MZNC_strtoutf8(msg.c_str()).c_str());

}


#if 0
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
#endif

void SmartForm::onClickLevel(AbstractWebElement* element, const char *level) {
	if (level != NULL && level[0] != '\0')
	{
		std::string type, value, tag;
		std::string id = stylePrefix+"_id";

		InputDescriptor *descriptor = inputs[currentModalInput];
		if (descriptor != NULL)
		{

			int length;
			bool mays = false;
			bool mins = false;
			bool numbers = false;
			bool simbols = false;
			if (strcmp(level,"high")==0)
			{
				length = 24;
				mays = mins = numbers = simbols = true;
			}
			else if (strcmp(level, "medium") == 0)
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
					id -> assignedValue = newPass.c_str();
					id -> input -> setProperty("value", newPass.c_str());
				}
			}
		}
		changeStatus(SF_STATUS_MODIFYING);
	}
}

void SmartForm::onClickSave(AbstractWebElement* element, const char *action) {
	currentAccount.id = "";
	currentAccount.friendlyName = L"";
	currentAccount.system = L"";
	currentAccount.account = L"";
	for (std::vector<AccountStruct>::iterator it = page->accounts.begin(); it != page->accounts.end(); it++)
	{
		if (it->id == action)
		{
			currentAccount = *it;
		}
	}

	save ();

}


void SmartForm::onClickAccount(AbstractWebElement* element, const char *account) {
	AbstractWebElement *input = NULL;
	std::string id;
	AccountStruct as;

	page->sanityCheck();

	if (currentModalInput >= 0 && currentModalInput < inputs.size())
	{
		InputDescriptor *descriptor = inputs[currentModalInput];
		if (descriptor != NULL && descriptor->input != NULL)
			input = descriptor->input;
	}

	page->getAccountStruct(account, as);

//	onClickModal (element);

	fetchAttributes (as, input);
}

class AuditInfo
{
public:
	std::wstring user;
	std::wstring key;
	std::wstring system;
	std::wstring account;
};

#ifdef WIN32
static DWORD WINAPI sendAuditInfo(
  LPVOID arg
)
#else
static void* sendAuditInfo (void *arg)
#endif
{
	AuditInfo* ai = (AuditInfo*) arg;
	SeyconService ss;
	SeyconResponse *response = ss.sendUrlMessage(L"/auditPassword?user=%ls&key=%ls&system=%ls&account=%ls&application=%ls",
			ai->user.c_str(), ai->key.c_str(), ai->system.c_str(), ai->account.c_str(),
			ss.escapeString(MZNC_getCommandLine()).c_str());
	if (response != NULL)
		delete response;
	delete ai;

	return 0;
}

void SmartForm::fetchAttributes(AccountStruct &as, AbstractWebElement *selectedElement)
{
	currentAccount = as;
	std::map<std::string,std::string> attributes;

	page->fetchAttributes(app, as, attributes);


	MZNSendDebugMessageA("Setting attributes");
	bool first = true;
	int unnamed = 1;
	for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		InputDescriptor *descr = *it;
		AbstractWebElement *e = descr->input;
//		MZNSendDebugMessageA("Setting attributes for element %s (visible = %s) type = %d",
//				descr->input->toString().c_str(),
//				e->isVisible() ? "true" : "false",
//				descr->type);
		e->sanityCheck();
		if (descr->type == IT_GENERAL &&
				(e->isVisible() || !descr->getMirrorOf().empty()))
		{
			std::string currentValue;
			std::string name;
			std::string id;
			name = descr->getAttributeName();
			if (name.empty())
			{
				char ach[10];
				sprintf (ach,"_%d", unnamed++);
				name = ach;
			}
			e->getProperty("value", currentValue);
			if (attributes.find(name) != attributes.end())
			{
				std::string v2 = MZNC_strtoutf8(attributes[name].c_str());
				if (v2 != currentValue)
				{
					if (e->isVisible())
						e->focus();
					descr->assignedValue = v2.c_str();
					e->setProperty("value", v2.c_str());
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
						descr-> assignedValue = value.c_str();
						e->setProperty("value", value.c_str());
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
								descr-> assignedValue = value.c_str();
								e->setProperty("value", value.c_str());
							}
						}
					}
				}
			} else {
				//MZNSendDebugMessageA("Cannot find value for %s", name.c_str());
			}
		}
		else if ((e->isVisible() || descr->getInputType() == "password") &&
					(descr->type == IT_PASSWORD ||
					 descr->type == IT_NEW_PASSWORD && selectedElement != NULL && e->equals(selectedElement) ))
		{
//			MZNSendDebugMessageA("It is password type");
			SecretStore s(MZNC_getUserName());
			std::wstring secret = L"pass.";
			secret += as.system;
			secret += L".";
			secret += as.account;
			wchar_t *pass = s.getSecret(secret.c_str());
			if (pass != NULL)
			{
				e->focus();
				std::string type;
				e->getAttribute("type", type);
				if (type == "password")
				{
					descr-> assignedValue = MZNC_wstrtoutf8(pass).c_str();
					e->setProperty("value", MZNC_wstrtoutf8(pass).c_str());
//					MZNSendDebugMessageA("Setting password value");
				} else {
//					MZNSendDebugMessageA("Ignoring dummy password input");
				}
				s.freeSecret(pass);
			}
			wchar_t *sessionKey = s.getSecret(L"sessionKey");
			wchar_t *user = s.getSecret(L"user");

			AuditInfo *ai = new AuditInfo;
			ai->user = user;
			ai->key = sessionKey;
			ai->system = as.system;
			ai->account = as.account;

#ifdef WIN32
			CreateThread (NULL,  0, sendAuditInfo, ai,0, NULL) ;
#else
			pthread_t threadId;
			pthread_create(&threadId, NULL, sendAuditInfo, ai);
#endif
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

	int unnamed = 1;
	for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		InputDescriptor *descr = *it;
		AbstractWebElement *e = descr->input;
		e->sanityCheck();
		std::string name;
		std::string id;
		name = (*it)->getAttributeName();
		if (name.empty())
		{
			char ach[10];
			sprintf (ach,"_%d", unnamed++);
			name = ach;
		}
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
	if (this->status != status)
	{
		this->status = status;
		MZNSendDebugMessage("Updating icons (status = %d)", status);

		for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
		{
			InputDescriptor *id = *it;
			updateIcon (id);
		}
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
	currentModalInput = 0;
	for (unsigned int i = 0; i < inputs.size(); i++)
	{
		if (inputs[i]->img != NULL && inputs[i]->img->equals(input) ||
				inputs[i]->input != NULL && inputs[i]->input->equals(input))
		{
			currentModalInput = i;
		}
	}

	return NULL;

}

void SmartForm::createModal(AbstractWebElement *img)
{
	std::vector<std::string> ids;
	std::vector<std::string> names;


	createModalDialog(img);

	for (std::vector<AccountStruct>::iterator it = page->accounts.begin(); it != page->accounts.end(); it++)
	{
		AccountStruct as = *it;
		std::string friendlyName = MZNC_wstrtoutf8(as.friendlyName.c_str());

		ids.push_back(std::string("a") + as.id);
		names.push_back( friendlyName );
	}
	app->selectAction("Select account", ids, names, img, onClickListener);
}

void SmartForm::createGenerateModal(AbstractWebElement *img)
{
	std::vector<std::string> ids;
	std::vector<std::string> names;

	createModalDialog(img);

	static const char *levels[] = {"high", "medium", "low"} ;
	static const char *levelDescription[] = {"High security", "Medium security", "Low security"} ;
	for (int i= 0; i < 3; i++)
	{
		ids.push_back( std::string ("l") + levels[i]);
		names.push_back( std::string(levelDescription[i]));
	}

	// ---------------- Save current account
	if (!currentAccount.id.empty())
	{
		ids.push_back(std::string("a") + currentAccount.id);
		names.push_back( MZNC_wstrtoutf8( (std::wstring(L"Get password for ")+currentAccount.friendlyName).c_str() ));
	}
	else if (! page->accounts.empty())
	{
		for (std::vector<AccountStruct>::iterator it = page->accounts.begin(); it != page->accounts.end(); it++)
		{
			AccountStruct as = *it;
			std::string friendlyName = MZNC_wstrtoutf8(as.friendlyName.c_str());

			ids.push_back(std::string("a") + as.id);
			names.push_back( friendlyName );
		}
	}
	app->selectAction("Generate password", ids, names, img, onClickListener);
}



void SmartForm::createSaveModal(AbstractWebElement *img)
{
	std::vector<std::string> ids;
	std::vector<std::string> names;

	createModalDialog(img);

	// ---------------- Save current account
	if (!currentAccount.id.empty())
	{
		ids.push_back( std::string("s")+currentAccount.id);
		names.push_back ( MZNC_wstrtoutf8( (std::wstring(L"Update identity ")+currentAccount.friendlyName).c_str()) );
	}

	// ---------------- Create new account
	ids.push_back( std::string("s-"));
	names.push_back ( std::string("As new identity"));

	// ---------------- Save as existing accounts
	if (currentAccount.id.empty())
	{
		for (std::vector<AccountStruct>::iterator it = page->accounts.begin(); it != page->accounts.end(); it++)
		{
			AccountStruct s = *it;
			ids.push_back( std::string("s")+s.id);
			names.push_back (  MZNC_wstrtoutf8( ( std::wstring(L"Update identity ")+s.friendlyName).c_str()));

		}
	}
	app->selectAction("Save identity", ids, names, img, onClickListener);
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



long InputDescriptor::getClientHeight()
{
	if (hasInputData)
		return data.clientHeight;
	else
		return getIntProperty(input, "clientHeight");
}

long InputDescriptor::getClientWidth (){
	if (hasInputData)
		return data.clientWidth;
	else
		return getIntProperty(input, "clientWidth");
}

std::string InputDescriptor::getDataBind () {
	if (hasInputData)
		return data.data_bind;
	else
	{
		std::string s;
		input->getAttribute("data-bind", s);
		return s;
	}
}

std::string InputDescriptor::getDisplay () {
	if (hasInputData)
		return data.data_bind;
	else
	{
		return input->getComputedStyle("display");
	}
}

std::string InputDescriptor::getId () {
	if (hasInputData)
		return data.id;
	else
	{
		std::string s;
		input->getProperty("id", s);
		return s;
	}
}

std::string InputDescriptor::getName() {
	if (hasInputData)
		return data.name;
	else
	{
		std::string s;
		input->getProperty("name", s);
		return s;
	}
}

std::string InputDescriptor::getType() {
	if (hasInputData)
		return data.type;
	else
	{
		std::string s;
		input->getProperty("type", s);
		return s;
	}
}

long InputDescriptor::getOffsetHeight() {
	if (hasInputData)
		return data.offsetHeight;
	else
		return getIntProperty(input, "offsetHeight");
}
long InputDescriptor::getOffsetLeft() {
	if (hasInputData)
		return data.offsetLeft;
	else
		return getIntProperty(input, "offsetLeft");
}

long InputDescriptor::getOffsetTop() {
	if (hasInputData)
		return data.offsetTop;
	else
		return getIntProperty(input, "offsetTop");
}

long InputDescriptor::getOffsetWidth() {
	if (hasInputData)
		return data.offsetWidth;
	else
		return getIntProperty(input, "offsetWidth");
}

bool InputDescriptor::isRightAlign () {
	return getTextAlign() == "right";
}

std::string InputDescriptor::getStyle () {
	if (hasInputData)
		return data.style;
	else
	{
		std::string s;
		input->getAttribute("style", s);
		return s;
	}
}

std::string InputDescriptor::getTextAlign () {
	if (hasInputData)
		return data.style;
	else
	{
		return input->getComputedStyle("text-align");
	}
}

std::string InputDescriptor::getValue() {
	std::string v;
	input->getProperty("value", v);
	return v;
}

std::string InputDescriptor::getVisibility ()  {
	if (hasInputData)
		return data.style;
	else
	{
		return input->getComputedStyle("visibility");
	}
}

bool InputDescriptor::isVisible ()  {
	if (hasInputData)
		return data.display != "none" && data.visibility != "hidden";
	else
	{
		return input->isVisible();
	}
}

InputDescriptor::~InputDescriptor() {
	if (input != NULL)
		input->release();
	input = NULL;
	if (img != NULL)
		img->release();
	img = NULL;
}

InputDescriptor::InputDescriptor() {
	input = NULL;
	img = NULL;
	type = IT_GENERAL;
	status = IS_EMPTY;
	hasInputData = false;

}

std::string InputDescriptor::getMirrorOf() {
	if (hasInputData)
		return data.mirrorOf;
	else
	{
		std::string mirror;
		input->getProperty("soffidMirrorOf", mirror);
		return mirror;
	}
}

std::string InputDescriptor::getInputType() {
	if (hasInputData)
		return data.inputType;
	else
	{
		std::string type;
		input->getProperty("soffidInputType", type);
		return type;
	}
}

std::string InputDescriptor::getAttributeName() {
	std::string name;
	name = getMirrorOf();
	if (name.empty())
		name = getName();
	if (name.empty())
		name = getId();
	if (name.empty())
		name = getDataBind();
	return name;
}
