/*
 * ComponentSpec.h
 *
 *  Created on: 16/02/2010
 *      Author: u07286
 */

#ifndef COMPONENTSPEC_H_
#define COMPONENTSPEC_H_

#include <pcreposix.h>
#include <vector>

class Action;
class ComponentMatcher;
class NativeComponent;

class ComponentSpec {
public:
	ComponentSpec();
	virtual ~ComponentSpec();
	char fullMatch;
	int dwDialogId;
	regex_t *reClass;
	regex_t *reName;
	regex_t *reTitle;
	regex_t *reText;
	regex_t *reDlgId;
	char *szId;
	char *szClass;
	char *szName;
	char *szTitle;
	char *szText;
	char *szDlgId;
	bool optional;
	int m_order;

	void setMatchedComponent (NativeComponent *component);
	NativeComponent* getMatchedComponent ();

	void dump();

	ComponentSpec* m_parent;
	std::vector<Action*> m_actions;
	std::vector<ComponentSpec*> m_children;
	NativeComponent *m_pMatchedComponent;
	bool m_repeat; // Se trata de una repeticion del match

	int match (NativeComponent &component, ComponentMatcher &matcher);

	int matchAttribute (regex_t *expr, NativeComponent &native, const char*attribute);
};

#endif /* COMPONENTSPEC_H_ */
