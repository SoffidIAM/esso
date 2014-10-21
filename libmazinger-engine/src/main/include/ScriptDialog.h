/*
 * ScriptDialog.h
 *
 *  Created on: 16/02/2010
 *      Author: u07286
 */

#ifndef SCRIPT_DIALOG_H_
#define SCRIPT_DIALOG_H_

#include <string>
#include <vector>

class ScriptDialog {
public:
	virtual ~ScriptDialog();
	ScriptDialog();
	virtual void alert(const char *msg) = 0;
	virtual void progressMessage (const char *msg) = 0;
	virtual void cancelProgressMessage () = 0;
	virtual std::wstring selectAccount(std::vector<std::wstring>accounts,std::vector<std::wstring>accountDescriptions) = 0;
	static void setScriptDialog (ScriptDialog *d);
	static ScriptDialog* getScriptDialog();
	static bool isScriptDialogSet();
};


#endif
