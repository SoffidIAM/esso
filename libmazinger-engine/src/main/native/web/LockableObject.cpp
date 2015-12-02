/*
 * LocableObject.cpp
 *
 *  Created on: 31/08/2015
 *      Author: gbuades
 */


// #define MEMORY_TEST

#include <LockableObject.h>
#include <MazingerInternal.h>
#include <stdlib.h>

LockableObject::LockableObject ()
{
	locks = 1;
	debug = false;
}

void LockableObject::lock() {
	locks ++;
#ifdef MEMORY_TEST
	if (debug)
		MZNSendDebugMessage("Locked object %s (%p). Locks = %d", toString().c_str(), this, locks);
#endif
}

void LockableObject::release() {
	locks --;
#ifdef MEMORY_TEST
	if (debug)
		MZNSendDebugMessage("Release object %s (%p). Locks = %d", toString().c_str(), this, locks);
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
		MZNSendDebugMessage("Using already released object: %s (%p)",  toString().c_str(), this);
		exit(1);
	}
}
