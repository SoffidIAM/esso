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

	fetchAccounts(app);

	std::vector<AbstractWebElement*> forms;

	app->getForms(forms);

	if (parsed)
	{
		if (forms.size() == 0)
		{
			rootForm->parse(app, app->getDocumentElement());
		}
		else
		{
			int i = 0;
			for (std::vector<AbstractWebElement*>::iterator it = forms.begin(); it != forms.end(); it++)
			{
				bool found = false;
				AbstractWebElement *element = *it;
				for (std::vector<SmartForm*>::iterator it2 = this->forms.begin(); it2 != this->forms.end(); it2++)
				{
					SmartForm *form2 = *it2;
					if (element->equals(form2->getElement()))
					{
						form2 -> parse(app, element);
						found = true;
						break;
					}
				}
				if ( ! found)
				{
					SmartForm *form = new SmartForm(this);
					this->forms.push_back(form);
					form->parse ( app, *it);
				}
			}
		}
	} else {
		if (forms.size() == 0)
		{
			rootForm->parse(app, app->getDocumentElement());
		}
		else
		{
			for (std::vector<AbstractWebElement*>::iterator it = forms.begin(); it != forms.end(); it++)
			{
				SmartForm *form = new SmartForm(this);
				this->forms.push_back(form);
				form->parse ( app, *it);
			}
		}
	}
	parsed = true;


}



bool SmartWebPage::getAccounts(const char *system, const char *prefix) {
	std::string host;
	SecretStore s (MZNC_getUserName());

	std::wstring secret = L"account.";
	secret += MZNC_strtowstr(system);

	std::vector<std::wstring> accountNames = s.getSecrets(secret.c_str());

	if (accountNames.empty())
		return false;

	int prefixLength = strlen (prefix);
	for (std::vector<std::wstring>::iterator it = accountNames.begin(); it != accountNames.end(); it++)
	{
		std::wstring account = *it;
		if ( strncmp (MZNC_wstrtostr(account.c_str()).c_str(), prefix, prefixLength) == 0)
		{
			AccountStruct as;
			as.account = account;
			as.system = MZNC_strtowstr(system);
			as.friendlyName = account.substr(strlen(prefix));
			accounts.push_back(as);
		}
	}
	return true;
}


void SmartWebPage::fetchAccounts(AbstractWebApplication *app) {
	accounts.clear();
	std::string url;
	app->getUrl(url);
	size_t i = url.find("://");
	if ( i != std::string::npos)
	{
		std::string host = url.substr(i+3);
		i = host.find(':');
		if ( i  != std::string::npos) host = host.substr (0, i);
		std::string subhost = host;

		accountDomain = subhost;

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

		SeyconCommon::readProperty("AutoSSOSystem", defaultSoffidSystem);

		std::string prefix = host + "/";
		getAccounts(defaultSoffidSystem.c_str(), prefix.c_str());
	}
}


int hextoint (char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	else if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else
		return -1;
}

char inttohex (int i)
{
	if (i < 10)
		return '0' + i;
	else
		return 'a' + i - 10;
}

static std::string unscape(const char* sz) {
	std::string result;
	int i = 0;
	while (sz[i])
	{
		if (sz[i] == '%')
		{
			int hex1 = hextoint (sz[++i]);
			if (hex1 >= 0)
			{
				int hex2 = hextoint (sz[++i]);
				if (hex2 >= 0)
				{
					i++;
					result += (char) (hex1 << 4 | hex2);
				}
			}
		}
		else if (sz[i] == '+')
		{
			result += ' ';
			i++;
		}
		else
		{
			result += sz[i++];
		}
	}
	return result;
}

static std::string scape(const char* sz) {
	std::string result;
	int i = 0;
	while (sz[i])
	{
		if (sz[i] == ' ')
			result += "+";
		else if (sz[i] < 'a' || sz[i] > 'z' && sz[i] < 'A' || sz[i] > 'Z' )
		{
			int s = (int) sz[i];
			if (s < 0) s += 256;
			result += "%";
			result += inttohex ( s % 16);
			result += inttohex ( s / 16);
		}
		else
			result += sz[i];
		i++;
	}
	return result;
}

void SmartWebPage::getAccountStruct (const char *account, AccountStruct &as)
{
	for (std::vector<AccountStruct>::iterator it = accounts.begin(); it != accounts.end(); it++)
	{
		AccountStruct as2 = *it;
		if (as2.friendlyName == MZNC_strtowstr(account))
		{
			as.account = as2.account;
			as.friendlyName = as2.friendlyName;
			as.system = as2.system;
			return;
		}
	}
	as.system = MZNC_strtowstr(defaultSoffidSystem.c_str());
	as.friendlyName = MZNC_strtowstr(account);
	as.account = MZNC_strtowstr(accountDomain.c_str())+L"/"+as.friendlyName;
}

void SmartWebPage::fetchAttributes(AbstractWebApplication *app, const char *account, std::map<std::string,std::string> &attributes) {
	std::string host;
	SecretStore s (MZNC_getUserName());
	AccountStruct as;

	getAccountStruct (account, as);

	std::wstring secret = L"sso.";
	secret += as.system;
	secret += L".";
	secret += as.account;


	std::vector<std::wstring> atts = s.getSecrets(secret.c_str());
	for (std::vector<std::wstring>::iterator it = atts.begin(); it != atts.end(); it ++)
	{
		std::wstring att = *it;
		size_t i = att.find('=');
		if ( i != std::string::npos)
		{
			std::string tag = MZNC_utf8tostr(unscape(MZNC_wstrtostr(att.substr(0, i).c_str()).c_str()).c_str());
			std::string value = MZNC_utf8tostr(unscape(MZNC_wstrtostr(att.substr(i+1).c_str()).c_str()).c_str());
			attributes[tag]= value;
		}
	}
}

bool SmartWebPage::sendSecret (const char *account, const char* tag, std::string &value, std::string &errorMsg)
{
	SecretStore s (MZNC_getUserName()) ;
	wchar_t *sessionKey = s.getSecret(L"sessionKey");
	wchar_t *user = s.getSecret(L"user");
	AccountStruct as;

	getAccountStruct (account, as);

	std::wstring wValue = MZNC_strtowstr(value.c_str());
	SeyconService ss;
	MZNSendDebugMessage("Storing secret /setSecret?user=%ls&key=%ls&system=%ls&account=%ls&value=%ls&sso=%hs",
			user, sessionKey, as.system.c_str(),
			as.account.c_str(),
			wValue.c_str(),
			tag);
	SeyconResponse *response = ss.sendUrlMessage(L"/setSecret?user=%ls&key=%ls&system=%ls&account=%ls&value=%ls&sso=%hs",
			user, sessionKey, as.system.c_str(),
			as.account.c_str(),
			wValue.c_str(),
			tag);

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
			if (tag == NULL){

				SecretStore s (MZNC_getUserName()) ;

				std::wstring wPass = MZNC_strtowstr(value.c_str());
				std::wstring secret = L"pass.";
				secret += as.system;
				secret += L".";
				secret += as.account;

				s.setSecret(secret.c_str(), wPass.c_str());
			} else {
				SecretStore s (MZNC_getUserName()) ;

				std::wstring secret = L"sso.";
				secret += as.system;
				secret += L".";
				secret += as.account;

				std::vector<std::wstring> atts = s.getSecrets(secret.c_str());
				for (std::vector<std::wstring>::iterator it = atts.begin(); it != atts.end(); it ++)
				{
					std::wstring att = *it;
					size_t i = att.find('=');
					if ( i != std::string::npos)
					{
						std::string tag2 = MZNC_utf8tostr(unscape(MZNC_wstrtostr(att.substr(0, i).c_str()).c_str()).c_str());
						if (tag2 == tag)
						{
							s.removeSecret(secret.c_str(), att.c_str());
						}
					}
				}

				std::wstring newSecret = MZNC_strtowstr(scape(MZNC_strtoutf8(tag).c_str()).c_str());
				newSecret += L"=";
				newSecret += MZNC_strtowstr(scape(MZNC_strtoutf8(value.c_str()).c_str()).c_str());

				s.putSecret(secret.c_str(), newSecret.c_str());
			}
			ok = true;
		}
		delete response;
	}
	return ok;

}


bool SmartWebPage::updateAttributes (const char *account, std::map<std::string,std::string> &attributes, std::string &errorMsg)
{
	for (std::map<std::string,std::string>::iterator it = attributes.begin(); it != attributes.end(); it++)
	{
		if (! sendSecret(account, it->first.c_str(), it->second, errorMsg))
			return false;
	}
	return true;
}

bool SmartWebPage::updatePassword (const char *account, std::string &password, std::string &errorMsg)
{
	if (sendSecret(account, "", password, errorMsg))
	{
		return true;
	}
	else
		return false;


}
