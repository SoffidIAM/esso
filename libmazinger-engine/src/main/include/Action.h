/*
 * Action.h
 *
 *  Created on: 16/02/2010
 *      Author: u07286
 */

#ifndef ACTION_H_
#define ACTION_H_

class ComponentSpec;
class ComponentMatcher;
class WebMatcher;
class HllMatcher;

class Action {
public:
	Action();
	virtual ~Action();

	char *szType;
	char *szText;
	char *szContent;
	char *szEvent;
	bool m_canRepeat;
	bool m_executed;
	int  m_delay;
	long m_lastExecution;

	ComponentSpec *m_component;

	void executeAction (ComponentMatcher& matcher);
	bool executeAction (WebMatcher& matcher);
	void executeAction (HllMatcher& matcher);
	void executeAction ();

private:
	bool canExecute ();

};

#endif /* ACTION_H_ */
