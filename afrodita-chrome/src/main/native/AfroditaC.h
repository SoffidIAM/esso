/*
 * MazingerFF.h
 *
 *  Created on: 22/11/2010
 *      Author: u07286
 */

#ifndef MAZINGERFF_H_
#define MAZINGERFF_H_

#ifdef WIN32
#include <windows.h>
#define DEBUG(x)
//#define DEBUG(x) MZNSendDebugMessage("%s",x);
#else
#define DEBUG(x)
//#define DEBUG(x) fprintf(stderr, "%s\n", x)
#endif

#include <string>
#include <vector>

#ifndef WIN32
#ifndef USE_QT

#include <gtk/gtk.h>

extern GtkWidget *signalWindow;
gboolean menuPopupHandler(gpointer userdata) ;

#endif
#endif
#endif /* MAZINGERFF_H_ */
