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
#include <vector>
#include <map>
#include "SmartForm.h"

class AccountStruct {
public:
	std::wstring account;
	std::wstring system;
	std::wstring friendlyName;
	AccountStruct () {};
	AccountStruct (const AccountStruct &other) {
		this->account = other.account;
		this->system = other.system;
		this->friendlyName = other.friendlyName;
	}
};

class SmartWebPage: public LockableObject {
public:
	SmartWebPage();
protected:
	virtual ~SmartWebPage();
	bool parsed;

public:
	std::vector<SmartForm*> forms;
	SmartForm *rootForm;

	void parse (AbstractWebApplication *app);
	void upgradeElements (AbstractWebApplication *app);
	void fetchAccounts (AbstractWebApplication *app);
	void fetchAttributes (AbstractWebApplication *app, const char *account, std::map<std::string,std::string> &attributes);
	bool updateAttributes (const char *account, std::map<std::string,std::string> &attributes, std::string &errorMsg);
	bool updatePassword (const char *account, std::string &password, std::string &errorMsg);
	void getAccountStruct (const char *account, AccountStruct &as);
private:
	bool sendSecret (const char *account, const char* sso, std::string &value, std::string &errorMsg);


public:
	std::string defaultSoffidSystem;
	std::string accountDomain;
	std::vector<AccountStruct> accounts;
	bool getAccounts(const char *system, const char *prefix);

};

#endif /* SMARTWEBPAGE_H_ */
