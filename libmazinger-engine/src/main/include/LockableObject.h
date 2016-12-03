/*
 * LocableObject.h
 *
 *  Created on: 31/08/2015
 *      Author: gbuades
 */

#ifndef LOCABLEOBJECT_H_
#define LOCABLEOBJECT_H_

#include <string>

class LockableObject {
private:
	int locks;
	std::string str;

public:
	LockableObject();
	virtual void lock ();
	virtual void release ();
	virtual std::string toString () = 0;
	virtual void sanityCheck ();
	bool debug;

protected:
	virtual ~LockableObject();
};


#endif /* LOCABLEOBJECT_H_ */
