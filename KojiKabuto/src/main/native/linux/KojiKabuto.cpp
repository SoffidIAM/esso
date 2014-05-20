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
#include <string>
#include <MazingerHook.h>
#include <MZNcompat.h>
#include <ssoclient.h>
#include <pthread.h>
#include "SessionDialogHandler.h"
#include <dlfcn.h>

GtkStatusIcon *tray_icon;

GIcon *iconEnabled;
GIcon *iconDisabled;

static int statusStarted = false;

void updateIconStatus() {
	const char *szUser = MZNC_getUserName();
	gdk_threads_enter();
	if (MZNIsStarted(szUser)) {
		if (!statusStarted) {
			std::string version;

			SeyconCommon::readProperty("MazingerVersion", version);

			std::string msg = "Actived (version " + version + ")";

			gtk_status_icon_set_from_gicon(tray_icon, iconEnabled);
			gtk_status_icon_set_tooltip(tray_icon, msg.c_str());
			statusStarted = true;
		}
	} else {
		if (statusStarted) {
			std::string version;

			SeyconCommon::readProperty("MazingerVersion", version);

			std::string msg = "Inactive (version " + version + ")";

			gtk_status_icon_set_from_gicon(tray_icon, iconDisabled);
			gtk_status_icon_set_tooltip(tray_icon, msg.c_str());
			statusStarted = false;
		}
	}
	gdk_threads_leave();
}

void view_popup_menu_onStart(GtkWidget *menuitem, gpointer userdata) {
	MZNStart(MZNC_getUserName());
	updateIconStatus();
}

void view_popup_menu_onStop(GtkWidget *menuitem, gpointer userdata) {
	MZNStop(MZNC_getUserName());
	updateIconStatus();
}

void view_popup_menu_onReload(GtkWidget *menuitem, gpointer userdata) {
	gdk_threads_enter();
	SeyconSession session;
	session.setUser(MZNC_getUserName());
	session.updateMazingerConfig();

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Mazinger", NULL,
			GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	GtkWidget *label = gtk_label_new("Configuration updated");
	gtk_box_pack_start(GTK_BOX(content), label, true, true, 10);
	gtk_widget_show(label);
	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);

	gdk_threads_leave();
}

void tray_icon_on_menu(GtkStatusIcon *status_icon, guint button,
		guint32 activate_time, gpointer user_data) {
	gdk_threads_enter();
	GtkWidget *menu, *menuitem;
	bool started = MZNIsStarted(MZNC_getUserName());

	menu = gtk_menu_new();

	menuitem = gtk_menu_item_new_with_label("Start");

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if (started)
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

	gdk_threads_leave();
}

void* updateKojiStatus(void * lpv) {
	do {
		updateIconStatus();
		sleep(5);
	} while (true);
	return NULL;

}

static GtkStatusIcon *create_tray_icon() {
	gdk_threads_enter();
	tray_icon = gtk_status_icon_new();
	g_signal_connect(G_OBJECT(tray_icon), "activate",
			G_CALLBACK(tray_icon_on_menu), NULL);
	g_signal_connect(G_OBJECT(tray_icon), "popup-menu",
			G_CALLBACK(tray_icon_on_menu), NULL);

	gtk_status_icon_set_from_gicon(tray_icon, iconDisabled);
	gtk_status_icon_set_visible(tray_icon, TRUE);

	pthread_t thread1;
	pthread_create(&thread1, NULL, updateKojiStatus, (void*) tray_icon);

	gdk_threads_leave();

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
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	return 0;
}
