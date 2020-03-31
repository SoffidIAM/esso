/*
 * ComponentMatcher.h
 *
 *  Created on: 15/02/2010
 *      Author: u07286
 */

#ifndef COMPONENTMATCHER_H_
#define COMPONENTMATCHER_H_

#include <ConfigReader.h>
#include <NativeComponent.h>
#include <vector>

class FailureReason;
class ComponentSpec;


class ComponentMatcher {
public:
	ComponentMatcher();
	virtual ~ComponentMatcher();
	int search (ConfigReader &reader, NativeComponent& focus);
	void clearFailures();
	void registerFailure(FailureReason *reason);

	ComponentSpec* getFocusComponent ();
	int isFound();
	void notifyMatch (ComponentSpec &spec, NativeComponent &native);
	void triggerBlurEvent () ;
	void triggerFocusEvent () ;
	std::vector<ComponentSpec*> &getAliasedComponents();
	ComponentMatcher *clone();
	void dumpDiagnostic (NativeComponent *top, NativeComponent *focus);

private:

	bool m_bMatched;
	ComponentSpec* m_component;
	ComponentSpec* m_focus;
	NativeComponent *m_nativeFocus;
	std::vector<ComponentSpec*> m_aliasedComponents;
	std::vector<FailureReason*> m_failedComponents ;
};

#endif /* COMPONENTMATCHER_H_ */
