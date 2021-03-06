/*
 * SmartWebPage.h
 *
 *  Created on: 31/08/2015
 *      Author: gbuades
 */

#ifndef SMARTWEBPAGE_H_
#define SMARTWEBPAGE_H_

#include <AbstractWebApplication.h>
#include <AbstractWebElement.h>
#include "WebListener.h"
#include <vector>
#include <map>

class SmartForm;

class AccountStruct {
public:
	std::string id;
	std::wstring account;
	std::wstring system;
	std::wstring friendlyName;
	std::wstring url;
	AccountStruct () {};
	AccountStruct (const AccountStruct &other) {
		this->account = other.account;
		this->system = other.system;
		this->friendlyName = other.friendlyName;
		this->id = other.id;
		this->url = other.url;
	}

	AccountStruct& operator = (const AccountStruct &other) {
		this->account = other.account;
		this->system = other.system;
		this->friendlyName = other.friendlyName;
		this->id = other.id;
		this->url = other.url;
		return *this;
	}
};

class SmartWebPage: public LockableObject {
public:
	SmartWebPage();
protected:
	bool parsed;
	WebListener *listener;
	virtual ~SmartWebPage();

public:
	std::vector<SmartForm*> forms;
	SmartForm *rootForm;

	void formlessParse (AbstractWebApplication *app);
	void parse (AbstractWebApplication *app);
	void fetchAccounts (AbstractWebApplication *app, const char *systemName);
	void fetchAccounts (const char *systemName);
	void fetchAttributes (AbstractWebApplication *app, AccountStruct &as, std::map<std::string,std::string> &attributes);
	bool updateAttributes (AccountStruct &account, std::map<std::string,std::string> &attributes, std::string &errorMsg);
	bool createAccount (const char *description, std::string &msg, AccountStruct &as);
	bool updatePassword (AccountStruct &daccount, std::string &password, std::string &errorMsg);
	void getAccountStruct (const char* id, AccountStruct &as);
	virtual std::string toString () ;
	std::string getAccountURL (AccountStruct &account);
	bool isAnyAttributeNamed (const char *attName);
	void setURL (const char *url) {
		this->url = url;
	}
private:
	bool sendSecret (AccountStruct &account, const char* sso, std::string &value, std::string &errorMsg);
	void parse (AbstractWebApplication *app, PageData* data, bool formless);


public:
	std::string defaultSoffidSystem;
	std::string accountDomain;
	std::string url;
	std::vector<AccountStruct> accounts;
	std::vector<std::string> accountAttributes;
	bool getAccounts(const char *system, const char *prefix);

};

#endif /* SMARTWEBPAGE_H_ */
