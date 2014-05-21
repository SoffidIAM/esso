/*
 * NotificationHandler.h
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#ifndef NOTIFICATIONHANDLER_H_
#define NOTIFICATIONHANDLER_H_

class NotificationHandler {
public:
	NotificationHandler();
	virtual void onProviderAdd () = 0;
	virtual void onCardInsert () = 0;
	virtual void onCardRemove () = 0;
	virtual ~NotificationHandler() = 0;
};

#endif /* NOTIFICATIONHANDLER_H_ */
