/*
 * LocableObject.cpp
 *
 *  Created on: 31/08/2015
 *      Author: gbuades
 */


#define MEMORY_TEST

#include <LockableObject.h>
#include <MazingerInternal.h>
#include <stdlib.h>

LockableObject::LockableObject ()
{
	locks = 1;
}

void LockableObject::lock() {
	locks ++;
}

void LockableObject::release() {
	locks --;
#ifdef MEMORY_TEST
	if (locks < 0) {
		MZNSendDebugMessage("Releasing already released object %s", toString().c_str());
		exit(1);
	}
#else
	if ( locks == 0)
		delete this;
#endif

}

LockableObject::~LockableObject() {
}

void LockableObject::sanityCheck () {
	if (locks <= 0) {
		MZNSendDebugMessage("Using already released object: %s",  toString().c_str());
		exit(1);
	}
}
