/*
 * FailureReason.h
 *
 *  Created on: 24/02/2010
 *      Author: u07286
 */

#ifndef FAILUREREASON_H_
#define FAILUREREASON_H_
class ComponentSpec;
class NativeComponent;

#include <pcreposix.h>

class FailureReason {
public:
	FailureReason(ComponentSpec *spec, NativeComponent *native) ;
	FailureReason(ComponentSpec *spec, int order) ;
	FailureReason(ComponentSpec *spec) ;
	FailureReason(NativeComponent *native) ;
	virtual ~FailureReason();
	void notify () ;
	ComponentSpec *m_pComponentSpec;
	NativeComponent *m_pComponent;
	int m_order;
private:
	void notifyAttribute (const char *attribute, regex_t* regExp, const char* regExpStr);
};

#endif /* FAILUREREASON_H_ */
