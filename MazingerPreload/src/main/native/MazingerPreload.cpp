#include <dlfcn.h>
#include <string.h>
#include "jni.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

static jint JNICALL (*OriginalJNI_CreateJavaVM)(JavaVM **pvm, void **penv,
		void *args) = NULL;
static void * (*Original_dlsym)(void *handle, const char *symbol) = NULL;
static void * (*Original_dlopen)(const char *filename, int flag) = NULL;
static void *patchedHandle;
static pthread_mutex_t mutex;

typedef int (*MZNPatchJvmType)(void *handle);
static MZNPatchJvmType MZNPatchJvm = NULL;

static JavaVM* pendingjvm[10];
static int pendingjvmsize = 0;

#define MAXPENDLIBRARY 50
static void* pendinglibrary[MAXPENDLIBRARY];
static int pendinglibrarysize = 0;
static bool patched = false;


static bool debugEnabled;

static JavaVM** findEmptyJVM() {
	for (int i = 0; i < pendingjvmsize; i++) {
		if (pendingjvm[i] == NULL
		)
			return &pendingjvm[i];
	}
	if (pendingjvmsize < 10) {
		pendingjvmsize++;
		return &pendingjvm[pendingjvmsize - 1];
	} else
		return NULL;
}

static
void* patchPendingJVMThread(void *) {
	static void *mazingerhook_so = NULL;
	static MZNPatchJvmType MZNPatchJvm = NULL;

	if (debugEnabled)
		printf ("Looking jvm to patch\n");
	pthread_mutex_lock(&mutex);
	bool anypending = false;
	for (int i = 0; i < pendinglibrarysize && ! patched; i++) {
		if (pendinglibrary[i] != NULL) {
			if (debugEnabled)
				printf ("Looking library %lx to patch\n", (long) pendinglibrary[i]);
			if (mazingerhook_so == NULL)
				mazingerhook_so = dlopen("libMazingerHook.so", RTLD_NOW);

			if (mazingerhook_so != NULL && MZNPatchJvm == NULL)
				MZNPatchJvm = (MZNPatchJvmType) dlsym(mazingerhook_so, "MZNPatchJvm2");

			if (MZNPatchJvm != NULL) {
				if (debugEnabled)
					fprintf(stderr, "patching %lx\n", (long) pendinglibrary[i]);
				patched = MZNPatchJvm(pendinglibrary[i]);
			} else
				anypending = true;
		}
	}
	fflush(stdout);
	pthread_mutex_unlock(&mutex);
	return NULL;
}

static
void patchPendingJVM() {

	if (patched) return;

	pthread_t thread1;
	if (pendinglibrarysize > 0)
		pthread_create(&thread1, NULL, patchPendingJVMThread, NULL);
}
static jint doJNI_CreateJavaVM(JavaVM **pvm, void **penv, void *args) {
	if (debugEnabled)
		fprintf(stderr, "Calling JNI_CreateJavaVM (%lx)\n",
				(long) OriginalJNI_CreateJavaVM);
	if (OriginalJNI_CreateJavaVM != NULL) {
		jint result = OriginalJNI_CreateJavaVM(pvm, penv, args);
		if (result == 0) {
			if (debugEnabled)
				fprintf(stderr, "CREATE jvm SUCCESS\n");
			JavaVM **info = findEmptyJVM();
			if (info != NULL) {
				if (debugEnabled)
					fprintf(stderr, "Registering jvm %lx\n", (long) *pvm);
				*info = *pvm;
			}
		} else {
			if (debugEnabled)
				fprintf(stderr, "Create JVM failed\n");
		}
		return result;
	} else
		return -1;
}

typedef Window (*XCreateWindowType)(Display *display, Window parent, int x,
		int y, unsigned int width, unsigned int height,
		unsigned int border_width, int depth, unsigned int clazz,
		Visual *visual, unsigned long valuemask,
		XSetWindowAttributes *attributes);

extern "C" Window XCreateWindow(Display *display, Window parent, int x, int y,
		unsigned int width, unsigned int height, unsigned int border_width,
		int depth, unsigned int clazz, Visual *visual, unsigned long valuemask,
		XSetWindowAttributes *attributes) {
	if (debugEnabled)
		printf ("XCreateWindow hook\n");

	static XCreateWindowType real = NULL;
	if (real == NULL) {
		real = (XCreateWindowType) dlsym(RTLD_NEXT, "XCreateWindow");
	}
	if (real != NULL) {
		Window w = real(display, parent, x, y, width, height, border_width,
				depth, clazz, visual, valuemask, attributes);
		patchPendingJVM();
		return w;
	} else {
		if (debugEnabled)
			printf ("Canno get XCreateWindow\n");
		return None;
	}
}

typedef Window (*XCreateSimpleWindowType)(Display *display, Window parent,
		int x, int y, unsigned int width, unsigned int height,
		unsigned int border_width, unsigned long border,
		unsigned long background);

extern "C" Window XCreateSimpleWindow(Display *display, Window parent, int x,
		int y, unsigned int width, unsigned int height,
		unsigned int border_width, unsigned long border,
		unsigned long background) {

	if (debugEnabled)
		printf ("XCreateSimpleWindow hook\n");
	static XCreateSimpleWindowType real = NULL;
	if (real == NULL) {
		real = (XCreateSimpleWindowType) dlsym(RTLD_NEXT,
				"XCreateSimpleWindow");
	}
	if (real != NULL) {
		Window w = real(display, parent, x, y, width, height, border_width,
				border, background);
		patchPendingJVM();
		return w;
	} else {
		if (debugEnabled)
			printf ("Canno get XCreateSimpleWindow\n");
		return None;
	}
}

static void *dlsymXXX(void *handle, const char *symbol) {
	if (Original_dlsym == NULL) {
		Original_dlsym = (void* (*)(void*, const char*)) dlvsym(RTLD_NEXT,
				"dlsym", "GLIBC_2.0");
	}

	if (Original_dlsym == NULL) {
		Original_dlsym = (void* (*)(void*, const char*)) dlvsym(RTLD_NEXT,
				"dlsym", "GLIBC_2.2.5");
	}

	if (Original_dlsym == NULL) {
		if (debugEnabled)
			fprintf(stderr, "Cannot load dlsym\n");
		return NULL;
	}

	if (strcmp(symbol, "JNI_CreateJavaVM") == 0) {
		if (debugEnabled)
			fprintf(stderr,
					"Getting JNI_CreateJavaVM Symbol (handle=%lx) (pid=%d)\n",
					(long) handle, getpid());
		if (OriginalJNI_CreateJavaVM == NULL)
		{
			OriginalJNI_CreateJavaVM = (jint JNICALL (*)(JavaVM **, void **,
					void *)) Original_dlsym(
					handle, symbol);patchedHandle = handle;
			if (debugEnabled)
			fprintf (stderr, "Got JNI_CreateJavaVM Symbol %lx\n", (long)OriginalJNI_CreateJavaVM );
		}
		if (debugEnabled)
			fprintf(stderr, "End Getting JNI_CreateJavaVM Symbol\n");
		fflush(stdout);
		if (handle == patchedHandle) {
			if (OriginalJNI_CreateJavaVM == NULL
				)
				return NULL;
			else
				return (void*) doJNI_CreateJavaVM;
		} else
			return Original_dlsym(handle, symbol);

	} else {
		void *r = Original_dlsym(handle, symbol);
		return r;
	}
}


extern "C" void *dlopen(const char *filename, int flag) {
	if (debugEnabled)
		fprintf(stderr, "dlopen %s\n", filename);

	if (Original_dlopen == NULL) {
		Original_dlopen = (void* (*)(const char*, int)) dlsym(RTLD_NEXT, "dlopen");
	}

	void * result = Original_dlopen (filename, flag);
	if (result != NULL) {
		if (filename != NULL && strstr (filename, "libjvm.so") != NULL && pendinglibrarysize < MAXPENDLIBRARY) {
			if (debugEnabled)
				printf ("Library to look for: %s: %lx\n", filename, (long) result);
			pendinglibrary[pendinglibrarysize++] = result;
		}
	}
	return result;
}

extern "C" void __attribute__ ((constructor)) MYlibrary_init() {
	debugEnabled = getenv("MAZINGER_PRELOAD_DEBUG") != NULL;
	if (debugEnabled)
		fprintf(stderr, "MAZINGER_PRELOAD_DEBUG ENABLED\n");
	pthread_mutex_init(&mutex, NULL);
}

