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

#define MEMORY_TEST

LockableObject::LockableObject ()
{
	locks = 1;
#ifdef MEMORY_TEST
	debug = true;
	MZNSendDebugMessage("Allocating object (%p). Locks = %d", this, locks);
#else
	debug = false;
#endif
}

void LockableObject::lock() {
#ifdef MEMORY_TEST
	if (locks == 0)
	{
		MZNSendDebugMessage("Locking already release object (%p).", this);
		exit (1);
	}
#endif
	locks ++;
	str = toString();
#ifdef MEMORY_TEST
	if (debug)
		MZNSendDebugMessage("Locked object %s (%p). Locks = %d", toString().c_str(), this, locks);
#endif
}

void LockableObject::release() {
	locks --;
	str = toString();
#ifdef MEMORY_TEST
	if (debug)
		MZNSendDebugMessage("Release object %s (%p). Locks = %d", toString().c_str(), this, locks);
	if (locks < 0) {
		MZNSendDebugMessage("Releasing already released object %s", toString().c_str());
		exit(1);
	}
#endif
	if ( locks == 0)
		delete this;

}

LockableObject::~LockableObject() {
	if (locks != 0)
	{
#ifdef MEMORY_TEST
		MZNSendDebugMessage("WARNING: Deleting not released object (%lx) (%ld locks) %s", (long) this, locks, str.c_str());
		exit(1);
#endif
	}
	else
	{
#ifdef MEMORY_TEST
		MZNSendDebugMessage("Deleting released object (%lx) (%ld locks) %s", (long) this, locks, str.c_str());
#endif
	}
}

void LockableObject::sanityCheck () {
	if (locks <= 0) {
		MZNSendDebugMessage("WARNING: Using already released object: %s (%p)",  toString().c_str(), this);
		if (debug)
			exit(1);
	} else
		str = toString();
}
