/*
 * KojiDialog.h
 *
 *  Created on: 22/06/2011
 *      Author: u07286
 */

#ifndef KOJIDIALOG_H_
#define KOJIDIALOG_H_


class KojiDialog: public SeyconDialog {
public:
	virtual bool askCard (const char* targeta, const char* cella, std::wstring &result);
	virtual bool askNewPassword (const char* reason, std::wstring &password);
	virtual DuplicateSessionAction askDuplicateSession (const char *details);
	virtual bool askAllowRemoteLogout ();
	virtual void notify (const char *message);
	virtual void progressMessage (const char *message) ;
};

#endif /* KOJIDIALOG_H_ */
