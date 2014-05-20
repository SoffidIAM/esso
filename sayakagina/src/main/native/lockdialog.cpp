#include "sayaka.h"
#include "winwlx.h"

#include "NotificationHandler.h"
#include "TokenHandler.h"
#include "Pkcs11Handler.h"


//////////////////////////////////////////////////////////
static BOOL CALLBACK lockDialogProc(
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	return 0;
}

class LockedNotificationHandler: public NotificationHandler {
public:
	LockedNotificationHandler();
	virtual void onProviderAdd ();
	virtual void onCardInsert ();
	virtual void onCardRemove ();
	virtual ~LockedNotificationHandler();
private:
	Log m_log;
};

LockedNotificationHandler::LockedNotificationHandler():
	m_log ("LockedNotificationHandler") {
}
void LockedNotificationHandler::onProviderAdd () {
	m_log.info ("onProviderAdd");
}
void LockedNotificationHandler::onCardInsert () {
	pWinLogon->WlxSasNotify(hWlx, WLX_SAS_TYPE_CTRL_ALT_DEL);
	m_log.info ("onCardInsert");
}
void LockedNotificationHandler::onCardRemove () {
	m_log.info ("onCardRemove");
}

LockedNotificationHandler::~LockedNotificationHandler() {}


void displayLockMessage ()
{
	LockedNotificationHandler notificator;

	if (p11Config != NULL) {
		p11Config -> setNotificationHandler(&notificator);
	}

	pWinLogon->WlxDialogBox(hWlx, hSayakaDll, MAKEINTRESOURCEW (IDD_LOCKED), NULL, lockDialogProc);

	if (p11Config != NULL) {
		p11Config->setNotificationHandler(NULL);
	}
}

