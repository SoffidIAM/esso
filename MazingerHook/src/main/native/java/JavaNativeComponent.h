
/*
 * JavaNativeComponent
 *
 *  Created on: 15/02/2010
 *      Author: u07286
 */

#ifndef JAVANATIVECOMPONENT_H_
#define JAVANATIVECOMPONENT_H_

#include <NativeComponent.h>

class JavaNativeComponent:public NativeComponent {
public:
	JavaNativeComponent (jobject object);
	~JavaNativeComponent ();

	virtual const char* getClass();
	virtual NativeComponent* getParent ();
	virtual void  getChildren (std::vector<NativeComponent*> &children);
	virtual void getAttribute (const char* name, std::string &value);
	virtual void setAttribute (const char* attribute, const char* value) ;
	virtual int equals (NativeComponent &component);
	virtual NativeComponent *clone();
	virtual void setFocus();
	virtual void click();
	virtual void freeze ();
	virtual void unfreeze ();

private:
	jstring getObjectClass ();
	jobject object;
	void removeFromFridge ();
};

#endif
