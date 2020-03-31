//


#ifdef __i386__
	#ifdef __bionic
		#include <gdkconfig_i386_bionic.h>
	#else
		#include <gdkconfig_i386.h>
	#endif
#else
	#ifdef __bionic
		#include <gdkconfig_x86_64_bionic.h>
	#else
		#include <gdkconfig_x86_64.h>
	#endif
#endif
