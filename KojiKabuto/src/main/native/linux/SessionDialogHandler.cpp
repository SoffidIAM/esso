/*
 * SessionDialogHandler.cpp
 *
 *  Created on: 27/01/2012
 *      Author: u07286
 */

#include "SessionDialogHandler.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <MZNcompat.h>
#include <ssoclient.h>
#include <string>
#include <sys/stat.h>
#include <gtk/gtk.h>

DialogHandler::DialogHandler() {
}

DialogHandler::~DialogHandler() {
}

void DialogHandler::run()
{
	const char *sessId = getenv ("MZN_SESSION");
	SeyconCommon::debug ("DialogHandler running");
	if (sessId != NULL) {
		std::string socketName = "/tmp/sayaka-";
		socketName += sessId;
		mkdir (socketName.c_str(), 0700);
		socketName += "/";
		socketName += "dlg";

		SeyconCommon::debug ("Creating socket %s", socketName.c_str() );
		// Crear socket
		struct sockaddr_un remote;
		int s = socket(AF_UNIX, SOCK_STREAM, 0);
		if (s >= 0) {
			remote.sun_family = AF_UNIX;  /* local is declared before socket() ^ */
			strcpy(remote.sun_path, socketName.c_str());
			int len = strlen(remote.sun_path) + sizeof(remote.sun_family);


			if (connect (s, (struct sockaddr *)&remote, len) == 0) {
				do {
					std::string line;
					do {
						char r;
						int read = recv (s, &r, 1, 0);
						if (read <= 0) {
							close (s);
							return;
						}
						if (r == '\0') break;
						line += r;
					} while (true);
					SeyconCommon::debug ("Receiving %s",line.c_str());
					std::string result = processMessage (line);
					if (send (s, result.c_str(), result.length()+1, 0) <= 0) {
						close (s);
						return;
					}
					SeyconCommon::debug ("Sent %s", result.c_str());
				} while (true);
			}
		}
	}
}

void* DialogHandler::main(void *)
{
	DialogHandler dialog;
	dialog.run();
}



std::string DialogHandler::processMessage (const std::string &input) {
	std::string result;
	int i = input.find('\n');
	if ( i != std::string::npos) {
		std::string tag = input.substr(0, i);
		std::string tail = input.substr(i+1);
		if (tag == "LOGOUT") {
			processLogout ();
		} else if (tag == "NOTIFY") {
			processNotify (tail.c_str());
		}
	}
	return result;
}


void DialogHandler::processLogout()
{
	gdk_threads_enter();
	timeoutDialog = gtk_dialog_new_with_buttons("Avís", NULL, GTK_DIALOG_MODAL, NULL);
	GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(timeoutDialog));

	GtkWidget *label = gtk_label_new("Request to close actual session received.\n"
			"If you want close session press \"Close session\" button.\n"
			"Otherwise, press \"Cancel\" button");
	gtk_box_pack_start(GTK_BOX(content), label, true, true, 10);

	GtkWidget *label2 = gtk_label_new("If you do not indicate anything, the system will automatically close");
	gtk_box_pack_start(GTK_BOX(content), label2, true, true, 10);

	timeoutLabel = gtk_label_new("30");
	gtk_box_pack_end(GTK_BOX(content), timeoutLabel, true, true, 10);


	gtk_widget_show(label);
	gtk_widget_show(label2);
	gtk_widget_show(timeoutLabel);

	gtk_dialog_add_button(GTK_DIALOG(timeoutDialog), "Close session",
			GTK_RESPONSE_ACCEPT);
	gtk_dialog_add_button(GTK_DIALOG(timeoutDialog), "Cancel",
			GTK_RESPONSE_CANCEL);
	ticks = 30;
	guint tout = g_timeout_add( 1000, DialogHandler::timeoutFunction, this );

	gint result = gtk_dialog_run(GTK_DIALOG(timeoutDialog));

	g_source_remove(tout);

	gtk_widget_destroy(timeoutDialog);
	gdk_threads_leave();

	if (result == GTK_RESPONSE_ACCEPT) {
		if (fork () == 0) {
			execlp ("gnome-session-save", "gnome-session-save", "--force-logout", NULL);
			execlp ("gnome-session-quit", "gnome-session-quit", "--force", NULL);
			exit(0);
		}
	}
}

bool DialogHandler::processTimeout()
{
	bool goon = true;
	ticks -- ;
	SeyconCommon::debug ("processTimeout [%d]", ticks);
	if (ticks == 0) {
		gdk_threads_enter();
		gtk_label_set_label(GTK_LABEL(timeoutLabel), "-");
		gtk_dialog_response(GTK_DIALOG(timeoutDialog), GTK_RESPONSE_ACCEPT);
		gdk_threads_leave();
		return false;
	} else {
		gdk_threads_enter();
		char ach[10];
		sprintf (ach, "%d", ticks);
		gtk_label_set_label(GTK_LABEL(timeoutLabel), ach);
		gdk_threads_leave();
	}
	return goon;
}

static gpointer doNotify (gpointer data){
	char *message = (char*) data;
	gdk_threads_enter();
	SeyconCommon::debug ("NOTIFY : %s", message);
	SeyconCommon::debug ("%s: %d", __FILE__, __LINE__);
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Notice", NULL,
			GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	GtkWidget *label = gtk_label_new(message);
	gtk_box_pack_start(GTK_BOX(content), label, true, true, 10);
	gtk_widget_show(label);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	gdk_threads_leave();

	free (message);

	return NULL;
}

void DialogHandler::processNotify(const char *message) {
	g_thread_create(doNotify, strdup(message), false, NULL);
}

gboolean DialogHandler::timeoutFunction(void *data)
{
	DialogHandler *h = (DialogHandler*) data;
	SeyconCommon::debug ("timeoutFunction");
	return h->processTimeout();
}
