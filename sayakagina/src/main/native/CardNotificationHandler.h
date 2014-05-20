/*
 * LoggedOutManager.h
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#ifndef _CARDNOTIFICATIONHANDLER_H_
#define _CARDNOTIFICATIONHANDLER_H_

#define _WIN32_IE 0x0500
#include <string>
#include "Log.h"
#include "NotificationHandler.h"
#include "logindialog.h"

class CardNotificationHandler: public NotificationHandler {
public:
	CardNotificationHandler(HWND *hwnd);
	virtual void onProviderAdd ();
	virtual void onCardInsert ();
	virtual void onCardRemove ();
	virtual ~CardNotificationHandler();
	bool isCardInserted () {
		return cardInserted;
	}
	void reset () {
		cardInserted = false;
	}
private:
	Log m_log;
	HWND *hwnd;
	bool cardInserted;
};

#endif /* _CARDNOTIFICATIONHANDLER_H_ */
