/*
 * SmartForm.h
 *
 *  Created on: 31/08/2015
 *      Author: gbuades
 */

#ifndef SMARTFORM_H_
#define SMARTFORM_H_

#include <AbstractWebApplication.h>
#include <AbstractWebElement.h>
#include <vector>
#include <map>

class SmartWebPage;
class AbstractWebApplication;
class AbstractWebElement;
class OnChangeListener;
class OnClickListener;
class OnBeforeUnloadListener;
class AccountStruct;

#define SF_STATUS_NEW 0
#define SF_STATUS_LOCKED 1
#define SF_STATUS_MODIFYING 2
#define SF_STATUS_GENERATING 3


class SmartForm: public LockableObject {
public:
	SmartForm(SmartWebPage *page);
	AbstractWebApplication *getApp(){return app;}
	AbstractWebElement *getElement() {return element;}
	void onChange (AbstractWebElement *element);
	void onClickImage (AbstractWebElement *element);
	void onClickAccount (AbstractWebElement *element);
	void onClickModal (AbstractWebElement *element);
	void createModal(AbstractWebElement *img);
	void onBeforeUnload ();

protected:
	virtual ~SmartForm();
	SmartWebPage *page;
	AbstractWebApplication *app;
	AbstractWebElement *element;
	void addIcon (AbstractWebElement *input, bool isPassword, bool isOldPassword, bool isNewPassword);
	void updateIcon (AbstractWebElement *input, bool isPassword, bool isOldPassword, bool isNewPassword);
	int status;
	void changeStatus (int status);

	OnChangeListener *onChangeListener;
	OnClickListener *onClickListener;
	OnBeforeUnloadListener *onBeforeUnloadListener;
	void releaseElements ();
	void fetchAttributes (AccountStruct &as);

	std::string stylePrefix;

public:
	std::vector<AbstractWebElement*> passwords;
	std::vector<AbstractWebElement*> inputs;
	std::vector<AbstractWebElement*> submits;
	void parse (AbstractWebApplication *app, AbstractWebElement *formRoot);

private:
	AbstractWebElement *findUsernameInput();
};

#endif /* SMARTFORM_H_ */
