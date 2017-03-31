/*
 * PamScriptDialog.h
 *
 *  Created on: 18/01/2012
 *      Author: u07286
 */

#ifndef PAMSCRIPTDIALOG_H_
#define PAMSCRIPTDIALOG_H_

#include "ScriptDialog.h"
#include "PamHandler.h"

class PamScriptDialog: public ScriptDialog {
	PamHandler* pamh;
public:
	PamScriptDialog(PamHandler *pamh);
	virtual ~PamScriptDialog();
	virtual void alert(const char *msg);
	virtual void progressMessage (const char *msg) ;
	virtual void cancelProgressMessage ();
	virtual std::wstring selectAccount(std::vector<std::wstring>accounts,
			std::vector<std::wstring>accountDescriptions);
	virtual std::wstring askPassword (std::wstring label) ;
	virtual std::wstring askText (std::wstring label) ;
};

#endif /* PAMSCRIPTDIALOG_H_ */
