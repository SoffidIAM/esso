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
#include <SmartWebPage.h>


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
#define SF_STATUS_SELECT 4


enum InputType {
	IT_GENERAL = 0,
	IT_PASSWORD,
	IT_NEW_PASSWORD
};

enum InputStatus {
	IS_EMPTY = 0,
	IS_SELECT,
	IS_LOCKED,
	IS_MODIFIED,
};

class InputDescriptor {
public:
	AbstractWebElement *input;
	AbstractWebElement *img;
	InputType type;
	InputStatus status;
};
class SmartForm: public LockableObject {
public:
	SmartForm(SmartWebPage *page);
	AbstractWebApplication *getApp(){return app;}
	void onChange (AbstractWebElement *element);
	void onClickImage (AbstractWebElement *element);
	void onClickLevel (AbstractWebElement *element);
	void onClickSave (AbstractWebElement *element);
	void onClickAccount (AbstractWebElement *element);
	void onClickModal (AbstractWebElement *element);
	void createModal(AbstractWebElement *img);
	void createGenerateModal(AbstractWebElement *img);
	void createSaveModal(AbstractWebElement *img);
	void onBeforeUnload ();
	AbstractWebElement *getRootElement () {return element;}
	virtual std::string toString ();
	AccountStruct currentAccount;

protected:
	void createStyle ();
	void save ();
	bool detectAttributeChange ();
	virtual ~SmartForm();
	SmartWebPage *page;
	AbstractWebApplication *app;
	AbstractWebElement *element;
	void addIcon (InputDescriptor *input);
	void updateIcon (InputDescriptor *input);
	void removeIcon (InputDescriptor *input);
	int status;
	void changeStatus (int status);

	OnChangeListener *onChangeListener;
	OnClickListener *onClickListener;
	OnBeforeUnloadListener *onBeforeUnloadListener;
	void releaseElements ();
	void fetchAttributes (AccountStruct &as);
	bool addNoDuplicate (AbstractWebElement* input);
	InputDescriptor* findInputDescriptor (AbstractWebElement *element);
	bool checkAnyPassword (std::vector<AbstractWebElement*> &elements);


	std::string stylePrefix;

public:
	unsigned int numPasswords;
	std::vector<InputDescriptor*> inputs;
	std::vector<AbstractWebElement*> submits;
	void parse (AbstractWebApplication *app, AbstractWebElement *formRoot);
	void reparse ();
};

#endif /* SMARTFORM_H_ */
