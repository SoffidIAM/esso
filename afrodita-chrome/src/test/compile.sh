glib-compile-resources --target=dialog.c --generate-source --sourcedir=. resources.xml
g++ test.cpp dialog.c -o test -I /usr/include/gtk-3.0 -I /usr/include/glib-2.0/ -I /usr/lib/x86_64-linux-gnu/glib-2.0/include -I /usr/include/pango-1.0 -I /usr/include/cairo -I /usr/include/gdk-pixbuf-2.0 -I/usr/include/atk-1.0 \
 						-lX11 -lrt -lz -lffi -lselinux -lresolv -lgobject-2.0 -lgtk-3 -lgdk-3 -lglib-2.0 -lgmodule-2.0 -lgio-2.0


