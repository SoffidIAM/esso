/*
 * SmartWebPage.cpp
 *
 *  Created on: 31/08/2015
 *      Author: gbuades
 */

#include "SmartWebPage.h"
#include <MazingerInternal.h>
#include <SecretStore.h>
#include <string.h>
#include <ScriptDialog.h>
#include <SeyconServer.h>

#include <img_password.h>
#include <img_save.h>
#include <img_generate.h>
#include <img_unlock.h>
#include <img_user.h>
#include <SmartForm.h>

#include <stdio.h>

#include <algorithm>
#include <vector>

SmartWebPage::SmartWebPage() {
	parsed = false;
	listener = NULL;
	rootForm = new SmartForm(this);
}

SmartWebPage::~SmartWebPage() {
	if (rootForm !=NULL)
		rootForm->release();
	for (std::vector<SmartForm*>::iterator it = forms.begin(); it != forms.end(); it++)
	{
		SmartForm *sf = *it;
		sf->release();
	}
}

class OnAnyElementFocusListener: public WebListener
{
public:
	virtual std::string toString () { return std::string("OnAnyElementFocusListener");}
	SmartWebPage *page;
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component);
};

void OnAnyElementFocusListener::onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component) {
	if (MZNC_waitMutex())
	{
		component->sanityCheck();
		std::string managed = "";
		std::string tag = "";
		component->getTagName(tag);
		component->getProperty("soffidManaged", managed);
		if (managed != "true" && strcasecmp (tag.c_str(), "input") == 0)
		{
			app->sanityCheck();
			if (page != NULL)
			{
				page->sanityCheck();
				page -> parse (app);
			}
		}
		MZNC_endMutex();
	}

}

void SmartWebPage::parse(AbstractWebApplication* app) {
	std::vector<AbstractWebElement*> forms;

	if (! parsed)
	{
		OnAnyElementFocusListener* wl;
		wl = new OnAnyElementFocusListener ();
		wl->page = this;

		listener = wl;

		std::vector<AbstractWebElement *> bodies;
		app->getElementsByTagName("body", bodies);
		for (std::vector<AbstractWebElement*>::iterator it = bodies.begin(); it != bodies.end (); it++)
		{
			(*it)->subscribe("focus", wl);
			parsed = true;
		}
		wl->release();
	}

	MZNSendDebugMessageA("* Parsing HTML document");
	PageData *data = app->getPageData();
	if (data != NULL)
	{
		data->dump();
		MZNSendDebugMessageA("* Parsing formless inputs");
		if (! rootForm->isParsed())
			rootForm->parse(app, NULL, &data->inputs);
		else
			rootForm->reparse(&data->inputs);

		for (std::vector<FormData>::iterator it = data->forms.begin(); it != data->forms.end(); it++)
		{
			FormData &form = *it;
			MZNSendDebugMessageA("* Parsing Form %s", form.soffidId.c_str());
			AbstractWebElement *element = app->getElementBySoffidId(form.soffidId.c_str());

			bool found = false;
			for (std::vector<SmartForm*>::iterator it2 = this->forms.begin(); it2 != this->forms.end(); it2++)
			{
				SmartForm *form2 = *it2;
				if (form2->getRootElement() != NULL && form2->getRootElement()->equals(element))
				{
					form2->reparse(&form.inputs);
					found = true;
					break;
				}
			}
			if (! found)
			{
				SmartForm *smartForm = new SmartForm(this);
				this->forms.push_back(smartForm);
				smartForm->parse ( app, element, &form.inputs);
			}
			element->release();
		}
	} else {
		if (! rootForm->isParsed())
			rootForm->parse(app, NULL, NULL);
		else
			rootForm->reparse(NULL);
		MZNSendDebugMessageA("* Parsed form");


		app->getForms(forms);

		int i = 0;
		for (std::vector<AbstractWebElement*>::iterator it = forms.begin(); it != forms.end(); it++)
		{
			AbstractWebElement *element = *it;
			std::string action;
			element->getAttribute("action", action);
			bool found = false;
			for (std::vector<SmartForm*>::iterator it2 = this->forms.begin(); it2 != this->forms.end(); it2++)
			{
				SmartForm *form2 = *it2;
				if (form2->getRootElement() != NULL && form2->getRootElement()->equals(element))
				{
					form2->reparse(NULL);
					found = true;
					break;
				}
			}
			if (! found)
			{
				SmartForm *form = new SmartForm(this);
				this->forms.push_back(form);
				form->parse ( app, element, NULL);
			}
			element->release();
		}
	}


}


static std::wstring searchInVector (std::vector<std::pair<std::wstring, std::wstring> > &vector, const std::wstring &key)
{
	std::wstring result;
	for (std::vector<std::pair<std::wstring, std::wstring> >::iterator it = vector.begin();
			it != vector.end();
			it++)
	{
		if (it->first == key)
		{
			result = it->second;
			break;
		}
	}
	return result;
}

/**
 *
 * system => Soffid agent name. User created accounts as in sso system
 * targetSystem => For user created accounts, contains the actual target domain name. Is empty for administrator created agents
 *
 */
bool SmartWebPage::getAccounts(const char *system, const char *targetSystem) {
	MZNSendDebugMessageA("** Fetching accounts for [%s]", system);
	std::wstring wTargetSystem;
	if (targetSystem != NULL)
		wTargetSystem = MZNC_strtowstr(targetSystem);

	SecretStore s (MZNC_getUserName());

	std::wstring secret = L"account.";
	secret += MZNC_strtowstr(system);

	std::vector<std::wstring> accountNames = s.getSecrets(secret.c_str());
	if (accountNames.empty())
	{
		return false;
	}

	std::vector<std::pair<std::wstring, std::wstring> > descriptions =
			s.getSecretsByPrefix2((std::wstring(L"accdesc.")+MZNC_strtowstr(system)+L".").c_str());
	std::vector<std::pair<std::wstring, std::wstring> > serverAttributes;
	std::wstring attPrefix ;
	attPrefix = std::wstring(L"sso.")+MZNC_strtowstr(system)+L".";
	serverAttributes = s.getSecretsByPrefix2(attPrefix.c_str());


	for (std::vector<std::wstring>::iterator it = accountNames.begin(); it != accountNames.end(); it++)
	{

		std::wstring account = *it;
		std::wstring attPrefix2 = attPrefix+account+L".";
		std::wstring secret2 = attPrefix2+L"Server";
		std::wstring server = searchInVector(serverAttributes, secret2);
		if (wTargetSystem.empty() || server == wTargetSystem )
		{

			char id[10];
			sprintf (id, "%d", accounts.size()+1);
			AccountStruct as;
			as.id = id;
			as.account = account;
			as.system = MZNC_strtowstr(system);
			std::wstring descEntry = L"accdesc.";
			descEntry += as.system + L"."+ account;

			std::wstring description;
			for (std::vector<std::pair<std::wstring, std::wstring> >::iterator it = descriptions.begin();
					it != descriptions.end();
					it++)
			{
				if (it->first == descEntry)
				{
					description = it->second;
					break;
				}
			}
			if (description.empty())
				as.friendlyName = account;
			else
				as.friendlyName = description;


			accounts.push_back(as);
		}
		// Fetch attributes
		int attPrefix2Len = attPrefix2.length();
		for (std::vector<std::pair<std::wstring, std::wstring> >::iterator it = serverAttributes.begin() ;
				it != serverAttributes.end () ; it ++)
		{
			if (it->first.length() > attPrefix2Len &&
					it -> first.substr(0, attPrefix2Len) == attPrefix2 )
			{
				std::wstring attName = it->first.substr(attPrefix2Len, std::string::npos).c_str();
				if (attName != L"Server" && attName != L"URL");
				{
					std::wstring value = it->second;
					size_t i = value.find('=');
					if ( i != std::string::npos)
					{
						std::string split1 = MZNC_wstrtostr(SeyconCommon::urlDecode(MZNC_wstrtoutf8(value.substr(0, i).c_str()).c_str()).c_str());
						std::string split2 = MZNC_wstrtostr(SeyconCommon::urlDecode(MZNC_wstrtoutf8(value.substr(i+1).c_str()).c_str()).c_str());
						if ( ! isAnyAttributeNamed (split1.c_str()))
						{
							MZNSendDebugMessageA("Registering attribute %s", split1.c_str());
							accountAttributes.push_back(split1);
						}
					}
				}

			}
		}
	}

	return true;
}


void SmartWebPage::fetchAccounts(AbstractWebApplication *app, const char *systemName) {
	MZNSendDebugMessageA("* Fetching accounts for %s", systemName);
	accounts.clear();
	app->getUrl(url);
	size_t i = url.find("://");
	if ( i != std::string::npos)
	{
		std::string host = url.substr(i+3);
		i = host.find("/");
		if ( i  != std::string::npos) host = host.substr (0, i);
		i = host.find(':');
		if ( i  != std::string::npos) host = host.substr (0, i);

		accountDomain = host;

		if (systemName != NULL)
		{
			getAccounts(systemName, "");
		} else {
			std::string subhost = host;



			do
			{
				getAccounts(subhost.c_str(), "");
				i = subhost.find('.');
				if (subhost.find('.') == std::string::npos)
					break;
				subhost = subhost.substr(i+1);
				if (subhost.find('.') == std::string::npos)
					break;
			} while (true);

		}
		SeyconCommon::readProperty("AutoSSOSystem", defaultSoffidSystem);

		std::string prefix = host;
		getAccounts(defaultSoffidSystem.c_str(), prefix.c_str());
	}

}


void SmartWebPage::getAccountStruct (const char* id, AccountStruct &as)
{
	for (std::vector<AccountStruct>::iterator it = accounts.begin(); it != accounts.end(); it++)
	{
		AccountStruct as2 = *it;
		if (as2.id  == id)
		{
			as = as2;
			return;
		}
	}
	as.system = MZNC_strtowstr(defaultSoffidSystem.c_str());
	as.friendlyName = L"New account";
	as.account = MZNC_strtowstr(accountDomain.c_str())+L"/"+as.friendlyName;
}


bool SmartWebPage::isAnyAttributeNamed (const char *attName)
{

	std::vector<std::string>::iterator it = std::find ( accountAttributes.begin (), accountAttributes.end(), std::string(attName));

	return it != accountAttributes.end ();

}

void SmartWebPage::fetchAttributes(AbstractWebApplication *app, AccountStruct &as, std::map<std::string,std::string> &attributes) {
	std::string host;
	SecretStore s (MZNC_getUserName());

	std::wstring secret = L"sso.";
	secret += as.system;
	secret += L".";
	secret += as.account;
	secret += L".";


	std::map<std::wstring,std::wstring> atts = s.getSecretsByPrefix(secret.c_str());
	for (std::map<std::wstring,std::wstring>::iterator it = atts.begin(); it != atts.end(); it ++)
	{
		std::wstring att = it->first;
		std::wstring value = it->second;
		if (att != secret + L"Server" && att != secret + L"URL");
		{
			size_t i = value.find('=');
			if ( i != std::string::npos)
			{
				std::string split1 = MZNC_wstrtostr(SeyconCommon::urlDecode(MZNC_wstrtoutf8(value.substr(0, i).c_str()).c_str()).c_str());
				std::string split2 = MZNC_wstrtostr(SeyconCommon::urlDecode(MZNC_wstrtoutf8(value.substr(i+1).c_str()).c_str()).c_str());
				attributes[split1] = split2;
//				MZNSendDebugMessage("Attribute %s=%s", split1.c_str(),split2.c_str());
			}
		}
	}
}

std::string SmartWebPage::getAccountURL(AccountStruct &as) {
	SecretStore s (MZNC_getUserName());

	std::wstring secret = L"sso.";
	secret += as.system;
	secret += L".";
	secret += as.account;
	secret += L".";
	secret += L"URL";


	wchar_t* secretValue = s.getSecret(secret.c_str());
	std::string url = MZNC_wstrtoutf8(secretValue);
	s.freeSecret(secretValue);

	return url;
}

static std::string storeSecret(AccountStruct &as, const char *tag, const char *value, std::wstring &encodedSecret)
{
	SecretStore s (MZNC_getUserName()) ;

	std::string attributeNumber;

	std::wstring secretName = L"sso.";
	secretName += as.system;
	secretName += L".";
	secretName += as.account;
	secretName += L".";


	std::string encodedTag = SeyconCommon::urlEncode(MZNC_strtowstr(tag).c_str());

	encodedSecret = MZNC_strtowstr(encodedTag.c_str());
	encodedSecret += L"=";
	encodedSecret += MZNC_strtowstr(SeyconCommon::urlEncode(MZNC_strtowstr(value).c_str()).c_str());

	bool found = false;

	int counter = 1;

	std::map<std::wstring,std::wstring> atts = s.getSecretsByPrefix(secretName.c_str());
	for (std::map<std::wstring,std::wstring>::iterator it = atts.begin(); it != atts.end(); it ++)
	{
		int d = 0;
		std::wstring sn = it->first;
		std::wstring sv = it->second;
		attributeNumber = MZNC_wstrtostr(sn.substr(secretName.length()).c_str());
		sscanf (attributeNumber.c_str(), "%d", &d);
		if (d >= counter) counter = d+1;
		size_t i = sv.find('=');
		if ( i != std::string::npos)
		{
			std::string tag2 = MZNC_wstrtostr(sv.substr(0, i).c_str());
			if (tag2 == encodedTag)
			{
				s.setSecret(sn.c_str(), encodedSecret.c_str());
				found = true;
				break;
			}
		}
	}

	if (! found)
	{
		char ach[20];
		sprintf (ach, "%d", counter);
		secretName += MZNC_strtowstr(ach);
		s.putSecret(secretName.c_str(), encodedSecret.c_str());
		attributeNumber = ach;
	}
	return attributeNumber;
}


bool SmartWebPage::sendSecret (AccountStruct &as, const char* tag, std::string &value, std::string &errorMsg)
{
	SecretStore s (MZNC_getUserName()) ;
	wchar_t *sessionKey = s.getSecret(L"sessionKey");
	wchar_t *user = s.getSecret(L"user");

	std::wstring wValue = MZNC_strtowstr(value.c_str());
	SeyconService ss;
	SeyconResponse *response = ss.sendUrlMessage(L"/setSecret?user=%ls&key=%ls&system=%ls&account=%hs&value=%hs&sso=%ls",
			user, sessionKey, as.system.c_str(),
			SeyconCommon::urlEncode(as.account.c_str()).c_str(),
			SeyconCommon::urlEncode(wValue.c_str()).c_str(),
			ss.escapeString(tag).c_str());

	s.freeSecret(sessionKey);
	s.freeSecret(user);

	bool ok = false;
	if (response == NULL)
	{
		errorMsg = "Cannot contact Soffid server";
	}
	else
	{
		std::string token = response->getToken(0);
		if (token != "OK")
		{
			errorMsg = response->getToken(0);
			errorMsg += ": ";
			errorMsg += response->getToken(1);
		}
		else
		{
			if (tag == NULL || tag[0] == '\0'){

				SecretStore s (MZNC_getUserName()) ;

				std::wstring wPass = MZNC_strtowstr(value.c_str());
				std::wstring secret = L"pass.";
				secret += as.system;
				secret += L".";
				secret += as.account;

				s.setSecret(secret.c_str(), wPass.c_str());
			}
			ok = true;
		}
		delete response;
	}
	return ok;

}


bool SmartWebPage::updateAttributes (AccountStruct &account, std::map<std::string,std::string> &attributes, std::string &errorMsg)
{
	for (std::map<std::string,std::string>::iterator it = attributes.begin(); it != attributes.end(); it++)
	{
		std::wstring encodedSecret;
		std::string attributeId = storeSecret(account, it->first.c_str(), it->second.c_str(), encodedSecret);
		std::string encodedSecretUtf8 = MZNC_wstrtostr(encodedSecret.c_str());
		if (! sendSecret(account, attributeId.c_str(), encodedSecretUtf8, errorMsg))
			return false;
	}
	if (getAccountURL(account).size() == 0)
	{
		if (! sendSecret(account, "URL", this->url, errorMsg) )
			return false;
		SecretStore ss(MZNC_getUserName());
		ss.setSecret((std::wstring(L"sso.")+account.system+L"."+account.account+L".URL").c_str(),
				MZNC_utf8towstr(url.c_str()).c_str());
	}
	return true;
}

bool SmartWebPage::updatePassword (AccountStruct &account, std::string &password, std::string &errorMsg)
{
	if (sendSecret(account, "", password, errorMsg))
	{
		return true;
	}
	else
	{
		return false;
	}


}

std::string SmartWebPage::toString() {
	std::string s = "SmartWebPage ";
	if (rootForm != NULL)
	{
		rootForm->sanityCheck();
		s += rootForm -> toString();
	}
	return s;
}

bool SmartWebPage::createAccount (const  char *descr, std::string &errorMsg, AccountStruct &as)
{
	SecretStore s (MZNC_getUserName()) ;
	wchar_t *sessionKey = s.getSecret(L"sessionKey");
	wchar_t *user = s.getSecret(L"user");
	std::string ssoSystem;
	SeyconCommon::readProperty("AutoSSOSystem", ssoSystem);


	SeyconService ss;
	SeyconResponse *response = ss.sendUrlMessage(L"/setSecret?user=%ls&key=%ls&system=%hs&description=%ls",
			user, sessionKey, ssoSystem.c_str(),
			MZNC_strtowstr(descr).c_str());

	s.freeSecret(sessionKey);
	s.freeSecret(user);

	bool ok = false;
	if (response == NULL)
	{
		errorMsg = "Cannot contact Soffid server";
	}
	else
	{
		std::string token = response->getToken(0);
		if (token != "OK")
		{
			errorMsg = response->getToken(0);
			errorMsg += ": ";
			errorMsg += response->getToken(1);
		}
		else
		{
			char ach[10];
			sprintf (ach, "%d", accounts.size()+1);
			as.id = ach;
			as.account = response->getWToken(1);
			as.friendlyName = MZNC_strtowstr(descr);
			as.system = MZNC_strtowstr(ssoSystem.c_str());
			accounts.push_back(as);
			ok = sendSecret(as, "Server", this->accountDomain, errorMsg);
			ok = ok && sendSecret(as, "URL", this->url, errorMsg);
			if (ok)
			{
				std::wstring secretName = L"sso.";
				secretName += as.system;
				secretName += L".";
				secretName += as.account;
				secretName += L".";

				s.setSecret((secretName+L"Server").c_str(), MZNC_utf8towstr(this->accountDomain.c_str()).c_str());
				s.setSecret((secretName+L"URL").c_str(), MZNC_utf8towstr(this->url.c_str()).c_str());

				secretName = L"account.";
				secretName += as.system;
				s.putSecret(secretName.c_str(), as.account.c_str());

				secretName = L"accdesc.";
				secretName += as.system;
				secretName += L".";
				secretName += as.account;
				s.putSecret(secretName.c_str(), as.friendlyName.c_str());

				this->accounts.push_back(as);
			}
		}
		delete response;
	}

	return ok;

}
