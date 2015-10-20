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

SmartWebPage::SmartWebPage() {
	parsed = false;
	rootForm = new SmartForm(this);
}

SmartWebPage::~SmartWebPage() {
	rootForm->release();
	for (std::vector<SmartForm*>::iterator it = forms.begin(); it != forms.end(); it++)
	{
		SmartForm *sf = *it;
		sf->release();
	}
}


void SmartWebPage::parse(AbstractWebApplication* app) {

	std::vector<AbstractWebElement*> forms;

	app->getForms(forms);

	if (app->getDocumentElement() != NULL)
	{
		MZNSendDebugMessageA("* Parsing HTML document");
		if (rootForm->getRootElement() == NULL)
			rootForm->parse(app, app->getDocumentElement());
		else
			rootForm->reparse();
	}

	int i = 0;
	for (std::vector<AbstractWebElement*>::iterator it = forms.begin(); it != forms.end(); it++)
	{
		MZNSendDebugMessageA("* Parsing HTML form %d", i++);
		bool found = false;
		AbstractWebElement *element = *it;
		for (std::vector<SmartForm*>::iterator it2 = this->forms.begin(); it2 != this->forms.end(); it2++)
		{
			SmartForm *form2 = *it2;
			if (form2->getRootElement() != NULL && form2->getRootElement()->equals(element))
			{
				form2->reparse();
				found = true;
				break;
			}
		}
		if (! found)
		{
			SmartForm *form = new SmartForm(this);
			this->forms.push_back(form);
			form->parse ( app, element);
		}
	}


}



bool SmartWebPage::getAccounts(const char *system, const char *prefix) {
	MZNSendDebugMessageA("** Fetching accounts for [%s]", system);
	std::wstring wPrefix;
	if (prefix != NULL)
		wPrefix = MZNC_strtowstr(prefix);

	SecretStore s (MZNC_getUserName());

	std::wstring secret = L"account.";
	secret += MZNC_strtowstr(system);

	std::vector<std::wstring> accountNames = s.getSecrets(secret.c_str());

	if (accountNames.empty())
	{
		return false;
	}

	int prefixLength = strlen (prefix);
	for (std::vector<std::wstring>::iterator it = accountNames.begin(); it != accountNames.end(); it++)
	{
		std::wstring account = *it;
		std::wstring secret2 = std::wstring(L"sso.")+MZNC_strtowstr(defaultSoffidSystem.c_str())+L"."+account+L".Server";
		wchar_t* server = s.getSecret(secret2.c_str());
		if (server != NULL && wcscmp (server, wPrefix.c_str()) == 0 ||
				prefix == NULL)
		{
			char id[10];
			sprintf (id, "%d", accounts.size()+1);
			AccountStruct as;
			as.id = id;
			as.account = account;
			as.system = MZNC_strtowstr(system);
			std::wstring prefix = L"accdesc.";
			wchar_t* fn =  s.getSecret( (prefix + as.system + L"."+ account).c_str());;
			if (fn == NULL || fn[0] == '\0')
				as.friendlyName = account;
			else
				as.friendlyName = fn;
			s.freeSecret(fn);
			accounts.push_back(as);
			MZNSendDebugMessageA("*** Found account %ls", account.c_str());
		}
		if (server != NULL) s.freeSecret(server);
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
		wchar_t ach[10];
		swprintf (ach, 10, L"%d", counter);
		secretName += ach;
		s.putSecret(secretName.c_str(), encodedSecret.c_str());
		attributeNumber = MZNC_wstrtostr(ach);
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
				this->accounts.push_back(as);
			}
		}
		delete response;
	}

	return ok;

}
