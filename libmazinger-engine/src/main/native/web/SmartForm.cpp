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

class OnChangeListener: public WebListener
{
public:
	SmartForm *form;
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component);

};

class OnClickListener: public WebListener
{
public:
	SmartForm *form;
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component);

};

class OnBeforeUnloadListener: public WebListener
{
public:
	SmartForm *form;
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component);

};

void OnChangeListener::onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component) {
	MZNSendDebugMessage("OnChangeListener");
	if (form != NULL)
	{
		form -> onChange (component);
	}
}

void OnClickListener::onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component) {
	MZNSendDebugMessage("OnClickListener");
	if (form != NULL)
	{
		std::string id;
		component->getAttribute("_soffid_handler", id);
		if (id == "true")
			form -> onClickImage(component);
		component->getAttribute("_soffid_account", id);
		if (!id.empty())
			form -> onClickAccount(component);
		component->getAttribute("_soffid_modal", id);
		if (id == "true")
			form -> onClickModal(component);
	}
}

void OnBeforeUnloadListener::onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component) {
	MZNSendDebugMessage("OnBeforeUnload");
	form->onBeforeUnload();
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
	sprintf (ach, "_soffid_%ld_id_", (long) this);
	stylePrefix = ach;
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
	}
	else
	{
		std::vector<AbstractWebElement*> children;
		element->getChildren(children);
		for (std::vector<AbstractWebElement*>::iterator it = children.begin(); it != children.end(); it++)
		{
			findInputs (app, *it, inputs);
		}
		element-> release();
	}
}

void SmartForm::updateIcon (AbstractWebElement *img, bool isPassword, bool isOldPassword, bool isNewPassword)
{
	MZNSendDebugMessageA("Updating icon on %ld", (long) img);
	if (status == SF_STATUS_NEW) // Unlocked state
	{
		if (isPassword || isOldPassword)
			img->setAttribute("src", _img_resource_password);
		else if (isNewPassword)
		{
			if (inputs.size() == 0 && passwords.size() < 3)
				img->setAttribute("src", _img_resource_user);
			else
				img->setAttribute("src", _img_resource_generate);
		}
		else
			img->setAttribute("src", _img_resource_user);
	}
	if (status == SF_STATUS_LOCKED)
	{
		img->setAttribute("src", _img_resource_unlock);
	}
	if (status == SF_STATUS_MODIFYING)
	{
		std::string v;
		img->getAttribute("value", v);
		if (isPassword || isOldPassword)
			img->setAttribute("src", _img_resource_save);
		else if (isNewPassword)
		{
			if (inputs.size() == 0 && passwords.size() < 3 && v.empty())
				img->setAttribute("src", _img_resource_user);
			else
				img->setAttribute("src", _img_resource_save);
		}
		else
		{
			if (inputs.size() == 0 && passwords.size() < 3 && v.empty())
				img->setAttribute("src", _img_resource_user);
			else
				img->setAttribute("src", _img_resource_save);
		}
	}
	if (status == SF_STATUS_GENERATING)
	{
		std::string v;
		img->getAttribute("value", v);
		if (isPassword || isOldPassword)
			img->setAttribute("src", _img_resource_unlock);
		else if (isNewPassword)
		{
			img->setAttribute("src", _img_resource_generate);
		}
		else
		{
			img->setAttribute("src", _img_resource_unlock);
		}
	}
}

void SmartForm::releaseElements ()
{
	for (std::vector<AbstractWebElement*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		(*it)->release();
	}
	int i = 0;
	for (std::vector<AbstractWebElement*>::iterator it = passwords.begin(); it != passwords.end(); it++)
	{
		(*it)->release();
	}
	for (std::vector<AbstractWebElement*>::iterator it = submits.begin(); it != submits.end(); it++)
	{
		(*it)->release();
	}
}

void SmartForm::addIcon (AbstractWebElement *input, bool isPassword, bool isOldPassword, bool isNewPassword)
{
	MZNSendDebugMessageA("En step 1");
	AbstractWebElement *next = input->getNextSibling();
	bool alreadyManaged = false;
	if (next != NULL )
	{
		std::string t ;
		next->getAttribute("_soffid_handler", t);
		if (t == "true")
		{
			alreadyManaged = true;
			updateIcon (next, isPassword, isOldPassword, isNewPassword);
		}
	}
	if (! alreadyManaged)
	{
		// Calculate position & size
		long width = getIntProperty(input, "offsetWidth");
		long height = getIntProperty (input, "offsetHeight");
		if (height <= 0) height = 20L;

		AbstractWebElement *img = app->createElement("img");
		if (img != NULL)
		{
			MZNSendDebugMessageA("En step 3");
			char achStyle[150];
			sprintf (achStyle, "margin-left:-%ldpx; position: absolute; height: %ldpx;",
					height + 3, height);
			img->setAttribute("style", achStyle);
			img->setAttribute("_soffid_handler", "true");
			if (isNewPassword) img->setAttribute("_soffid_new_password", "true");
			if (isOldPassword) img->setAttribute("_soffid_old_password", "true");
			if (isPassword) img->setAttribute("_soffid_password", "true");
			updateIcon(img, isPassword, isOldPassword, isNewPassword);
			input->getParent()->insertBefore(img, input->getNextSibling());
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
		}
	}
}

void SmartForm::parse(AbstractWebApplication *app, AbstractWebElement *formRoot) {
	std::vector<AbstractWebElement*> elements;
	if (this->element != NULL)
		this->element->release();
	this->element = formRoot;
	this->element -> lock();
	if (this->app != NULL)
		this->app->release();
	this->app = app;
	this->app->lock();
	findInputs(app, formRoot, elements);

	passwords.clear();
	inputs.clear();
	submits.clear();
	for (std::vector<AbstractWebElement*>::iterator it = elements.begin(); it != elements.end(); it++)
	{
		AbstractWebElement *input = *it;
		std::string type;
		input -> getAttribute("type", type);
		if (strcasecmp ( type.c_str(), "password") == 0)
		{
			input->lock();
			passwords.push_back(input);
		}
        // if (type == undefined || type.toLowerCase() == "text" || type.toLowerCase() == "input"  || type.toLowerCase() == "email" || type == "")
		else if (strcasecmp ( type.c_str(), "") == 0 ||
				strcasecmp ( type.c_str(), "text") == 0 ||
				strcasecmp ( type.c_str(), "input") == 0 ||
				strcasecmp ( type.c_str(), "email") == 0)
		{
			input->lock();
			this->inputs.push_back(input);
		}
		else if (strcasecmp ( type.c_str(), "submit") == 0)
		{
			input->lock();
			submits.push_back(input);
		}
	}

	MZNSendDebugMessage("Text Inputs Size = %d", this->inputs.size());
	MZNSendDebugMessage("Passwords Size = %d", passwords.size());
	MZNSendDebugMessage("Submit Size = %d", submits.size());
	if (passwords.size() > 0)
	{
		for (std::vector<AbstractWebElement*>::iterator it = inputs.begin(); it != inputs.end(); it++)
		{
			addIcon (*it, false, false, false);
		}
		int i = 0;
		for (std::vector<AbstractWebElement*>::iterator it = passwords.begin(); it != passwords.end(); it++)
		{
			addIcon (*it, passwords.size() == 1, passwords.size() == 3 && i == 0, i > 1 || passwords.size() == 2);
			i ++;
		}
	}

	if (onBeforeUnloadListener == NULL)
	{
		onBeforeUnloadListener = new OnBeforeUnloadListener();
		onBeforeUnloadListener->form = this;
		app->subscribe("beforeunload", onBeforeUnloadListener);
	}

	if (page->accounts.size() == 1)
	{
		AccountStruct as = page->accounts[0];
		fetchAttributes(as);
	}
}



AbstractWebElement* SmartForm::findUsernameInput() {
	AbstractWebElement *element = NULL;
	if (inputs.size() == 0)
		return NULL;
	for (std::vector<AbstractWebElement*>::iterator it = inputs.end (); it != inputs.begin();)
	{
		it --;
		AbstractWebElement *e = *it;
		std::string name;
		e->getAttribute("name", name);
		if (strstr(name.c_str(), "user") != NULL ||
				strstr(name.c_str(), "login") != NULL ||
				strstr(name.c_str(), "USER") != NULL ||
				strstr(name.c_str(), "LOGIN") != NULL)
		{
			return e;
		}
	}
	return inputs[0];
}

void SmartForm::onChange(AbstractWebElement* element) {
	if (this->status == SF_STATUS_NEW)
	{
		changeStatus(SF_STATUS_MODIFYING);
	}
	if (this->status == SF_STATUS_GENERATING)
	{
		AbstractWebElement *icon = element->getNextSibling();
		if (icon != NULL)
		{
			std::string s;
			icon->getAttribute("_soffid_new_password", s);
			if (s == "true")
			{
				changeStatus (SF_STATUS_MODIFYING);
			}
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

static AbstractWebElement *findLoginElement (std::vector<AbstractWebElement*> &inputs)
{
	if (inputs.size() == 0)
		return NULL;

	for (std::vector<AbstractWebElement*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		AbstractWebElement *element = *it;
		std::string name;
		std::string id;
		element->getAttribute("name", name);
		name = toLowerCase(name.c_str());
		element->getAttribute("id", id);
		id = toLowerCase(id.c_str());
		if (strstr(name.c_str(),"user") != NULL ||
				strstr(name.c_str(),"login") != NULL ||
				strstr(id.c_str(),"user") != NULL ||
				strstr(id.c_str(),"login") != NULL)
			return element;
	}
	return *inputs.begin();
}

void SmartForm::onClickImage(AbstractWebElement* element) {
	if (this->status == SF_STATUS_NEW)
	{
		createModal(element);
	}
	if (this->status == SF_STATUS_MODIFYING)
	{
		AbstractWebElement *loginElement = findLoginElement(inputs);
		if (loginElement != NULL)
		{
			std::string account;
			loginElement->getProperty("value", account);
			MZNSendDebugMessageA("Account value = %s", account.c_str());
			if (!account.empty())
			{
				MZNSendDebugMessageA("Sending password and attributes to server");
				std::map<std::string,std::string> attributes;
				std::string msg;
				for (std::vector<AbstractWebElement*>::iterator it = inputs.begin(); it != inputs.end(); it++)
				{
					AbstractWebElement *element = *it;
					if (element != loginElement)
					{
						std::string value;
						std::string name;
						std::string id;
						element->getProperty("value", value);
						element->getAttribute("name", name);
						element->getAttribute("id", id);
						if (name.empty())
							if (!id.empty()) attributes[id] = value;
						else
							attributes[name] = value;
					}
				}
				if (passwords.size() > 0)
				{
					AbstractWebElement *p = passwords.size() < 3 ? passwords.at(0): passwords.at(1);
					std::string value;
					p->getProperty("value", value);
					if (!this->page->updatePassword (account.c_str(), value, msg))
					{
						this->app->alert(MZNC_strtoutf8(msg.c_str()).c_str());
						return;
					}
				}
				if (this->page->updateAttributes (account.c_str(), attributes, msg))
					changeStatus(SF_STATUS_LOCKED);
 			}
		}
	}
	if (this->status == SF_STATUS_LOCKED)
	{
		changeStatus(SF_STATUS_NEW);
	}
}

void SmartForm::onClickModal(AbstractWebElement* element) {
	std::string id = stylePrefix+"_id";
	AbstractWebElement *masterDiv = app->getElementById(id.c_str());
	if (masterDiv != NULL)
	{
		AbstractWebElement *parent = masterDiv->getParent();
		parent->removeChild(masterDiv);
		parent->release();
		masterDiv->release();
	}
	id =  (stylePrefix+"_modal");
	AbstractWebElement *modal = app->getElementById(id.c_str());
	if (modal != NULL)
	{
		AbstractWebElement *parent = modal->getParent();
		parent->removeChild(modal);
		parent->release();
		modal->release();
	}
}

void SmartForm::onClickAccount(AbstractWebElement* element) {
	std::string friendlyName;
	element->getAttribute("_soffid_account", friendlyName);
	AccountStruct as;
	page->getAccountStruct(friendlyName.c_str(), as);
	onClickModal (element);
	fetchAttributes (as);
}

void SmartForm::fetchAttributes(AccountStruct &as)
{
	std::map<std::string,std::string> attributes;
	AbstractWebElement *element = findUsernameInput();

	page->fetchAttributes(app, MZNC_wstrtostr(as.friendlyName.c_str()).c_str(),
			attributes);

	for (std::vector<AbstractWebElement*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		AbstractWebElement *e = *it;
		std::string name;
		std::string id;
		if (e == element)
			e->setProperty("value", MZNC_wstrtoutf8(as.friendlyName.c_str()).c_str());
		else
		{
			e->getAttribute("name", name);
			if (name.empty())
				e->getAttribute("id", name);
			std::string value = attributes.at(name);
			e->setProperty("value", MZNC_strtoutf8(value.c_str()).c_str());
		}
	}
	int i = 0;
	for (std::vector<AbstractWebElement*>::iterator it = passwords.begin(); it != passwords.end(); it++)
	{
		AbstractWebElement *e = *it;
		if (i == 0 && passwords.size() != 2)
		{
			SecretStore s(MZNC_getUserName());
			std::wstring secret = L"pass.";
			secret += as.system;
			secret += L".";
			secret += as.account;
			wchar_t *pass = s.getSecret(secret.c_str());
			if (pass != NULL)
			{
				e->setProperty("value", MZNC_wstrtoutf8(pass).c_str());
				s.freeSecret(pass);
			}
		}
		i ++;
	}
	if (passwords.size() < 2)
		changeStatus(SF_STATUS_LOCKED);
	else
		changeStatus(SF_STATUS_GENERATING);
}

void SmartForm::changeStatus (int status)
{
	MZNSendDebugMessage("New status = %d", status);
	this->status = status;
	for (std::vector<AbstractWebElement*>::iterator it = inputs.begin(); it != inputs.end(); it++)
	{
		AbstractWebElement *e = *it;
		if (status == SF_STATUS_LOCKED)
			e->setAttribute("readOnly", "");
		else
			e->removeAttribute("readOnly");
		AbstractWebElement *img = e->getNextSibling();
		if (img != NULL)
		{
			updateIcon(img, false, false, false);
			img->release();
		}
	}
	int i = 0;
	for (std::vector<AbstractWebElement*>::iterator it = passwords.begin(); it != passwords.end(); it++)
	{
		AbstractWebElement *e = *it;
		if (status == SF_STATUS_LOCKED ||
				status == SF_STATUS_GENERATING && passwords.size() > 2 && i == 0)
			e->setAttribute("readOnly", "");
		else
			e->removeAttribute("readOnly");
		AbstractWebElement *img = (*it)->getNextSibling();
		if (img != NULL)
		{
			updateIcon(img, passwords.size() == 1, passwords.size() == 3 && i == 0, i > 1 || passwords.size() == 2);
			img->release();
		}
		i ++;
	}

}

void SmartForm::onBeforeUnload ()
{
	for (std::vector<AbstractWebElement*>::iterator it = passwords.begin(); it != passwords.end(); it++)
	{
		MZNSendDebugMessage("Hidding element password");
		AbstractWebElement *e = *it;
		e->setAttribute("type", "hidden");
	}
}

void SmartForm::createModal(AbstractWebElement *img)
{
	AbstractWebApplication *app = img->getApplication();
	AbstractWebElement *parent = img->getParent();
	std::string id = stylePrefix+"_id";
	AbstractWebElement *masterDiv = app->getElementById(id.c_str());
	if (masterDiv != NULL)
	{
		parent->removeChild(masterDiv);
		masterDiv->release();
	}

	masterDiv = app->createElement("div");
	long height = getIntProperty (img, "offsetHeight");

	masterDiv->setAttribute("_soffid_modal", "true");
	char ach[1000];
	sprintf (ach, "z-index: 10000; text-align:left; border: solid white 2px; display: inline-block; margin-left:-198px; margin-top: %ldpx; position: absolute; min-width: 200px; "
			"box-shadow: 10px 10px 4px #888888",
			height);
	masterDiv->setAttribute("style", ach);
	masterDiv->setAttribute("id", id.c_str());
	parent->insertBefore(masterDiv, img);
	AbstractWebElement *style = app->createElement("style");
	style->setTextContent((std::string()+
				"." + stylePrefix+ "_selector:hover {background-color: #a6d100;}"
				"." + stylePrefix+ "_selector {"
									"background-color: #5f7993; "
									"cursor: pointer; "
									"padding-left:5px; "
									"padding-right: 5px;"
						  	  	  	"padding-top:3px; "
						  	  	  	"padding-bottom: 3px;}"
				"." + stylePrefix+ "_header {"
						   	   	   "background-color: #17b5c8; "
						   	   	   "padding-left:5px; "
						   	   	   "padding-right: 5px;"
						   	   	   "padding-top:3px; "
						   	   	   "padding-bottom: 3px;}"
				"." + stylePrefix+ "_modal  { "
						   	   	   "position: fixed;"
						   	   	   "top:0; right: 0 ; bottom: 0; left:0;"
						   	   	   "background:black;"
						   	   	   "z-index: 9999;"
						   	   	   "opacity: 0.3;"
						   	   	   "pointer-events: auto;}").c_str());
	masterDiv->appendChild(style);
	style->release();

	AbstractWebElement *user = app->createElement("div");
	user->setAttribute("class", (stylePrefix+ "_header").c_str());
	user->setTextContent("Select account");
	masterDiv->appendChild(user);
	user->release();
	for (std::vector<AccountStruct>::iterator it = page->accounts.begin(); it != page->accounts.end(); it++)
	{
		AccountStruct as = *it;
		std::string friendlyName = MZNC_wstrtostr(as.friendlyName.c_str());
		AbstractWebElement *user = app->createElement("div");
		user->setAttribute("class", (stylePrefix+ "_selector").c_str());
		user->setAttribute("_soffid_account", friendlyName.c_str());
		user->setTextContent(friendlyName.c_str());
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
	modal->setAttribute("class", id.c_str());
	modal->setAttribute("id", id.c_str());
	modal->setAttribute("_soffid_modal", "true");
	masterDiv->appendChild(modal);
	modal->subscribe("click", onClickListener);
	parent->insertBefore(modal, img);
	modal->release();
	masterDiv->release();
	parent->release();
}
