/*
 * SessionDialogHandler.h
 *
 *  Created on: 27/01/2012
 *      Author: u07286
 */

#ifndef SESSIONDIALOGHANDLER_H_
#define SESSIONDIALOGHANDLER_H_

#include <string>
#include <gtk/gtk.h>

class DialogHandler {
public:
	std::string processMessage (const std::string &input);
	DialogHandler();
	virtual ~DialogHandler();
	void run ();
	static void* main (void *);

private:
	int ticks;
	GtkWidget *timeoutLabel;
	GtkWidget *timeoutDialog;

	bool processTimeout ();
	void processLogout ();
	void processNotify (const char* message);
	static int timeoutFunction (void *data);
};

#endif /* SESSIONDIALOGHANDLER_H_ */
