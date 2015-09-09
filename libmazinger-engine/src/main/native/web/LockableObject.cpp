/*
 * LocableObject.cpp
 *
 *  Created on: 31/08/2015
 *      Author: gbuades
 */



#include <LockableObject.h>

LockableObject::LockableObject ()
{
	locks = 1;
}

void LockableObject::lock() {
	locks ++;
}

void LockableObject::release() {
	locks --;
	if ( locks == 0)
		delete this;
}

LockableObject::~LockableObject() {
}
