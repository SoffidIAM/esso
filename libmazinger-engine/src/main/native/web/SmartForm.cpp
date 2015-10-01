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

SmartForm::SmartForm(SmartWebPage *page) {
	this->page = page;
	this->app = NULL;
	this->element = NULL;
	this->onChangeListener = NULL;
	this->onClickListener = NULL;
	this->onBeforeUnloadListener = NULL;
	status = SF_STATUS_NEW;
	char ach[100];
	sprintf (ach, "_soffid_%ld_id_", (long) (long long) this);
	stylePrefix = ach;
	numPasswords = 0;
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
}


static long getIntProperty (AbstractWebElement *element, const char* property)
{
	std::string s;
	element->getProperty(property, s);
	long l = 0;
	sscanf (s.c_str(), " %ld", &l);
	return l;
}


static void findInputs (AbstractWebApplication* app, AbstractWebElement *element, std::vector<AbstractWebElement*> &inputs)
{
	std::string tagname;
	element->getTagName(tagname);
	if (strcasecmp (tagname.c_str(), "input") == 0)
	{
		inputs.push_back(element);
		element->lock();
	}
	else
	{
		std::vector<AbstractWebElement*> children;
		element->getChildren(children);
		for (std::vector<AbstractWebElement*>::iterator it = children.begin(); it != children.end(); it++)
		{
			AbstractWebElement *child = *it;
			findInputs (app, child, inputs);
			child->release();
		}
	}
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

//	MZNSendDebugMessage("At %s:%d", __FILE__,__LINE__);

	addIcon(input);

//	MZNSendDebugMessage("At %s:%d", __FILE__,__LINE__);

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
			setDisplayStyle(input->img, "inline");
			input->img->setAttribute("src", _img_resource_generate);
		}
	}
	else if (input->status == IS_LOCKED)
	{
		setDisplayStyle(input->img, "inline");
		input->img->setAttribute("src", _img_resource_unlock);
	}
	else if (input->status == IS_SELECT)
	{
		setDisplayStyle(input->img, "inline");
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
		setDisplayStyle(input->img, "inline");
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

void SmartForm::addIcon (InputDescriptor *descriptor)
{
	AbstractWebElement *input = descriptor->input;
	if ( descriptor->img == NULL)
	{
		// Calculate position & size
		long width = getIntProperty(input, "offsetWidth");
		long height = getIntProperty (input, "offsetHeight");
		long height2 = getIntProperty (input, "clientHeight"); // Without border
		long width2 = getIntProperty (input, "clientWidth"); // Without border
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
				sprintf (achStyle, "margin-left: %ldpx; margin-top: %ldpx; position: absolute; height: %ldpx; width:%ldpx; z-index:+1",
						(long) (width-height2-4L-borderW), 1L + borderH, (long) height2 - 2L, (long) height2 - 2L);
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
//	MZNSendDebugMessage("Analyzing form =============================");
	for (std::vector<AbstractWebElement*>::iterator it = elements.begin(); it != elements.end(); it++)
	{
		AbstractWebElement *input = *it;

		input->sanityCheck();

		std::string type;
		input -> getAttribute("type", type);
		if (strcasecmp ( type.c_str(), "password") == 0)
		{
//			MZNSendDebugMessage("Found password element %s", input->toString().c_str());
			anyPassword = true;
		} else if (strcasecmp ( type.c_str(), "hidden") != 0) {
//			MZNSendDebugMessage("Found non password element %s", input->toString().c_str());
			nonPassword ++;
		}
	}

	// Do nothing
	if (! anyPassword || nonPassword > 10)
	{
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
		return true;
}

void SmartForm::parse(AbstractWebApplication *app, AbstractWebElement *formRoot) {
	std::vector<AbstractWebElement*> elements;
	if (this->element != NULL)
		this->element->release();
	this->element = formRoot;
//	MZNSendDebugMessage(formRoot->toString().c_str());
	this->element -> lock();
	if (this->app != NULL)
		this->app->release();
	this->app = app;
	this->app->lock();

	findInputs(app, formRoot, elements);

	releaseElements();


	if (page->accounts.size () == 0)
		status = SF_STATUS_MODIFYING;


	if (! checkAnyPassword(elements))
		return;


	for (std::vector<AbstractWebElement*>::iterator it = elements.begin(); it != elements.end(); it++)
	{
		AbstractWebElement *input = *it;

		input->sanityCheck();

		addNoDuplicate(input);

		input->release();
	}

	if (numPasswords > 0)
	{
		for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
		{
			updateIcon (*it);
		}
	}

	if (onBeforeUnloadListener == NULL)
	{
		onBeforeUnloadListener = new OnBeforeUnloadListener();
		onBeforeUnloadListener->form = this;
//		app->subscribe("beforeunload", onBeforeUnloadListener);
	}


	if (page->accounts.size() == 1 && inputs.size() < 5)
	{
		AccountStruct as = page->accounts[0];
		fetchAttributes(as);
	} else if (page->accounts.size() > 1)
	{
		changeStatus(SF_STATUS_SELECT);
	} else if (numPasswords >= 2)
	{
//		changeStatus(SF_STATUS_GENERATING);
	}
}

void SmartForm::reparse() {
	std::vector<AbstractWebElement*> elements;
	findInputs(app, this->element, elements);
	bool newPassword = false;

	if (! checkAnyPassword(elements))
		return;

	for (std::vector<AbstractWebElement*>::iterator it = elements.begin(); it != elements.end(); it++)
	{
		AbstractWebElement *input = *it;
		input->sanityCheck();
		addNoDuplicate (input);
		input->release();
	}

	if (numPasswords > 0)
	{
		for (std::vector<InputDescriptor*>::iterator it = inputs.begin(); it != inputs.end(); it++)
		{
			updateIcon (*it);
		}
	}

	if (page->accounts.size() == 1 && status == SF_STATUS_NEW && inputs.size() < 5)
	{
		AccountStruct as = page->accounts[0];
		fetchAttributes(as);
	} else if (page->accounts.size() > 1  && status == SF_STATUS_NEW)
	{
		changeStatus(SF_STATUS_SELECT);
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
		if (input->status != IS_EMPTY && !value.empty())
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
			createModal(element);
	}
	else if (descr->status == IS_MODIFIED)
	{
//		MZNSendDebugMessage("At %s:%d", __FILE__,__LINE__);
		if (currentAccount.account.empty() && page->accounts.size() == 0)
			save ();
		else //if (detectAttributeChange())
			createSaveModal(element);
//		MZNSendDebugMessage("At %s:%d", __FILE__,__LINE__);
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
			{
				name = id;
			}

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
	element->sanityCheck();
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
			AbstractWebElement *input = masterDiv->getNextSibling();
			do
			{
				input->getTagName(tag);
				if (strcasecmp(tag.c_str(),"input") == 0)
				{
					input->getProperty("type", type);
					input->getProperty("value", value);
					break;
				}
				else
					input = input->getPreviousSibling();
			} while (input != NULL);

			InputDescriptor *descriptor = findInputDescriptor(input);
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
	page->sanityCheck();
	element->sanityCheck();
	std::string id;
	element->getAttribute("_soffid_account", id);
	AccountStruct as;
	page->getAccountStruct(id.c_str(), as);
	onClickModal (element);
	fetchAttributes (as);
}

void SmartForm::fetchAttributes(AccountStruct &as)
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
		if (descr->type == IT_GENERAL)
		{
			std::string name;
			std::string id;
			e->getAttribute("name", name);
			if (name.empty())
				e->getAttribute("id", name);
			if (attributes.find(name) != attributes.end())
			{
				std::string value = attributes[name];
				e->focus();
				e->setProperty("value", MZNC_strtoutf8(value.c_str()).c_str());
			}
			else if (first)
			{
				first = false;
				std::string ssoSystem;
				SeyconCommon::readProperty("AutoSSOSystem", ssoSystem);
				if (currentAccount.system != MZNC_strtowstr(ssoSystem.c_str()))
				{
					e->focus();
					e->setProperty("value", MZNC_wstrtoutf8(currentAccount.account.c_str()).c_str());
				}
				else
				{
					for (std::map<std::string,std::string>::iterator it = attributes.begin(); it != attributes.end(); it++)
					{
						std::string name = toLowerCase(it->first.c_str());
						if ( name.find("user") != name.npos || name.find("login") != name.npos)
						{
							e->focus();
							e->setProperty("value", MZNC_strtoutf8(it->second.c_str()).c_str());
						}
					}
				}

			}
		}
		else if (descr->type == IT_PASSWORD)
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
		}
	}

	if (numPasswords < 2)
		changeStatus(SF_STATUS_LOCKED);
	else
		changeStatus(SF_STATUS_GENERATING);
}

bool SmartForm::detectAttributeChange()
{
//	MZNSendDebugMessage("At %s:%d", __FILE__,__LINE__);
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

static const char *STYLE_MASTER = "z-index: 10000; text-align:left; border: solid white 2px; display: inline-block; margin-left:%ldpx; margin-top: %ldpx; position: absolute; min-width: 200px; "
		"box-shadow: 10px 10px 4px #888888; background-color: white; ";
static const char *STYLE_MODAL = "position: fixed;"
	   	   "top:0; right: 0 ; bottom: 0; left:0;"
	   	   "background:black;"
	   	   "z-index: 9999;"
	   	   "opacity: 0.3;"
	   	   "pointer-events: auto;"
	   	   "filter: progid:DXImageTransform.Microsoft.Alpha(Opacity=30);"
           "-ms-filter: progid:DXImageTransform.Microsoft.Alpha(Opacity=30);";
static const char *STYLE_SELECTOR_HOVER = "_selector:hover {background-color: #a6d100;}\n";
static const char *STYLE_SELECTOR = "_selector { "
		"background-color: #5f7993; "
		"cursor: pointer; "
		"padding-left:5px; "
		"padding-right: 5px;"
  	  	"padding-top:3px; "
  	  	"padding-bottom: 3px;}\n";
static const char *STYLE_HEADER= "_header {"
	   	   "background-color: #17b5c8; "
	   	   "padding-left:5px; "
	   	   "padding-right: 5px;"
	   	   "padding-top:3px; "
	   	   "padding-bottom: 3px;}\n";

void SmartForm::createStyle () {

	AbstractWebApplication * app = element->getApplication();
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
	if (app != NULL) app->release();
}
void SmartForm::createModal(AbstractWebElement *img)
{
	std::string link;

	SeyconCommon::readProperty("AutoSSOURL", link);

	AbstractWebApplication *app = img->getApplication();
	AbstractWebElement *parent = img->getParent();
	if (parent != NULL)
	{
		std::string id = stylePrefix+"_id";
		AbstractWebElement *masterDiv = app->getElementById(id.c_str());
		if (masterDiv != NULL)
		{
			parent->removeChild(masterDiv);
			masterDiv->release();
		}
		AbstractWebElement *input = img->getNextSibling();
		if ( input == NULL)
		{
			MZNSendDebugMessageA("Cannot find input element");
			return;
		}

		masterDiv = app->createElement("div");
		if (masterDiv == NULL)
		{
			MZNSendDebugMessageA("Cannot create master div");
			return;
		}

		masterDiv->setAttribute("_soffid_element", "true");
		long height = getIntProperty (input, "offsetHeight");
		long width = getIntProperty (input, "offsetWidth");

		masterDiv->setAttribute("_soffid_modal", "true");
		char ach[1000];
		sprintf (ach, STYLE_MASTER, width-200+2, height);
		masterDiv->setAttribute("style", ach);
		masterDiv->setAttribute("id", id.c_str());
		parent->insertBefore(masterDiv, img);

		createStyle();

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
			std::string friendlyName = MZNC_wstrtostr(as.friendlyName.c_str());
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

		id =  (stylePrefix+"_modal");
		AbstractWebElement *modal = app->getElementById(id.c_str());
		if (modal != NULL)
		{
			parent->removeChild(modal);
			modal->release();
		}
		modal = app->createElement("div");
		if (modal == NULL) return;
		modal->setAttribute("_soffid_element", "true");
		modal->setAttribute("style", STYLE_MODAL);
		modal->setAttribute("id", id.c_str());
		modal->setAttribute("_soffid_modal", "true");
		masterDiv->appendChild(modal);
		modal->subscribe("click", onClickListener);
		parent->insertBefore(modal, img);
		modal->release();
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

		AbstractWebElement *masterDiv = app->getElementById(id.c_str());
		if (masterDiv != NULL)
		{
			parent->removeChild(masterDiv);
			masterDiv->release();
		}

		masterDiv = app->createElement("div");
		if (masterDiv == NULL) return;
		masterDiv->setAttribute("_soffid_element", "true");
		long height = getIntProperty (input, "offsetHeight");
		long width = getIntProperty (input, "offsetWidth");

		masterDiv->setAttribute("_soffid_modal", "true");
		char ach[1000];
		sprintf (ach, STYLE_MASTER, width-200+2, height);
		masterDiv->setAttribute("style", ach);
		masterDiv->setAttribute("id", id.c_str());
		parent->insertBefore(masterDiv, input);

		createStyle();

		AbstractWebElement *user = app->createElement("div");

		if (user == NULL) return;
		user->setAttribute("class", (stylePrefix+ "_header").c_str());
		user->setTextContent("Security level");
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

		id =  (stylePrefix+"_modal");
		AbstractWebElement *modal = app->getElementById(id.c_str());
		if (modal != NULL)
		{
			parent->removeChild(modal);
			modal->release();
		}
		modal = app->createElement("div");
		if (modal == NULL) return;
		modal->setAttribute("_soffid_element", "true");
		modal->setAttribute("style", STYLE_MODAL);
		modal->setAttribute("id", id.c_str());
		modal->setAttribute("_soffid_modal", "true");
		masterDiv->appendChild(modal);
		modal->subscribe("click", onClickListener);
		parent->insertBefore(modal, img);
		modal->release();
		masterDiv->release();
		parent->release();
		input->release();
	}
}



void SmartForm::createSaveModal(AbstractWebElement *img)
{
//	MZNSendDebugMessage("At %s:%d", __FILE__,__LINE__);
	AbstractWebApplication *app = img->getApplication();
	AbstractWebElement *parent = img->getParent();
	if (parent != NULL)
	{
		std::string id = stylePrefix+"_id";
		AbstractWebElement *input = img->getNextSibling();
		if (input == NULL)
			return;

		AbstractWebElement *masterDiv = app->getElementById(id.c_str());
		if (masterDiv != NULL)
		{
			parent->removeChild(masterDiv);
			masterDiv->release();
		}

		masterDiv = app->createElement("div");
		if (masterDiv == NULL) return;
		masterDiv->setAttribute("_soffid_element", "true");
		long height = getIntProperty (input, "offsetHeight");
		long width = getIntProperty (input, "offsetWidth");

		masterDiv->setAttribute("_soffid_modal", "true");
		char ach[1000];
		sprintf (ach, STYLE_MASTER, width-200+2, height);
		masterDiv->setAttribute("style", ach);
		masterDiv->setAttribute("id", id.c_str());
		parent->insertBefore(masterDiv, input);

		createStyle();

		AbstractWebElement *user = app->createElement("div");
		if (user == NULL) return;
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


		id =  (stylePrefix+"_modal");
		AbstractWebElement *modal = app->getElementById(id.c_str());
		if (modal != NULL)
		{
			parent->removeChild(modal);
			modal->release();
		}
		modal = app->createElement("div");
		if (modal == NULL) return;
		modal->setAttribute("_soffid_element", "true");
		modal->setAttribute("style", STYLE_MODAL);
		modal->setAttribute("id", id.c_str());
		modal->setAttribute("_soffid_modal", "true");
		masterDiv->appendChild(modal);
		modal->subscribe("click", onClickListener);
		parent->insertBefore(modal, img);
		modal->release();
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
	}
	return s;
}
