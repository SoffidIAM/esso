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
#include <PageData.h>

class SmartWebPage;
class AbstractWebApplication;
class AbstractWebElement;
class OnChangeListener;
class OnClickListener;
class OnBeforeUnloadListener;
class OnHiddenElementFocusListener;
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

	~InputDescriptor ();
	InputDescriptor ();
	AbstractWebElement *input;
	AbstractWebElement *img;
	InputType type;
	InputStatus status;
	bool hasInputData;
	InputData data;

	long getClientHeight();
	long getClientWidth ();
	std::string getDataBind ();
	std::string getDisplay ();
	std::string getId ();
	std::string getName();
	std::string getType();
	long getOffsetHeight();
	long getOffsetLeft();
	long getOffsetTop();
	long getOffsetWidth();
	bool isRightAlign ();
	bool isVisible ();
	std::string getStyle ();
	std::string getTextAlign ();
	std::string getValue();
	std::string getVisibility ();
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
	void onFocus (AbstractWebElement *element);
	void createModal(AbstractWebElement *img);
	void createGenerateModal(AbstractWebElement *img);
	void createSaveModal(AbstractWebElement *img);
	void onBeforeUnload ();
	bool isParsed () { return parsed; }
	AbstractWebElement *getRootElement () {return element;}
	virtual std::string toString ();
	AccountStruct currentAccount;

protected:
	bool parsed;
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
	OnHiddenElementFocusListener *onHiddenElementFocusListener;
	void releaseElements ();
	void fetchAttributes (AccountStruct &as, AbstractWebElement *selectedElement);
	bool addNoDuplicate (InputDescriptor *data);
	InputDescriptor* findInputDescriptor (AbstractWebElement *element);
	bool checkAnyPassword (std::vector<InputDescriptor*> &elements);
	void findInputs (AbstractWebApplication* app, AbstractWebElement *element, std::vector<InputDescriptor*> &inputs,
			bool first, bool visible, std::string indent);
	void findInputs (AbstractWebApplication* app, std::vector<InputDescriptor*> &inputs);
	void findInputs (std::vector<InputData> *data, std::vector<InputDescriptor*> &inputs);
	AbstractWebElement* createModalDialog (AbstractWebElement *input);


	std::string stylePrefix;

	int currentModalInput;

public:
	unsigned int numPasswords;
	std::vector<InputDescriptor*> inputs;
	std::vector<AbstractWebElement*> submits;
	void parse (AbstractWebApplication *app, AbstractWebElement *formRoot,std::vector<InputData> *data );
	void reparse (std::vector<InputData> *data);
};

#endif /* SMARTFORM_H_ */
