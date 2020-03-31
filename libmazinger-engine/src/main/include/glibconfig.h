	//


#ifdef __i386__
	#ifdef __bionic
		#include <glibconfig_i386_bionic.h>
	#else
		#include <glibconfig_i386.h>
	#endif
#else
	#ifdef __bionic
		#include <glibconfig_x86_64_bionic.h>
	#else
		#include <glibconfig_x86_64.h>
	#endif
#endif
