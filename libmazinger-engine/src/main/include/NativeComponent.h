/*
 * NativeComponent.h
 *
 *  Created on: 15/02/2010
 *      Author: u07286
 */

#ifndef NATIVECOMPONENT_H_
#define NATIVECOMPONENT_H_

#include <vector>
#include <string>



class NativeComponent {
public:
	virtual ~NativeComponent ();
	virtual const char* getClass() = 0;
	virtual void getChildren (std::vector<NativeComponent*>  &children) = 0;
	virtual NativeComponent* getParent () = 0;
	virtual void getAttribute (const char* name, std::string &value) = 0;
	virtual void setAttribute (const char* attribute, const char* value) = 0;
	virtual int equals (NativeComponent &component) = 0;
	virtual NativeComponent *clone() = 0;
	virtual void setFocus() = 0;
	virtual void click() = 0;
	virtual void freeze ();
	virtual void unfreeze ();
	void dump ();
	void dumpXML (int depth, NativeComponent *focus);
	void dumpXML (int depth) {dumpXML (depth, NULL);}

protected:
	NativeComponent ();
	void dumpAttribute  (const char *attribute) ;
};

#endif /* NATIVECOMPONENT_H_ */
