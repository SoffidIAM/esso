#include <gtk/gtk.h>
#include <X11/Xlib.h>


void getMousePointer (int *x, int *y)
{
	*x = 0;
	*y = 0;
	Display *dsp = XOpenDisplay( NULL );
	if( !dsp ){ return ; }

	int screenNumber = DefaultScreen(dsp);

	XEvent event;

	event.xbutton.x = event.xbutton.y = 0;

	/* get info about current pointer position */
	XQueryPointer(dsp, RootWindow(dsp, DefaultScreen(dsp)),
	&event.xbutton.root, &event.xbutton.window,
	&event.xbutton.x_root, &event.xbutton.y_root,
	&event.xbutton.x, &event.xbutton.y,
	&event.xbutton.state);

	printf("Mouse Coordinates: %d %d\n", event.xbutton.x, event.xbutton.y);

	*x = event.xbutton.x;
	*y = event.xbutton.y;

	XCloseDisplay( dsp );

}

gboolean onActivate (GtkWidget *dialog, gpointer userdata) {
	printf("On activate\n");
	gtk_dialog_response(GTK_DIALOG(dialog), 0);
	return true;
}

gboolean onClose (GtkWidget *widget, gpointer userdata) {
	static GtkWidget *dialog = gtk_widget_get_toplevel(widget);
	gint* ud = (gint*) userdata;
	gtk_dialog_response(GTK_DIALOG(dialog), *ud);
	return true;
}


gboolean menuPopupHandler (gpointer p) {
	
// Set up dialog...

/*
	GtkWidget *menu, *menuitem;
	menu = gtk_menu_new();

	GtkWidget* signalWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	menuitem = gtk_menu_item_new_with_label("Menu item");


	printf("Created menu\n");

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_set_sensitive(menuitem, false);

	menuitem = gtk_menu_item_new_with_label("Hola");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);


	menuitem = gtk_menu_item_new_with_label("Adios");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	printf("Show menu\n");
	// gtk_widget_show_all(menu);
	printf("Setting menu data \n");

	printf("Opening popup\n");
	gtk_menu_popup_at_widget(GTK_MENU(menu), GTK_WIDGET(signalWindow), GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_SOUTH_WEST, NULL);
	printf("Closed popup\n");

*/


	printf("a\n");


	int x;
	int y;

	getMousePointer(&x, &y);

	GtkWidget* signalWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
//	gtk_window_set_default_size(GTK_WINDOW(signalWindow), 1, 1);
//	gtk_window_move(GTK_WINDOW(signalWindow), x, y);
	gtk_window_set_position(GTK_WINDOW(signalWindow), GTK_WIN_POS_MOUSE);
//	gtk_widget_show(signalWindow);

	static GtkWidget *dialog = gtk_dialog_new();
	GdkWindow * w = gtk_widget_get_parent_window ( dialog );
	gdk_window_set_events ( w, GDK_FOCUS_CHANGE_MASK );

	gtk_window_set_transient_for ( GTK_WINDOW(dialog), GTK_WINDOW (signalWindow));
	GtkWidget * label = gtk_label_new("Prueba");
	GtkWidget * button = gtk_button_new_with_label("Prueba");
	GtkWidget * content = gtk_dialog_get_content_area(
			GTK_DIALOG(dialog));
	gtk_container_add (
			GTK_CONTAINER( content ), button);

	gtk_widget_show(GTK_WIDGET(button));

	GtkWidget * child = gtk_link_button_new_with_label("Opción A", "Opción A");
	gtk_widget_show(GTK_WIDGET(child));
	g_signal_connect(dialog, "focus-out-event",
				G_CALLBACK(onActivate), NULL);

	gtk_container_add (
			GTK_CONTAINER( content ), child);

	GtkWidget * child2 = gtk_link_button_new_with_label("Opción B", "Opción B");
	gtk_widget_show(GTK_WIDGET(child2));
	gtk_container_add (
			GTK_CONTAINER( content ), child2);

	gint i = 1;
	g_signal_connect(child2, "activate-link",
				G_CALLBACK(onClose), &i);


	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);



	g_signal_connect(dialog, "focus-out-event",
				G_CALLBACK(onActivate), NULL);


	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	printf ("Result = %d\n", result);

	return G_SOURCE_REMOVE;
}


int main (int argc, char **argv)
{
	gtk_init(&argc, &argv);

	menuPopupHandler (NULL);
}
