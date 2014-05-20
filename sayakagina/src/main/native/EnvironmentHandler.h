/*
 * EnvironmentHandler.h
 *
 *  Created on: 21/01/2011
 *      Author: u07286
 */

#ifndef ENVIRONMENTHANDLER_H_
#define ENVIRONMENTHANDLER_H_

class EnvironmentHandler {
public:
	EnvironmentHandler();
	EnvironmentHandler(wchar_t* pEnv) ;

	virtual ~EnvironmentHandler();
	wchar_t *generate (HANDLE hToken, MSV1_0_INTERACTIVE_PROFILE* pProfile);

private:
	wchar_t *m_pEnv;
	int m_len;
	void addVariable (const wchar_t *var, const wchar_t*value);

};

#endif /* ENVIRONMENTHANDLER_H_ */
