/*
 * LocableObject.h
 *
 *  Created on: 31/08/2015
 *      Author: gbuades
 */

#ifndef LOCABLEOBJECT_H_
#define LOCABLEOBJECT_H_

class LockableObject {
private:
	int locks;

public:
	LockableObject();
	virtual void lock ();
	virtual void release ();

protected:
	virtual ~LockableObject();
};


#endif /* LOCABLEOBJECT_H_ */
