
/*
 * JavaNativeComponent
 *
 *  Created on: 15/02/2010
 *      Author: u07286
 */

#ifndef WINDOWSNATIVECOMPONENT_H_
#define WINDOWSNATIVECOMPONENT_H_

#include <NativeComponent.h>

class WindowsNativeComponent:public NativeComponent {
public:
	WindowsNativeComponent (HWND hWnd);

	virtual const char* getClass();
	virtual NativeComponent* getParent ();
	virtual void getChildren (std::vector<NativeComponent*> &children);
	virtual void getAttribute (const char* name, std::string &value);
	virtual void setAttribute (const char* attribute, const char* value) ;
	virtual int equals (NativeComponent &component);
	virtual NativeComponent *clone();
	virtual void setFocus();
	virtual void click();

private:
	HWND m_hwnd;
};

#endif
