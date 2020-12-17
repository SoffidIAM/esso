#ifndef _SHIRO_SEYCONDIALOG_H
#define _SHIRO_SEYCONDIALOG_H

#include <ssoclient.h>

class ShiroSeyconDialog: public SeyconDialog {
public:
	virtual ~ShiroSeyconDialog ();
	virtual bool askCard (const char* targeta, const char* cella, std::wstring &result);
	virtual bool askNewPassword (const char* reason, std::wstring &password);
	virtual DuplicateSessionAction askDuplicateSession (const char *details);
	virtual bool askAllowRemoteLogout ();
	virtual void notify (const char *message);
	virtual void progressMessage (const char *message);

	std::wstring newPass;
	bool needsNewPassword = false;
	bool passwordUsed = false;
};


#endif
