/*
 * KOjiKabuto.cpp
 *
 *  Created on: 14/06/2011
 *      Author: u07286
 */

#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <MazingerHook.h>
#include <MZNcompat.h>
#include <ssoclient.h>
#include <pthread.h>
#include "SessionDialogHandler.h"
#include <dlfcn.h>


#define TRACE printf("[%ld]: At %s:%d\n", pthread_self(), __FILE__, __LINE__)
//#define TRACE

GtkStatusIcon *tray_icon;

GIcon *iconEnabled;
GIcon *iconDisabled;

static int statusStarted = false;

void updateIconStatus() {
	const char *szUser = MZNC_getUserName();

	if (MZNIsStarted(szUser)) {
		if (!statusStarted) {
			std::string version;

			SeyconCommon::readProperty("MazingerVersion", version);

			std::string msg = "Actived (version " + version + ")";

			gtk_status_icon_set_from_gicon(tray_icon, iconEnabled);
			gtk_status_icon_set_tooltip_text(tray_icon, msg.c_str());
			statusStarted = true;
		}
	} else {
		if (statusStarted) {
			std::string version;

			SeyconCommon::readProperty("MazingerVersion", version);

			std::string msg = "Inactive (version " + version + ")";

			gtk_status_icon_set_from_gicon(tray_icon, iconDisabled);
			gtk_status_icon_set_tooltip_text(tray_icon, msg.c_str());
			statusStarted = false;
		}
	}
}

static bool paused = false;
static bool started = false;
static SeyconSession session;


static bool tryLogin (const std::string &user, const std::string &pass)
{

//	session.setDialog(new DialogHandler());
	TRACE;

	GdkCursor *c = gdk_cursor_new(GDK_WATCH);
	GtkBuilder *builder2 = gtk_builder_new();
	GError *error = NULL;
	gtk_builder_add_from_resource(builder2, "/login/progress.glade", &error);
	GObject *pw = gtk_builder_get_object(builder2, "progressWindow");

	gtk_widget_show_now(GTK_WIDGET(pw));
	gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(pw)), c);

	while ( gtk_events_pending()) {
		gtk_main_iteration();
	}

	int result = session.passwordSessionStartup(user.c_str(), MZNC_strtowstr(pass.c_str()).c_str());

	g_object_unref(c);
	gtk_widget_destroy(GTK_WIDGET(pw));
	g_object_unref(builder2);

	printf ("Result = %d", result);

	if (result == LOGIN_SUCCESS)
	{
		return true;
	} else if (result == LOGIN_ERROR)
	{
		TRACE;
		GtkWidget *dlg = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				"%s",
				session.getErrorMessage());
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(GTK_WIDGET(dlg));
		TRACE;
		return false;
	}
	else if (result == LOGIN_DENIED)
	{
		TRACE;
		GtkWidget *dlg = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_WARNING,
				GTK_BUTTONS_OK,
				"%s",
				session.getErrorMessage());
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(GTK_WIDGET(dlg));
		TRACE;
		return false;
	}
	else // LOGIN_UNKNOWNUSER
	{
		TRACE;
		GtkWidget *dlg = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_WARNING,
				GTK_BUTTONS_OK,
				"Unknown user");
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(GTK_WIDGET(dlg));
		TRACE;
		return false;
	}
}

gboolean window_login_doLogin(GtkWidget *button, gpointer userdata) {
	GObject *window = gtk_builder_get_object(GTK_BUILDER(userdata), "loginDialog");
	gtk_dialog_response(GTK_DIALOG(window), 1);
	return true;
}

gboolean window_login_doClose(GtkWidget *button, gpointer userdata) {
	GObject *window = gtk_builder_get_object(GTK_BUILDER(userdata), "loginDialog");
	gtk_dialog_response(GTK_DIALOG(window), 0);
	return true;
}

gboolean view_popup_menu_onLogin(GtkWidget *menuitem, gpointer userdata) {
	GtkBuilder *builder = gtk_builder_new();
	GError *error = NULL;
    gtk_builder_add_from_resource(builder, "/login/login.glade", &error);

	TRACE;

	TRACE;
	GObject *window = gtk_builder_get_object(builder, "loginDialog");
	TRACE;


	GObject *username = gtk_builder_get_object(builder, "username");
	GObject *password = gtk_builder_get_object(builder, "password");

	std::string user = MZNC_getUserName();
	gtk_entry_set_text(GTK_ENTRY(username), user.c_str());
	g_signal_connect(gtk_builder_get_object(builder, "buttonConnect"), "clicked",
			G_CALLBACK(window_login_doLogin), builder);
	g_signal_connect(gtk_builder_get_object(builder, "buttonClose"), "clicked",
			G_CALLBACK(window_login_doClose), builder);

	TRACE;
	do
	{
		gint result = gtk_dialog_run(GTK_DIALOG(window));
		if (result)
		{
			user = gtk_entry_get_text(GTK_ENTRY(username));
			std::string pass = gtk_entry_get_text(GTK_ENTRY(password));
			if (tryLogin(user, pass))
				break;
		}
		else
			break;
	} while (true);

	gtk_widget_destroy(GTK_WIDGET(window));

	return true;
}

gboolean view_popup_menu_onLogout(GtkWidget *menuitem, gpointer userdata) {
	if (started)
		session.close();
	MZNStop(MZNC_getUserName());
}

void view_popup_menu_onStart(GtkWidget *menuitem, gpointer userdata) {
	paused = false;
	MZNStart(MZNC_getUserName());
	updateIconStatus();
}

void view_popup_menu_onStop(GtkWidget *menuitem, gpointer userdata) {
	paused = true;
	MZNStop(MZNC_getUserName());
	updateIconStatus();
}

void view_popup_menu_onReload(GtkWidget *menuitem, gpointer userdata) {
	if ( ! session.isOpen())
	{
		session.setUser(MZNC_getUserName());
		session.setSoffidUser(MZNC_getUserName());
	}
	session.updateMazingerConfig();

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Mazinger", NULL,
			GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	GtkWidget *label = gtk_label_new("Configuration updated");
	gtk_box_pack_start(GTK_BOX(content), label, true, true, 10);
	gtk_widget_show(label);
	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);

}

gboolean tray_icon_on_menu(GtkStatusIcon *status_icon, guint button,
		guint32 activate_time, gpointer user_data) {


	printf ("Started tray_icon_on_menu\n");
	fflush(stdout);

	GtkWidget *menu, *menuitem;
	bool started = MZNIsStarted(MZNC_getUserName());

	menu = gtk_menu_new();



	menuitem = gtk_menu_item_new_with_label("Login");

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if (started)
		gtk_widget_set_sensitive(menuitem, false);
	else
		g_signal_connect(menuitem, "activate",
				(GCallback) view_popup_menu_onLogin, NULL);


	menuitem = gtk_menu_item_new_with_label("Logout");

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if (! started || paused)
		gtk_widget_set_sensitive(menuitem, false);
	else
		g_signal_connect(menuitem, "activate",
				(GCallback) view_popup_menu_onLogout, NULL);

	menuitem = gtk_menu_item_new_with_label("Enable");

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if (started || ! paused)
		gtk_widget_set_sensitive(menuitem, false);
	else
		g_signal_connect(menuitem, "activate",
				(GCallback) view_popup_menu_onStart, NULL);

	menuitem = gtk_menu_item_new_with_label("Stop");

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if (started)
		g_signal_connect(menuitem, "activate",
				(GCallback) view_popup_menu_onStop, NULL);
	else
		gtk_widget_set_sensitive(menuitem, false);

	menuitem = gtk_menu_item_new_with_label("Update");

	g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onReload,
			NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	gtk_widget_show_all(menu);

	/* Note: event can be NULL here when called from view_onPopupMenu;
	 *  gdk_event_get_time() accepts a NULL argument */
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, activate_time);

	return TRUE;
}

gboolean updateKojiStatus(void * lpv) {
	gdk_threads_enter();

	TRACE;

	updateIconStatus();

	gdk_threads_leave();

	TRACE;

	return true;

}

static GtkStatusIcon *create_tray_icon() {
	tray_icon = gtk_status_icon_new();
	g_signal_connect(G_OBJECT(tray_icon), "activate",
			G_CALLBACK(tray_icon_on_menu), NULL);
	g_signal_connect(G_OBJECT(tray_icon), "popup-menu",
			G_CALLBACK(tray_icon_on_menu), NULL);

	gtk_status_icon_set_from_gicon(tray_icon, iconDisabled);
	gtk_status_icon_set_visible(tray_icon, TRUE);

	updateIconStatus();

	g_timeout_add_seconds(5, updateKojiStatus , (void*) tray_icon);

	return tray_icon;
}

extern "C" int main(int argc, char **argv) {
	static void   (*pg_thread_init)   (GThreadFunctions *vtable);

	pg_thread_init = (void   (*)   (GThreadFunctions *vtable)) dlsym (RTLD_DEFAULT, "g_thread_init");
	if (pg_thread_init != NULL)
		pg_thread_init (NULL);

	gdk_threads_init();
	gtk_init(&argc, &argv);

	GFile* f = g_file_new_for_path("/usr/share/mazinger/koji_kabuto.png");
	iconEnabled = g_file_icon_new(f);
	g_object_unref(G_OBJECT(f));

	f = g_file_new_for_path("/usr/share/mazinger/koji_gray.png");
	iconDisabled = g_file_icon_new(f);
	g_object_unref(G_OBJECT(f));

	create_tray_icon();


	g_thread_create(DialogHandler::main, NULL, false, NULL);


	gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_main();

	return 0;
}
