#include "../MazingerHookImpl.h"
#include <jni.h>
#include <pcreposix.h>
#include <stdio.h>
#include <ComponentMatcher.h>
#include <NativeComponent.h>
#include "JavaNativeComponent.h"
#include "JavaVirtualMachine.h"
#ifdef WIN32
#include <psapi.h>
#include <winver.h>
#include <string.h>
#else
#include <dlfcn.h>
#include <string.h>
#endif

void executeActions(JNIEnv *env, jobject focus);

int fail(const char* message) {
	MZNSendDebugMessage(message);
	return 0;
}


extern "C" {

JNICALL void Java_es_caib_seycon_sso_windows_Hook_notifyFocus(
		JNIEnv *env, jobject hook, jobject window, jobject focus) {
	if (focus != NULL) {
		JavaVirtualMachine::setCurrent(env, hook);
		//		sendJavaFocusDebugInformation(env, focus);
		// Detectar component
		ConfigReader *config = MazingerEnv::getDefaulEnv()->getConfigReader();
		ComponentMatcher m;
		JavaNativeComponent component(focus);
		m.search(*config, component);
		if (m.isFound()) {
			TRACE;
			m.triggerFocusEvent();
		}
		env->ExceptionClear();
		JavaVirtualMachine::setCurrent(NULL, NULL);
	}
}


JNICALL void Java_es_caib_seycon_sso_windows_Hook13_notifyFocus(
		JNIEnv *env, jobject hook, jobject focus) {
	if (focus != NULL) {
		JavaVirtualMachine::setCurrent(env, hook);
		//		sendJavaFocusDebugInformation(env, focus);
		// Detectar component
		ConfigReader *config = MazingerEnv::getDefaulEnv()->getConfigReader();
		ComponentMatcher m;
		JavaNativeComponent component(focus);
		m.search(*config, component);
		if (m.isFound()) {
			m.triggerFocusEvent();
		}
		env->ExceptionClear();
		JavaVirtualMachine::setCurrent(NULL, NULL);
	}
}

}

const std::wstring getJavaHookFile ()
{
	std::string szFile = getMazingerDir();
#ifdef WIN32
	szFile += "\\ProfYumi.jar";
#else
	szFile += "/ProfYumi.jar";
#endif
	std::wstring wszFile = MZNC_strtowstr(szFile.c_str());
	return wszFile;
}


bool isToolkitStarted (JNIEnv* jenv) {
	// file = new File (...)
	jclass toolkitC = jenv->FindClass("java/awt/Toolkit");
	if (toolkitC == NULL)
		return false;
	jfieldID fid = jenv->GetStaticFieldID(toolkitC, "toolkit", "Ljava/awt/Toolkit;");
	if (fid == NULL)
		return false;
	jobject obj = jenv->GetStaticObjectField(toolkitC, fid);
	if (obj == NULL)
		return false;

	return true;

}

jclass installJvmClass(JNIEnv* jenv, bool jkd13) {
	// file = new File (...)
	jclass fileC = jenv->FindClass("java/io/File");
	if (fileC == NULL) {
		fail("Cannot locate class java.io.File\n");
		return NULL;
	}
	jvalue args[10];
	jmethodID fileCons = jenv->GetMethodID(fileC, "<init>",
			"(Ljava/lang/String;)V");
	if (fileCons == NULL) {
		fail("Cannot locate Method <init> on File");
		return NULL;
	}
	std::wstring fileName = getJavaHookFile();
    MZNSendDebugMessageW(L"JVM: Loading %ls", fileName.c_str());
	// Determinar el nom de l'arxiu JAR
	//		getmodulefilename ()
    std::string utfFileName = MZNC_wstrtoutf8(fileName.c_str());
	jstring str = jenv->NewStringUTF(utfFileName.c_str());
	jobject file = jenv->NewObject(fileC, fileCons, str);
	if (file == NULL) {
		fail("Failed to create File object");
		return NULL;
	}
	//
	// url = file.toURL ();
	jmethodID toURLMethod = jenv->GetMethodID(fileC, "toURL",
			"()Ljava/net/URL;");
	if (toURLMethod == NULL) {
		fail("Cannot locate Method toURL on File");
		return NULL;
	}
	jobject url = jenv->CallObjectMethod(file, toURLMethod);
	if (url == NULL) {
		fail("Cannot instantiate URL object");
		return NULL;
	}
	//
	// urlArray = new URL[] {url}
	jclass urlC = jenv->FindClass("java/net/URL");
	if (urlC == NULL) {
		fail("Cannot locate class java.net.URL");
		return NULL;
	}
	jarray urlArray = jenv->NewObjectArray(1, urlC, url);
	// cl = new java.net.URLClassLoader ( urlArray );
	//
	jclass uclC = jenv->FindClass("java/net/URLClassLoader");
	if (uclC == NULL) {
		fail("Cannot create object java.net.URLClassLoader");
		return NULL;
	}
	jmethodID uclCons = jenv->GetMethodID(uclC, "<init>", "([Ljava/net/URL;)V");
	if (uclCons == NULL) {
		fail("Cannot get constructor for URLClassLoader");
		return NULL;
	}
	args[0].l = urlArray;
	jobject ucl = jenv->NewObjectA(uclC, uclCons, args);
	if (ucl == NULL) {
		fail("Cannot create URLCalssLoader");
		return NULL;
	}
	//
	// clazz = cl.loadClass ("es.caib.seycon.sso.windows.Hook");
	jmethodID loadClassMethod = jenv->GetMethodID(uclC, "loadClass",
			"(Ljava/lang/String;Z)Ljava/lang/Class;");
	if (loadClassMethod == NULL) {
		fail("Cannot get loadClassMethod on URLClassLoader");
		return NULL;
	}
	if (jkd13) {
		args[0].l = jenv->NewStringUTF("es.caib.seycon.sso.windows.Hook13");
		args[1].z = false;
		jclass clazz = (jclass) jenv->CallObjectMethodA(ucl, loadClassMethod, args);
		if (clazz == NULL) {
			fail("Cannot load es.caib.seycon.sso.windows.Hook13 class");
			return NULL;
		}
		jmethodID m = jenv->GetMethodID(clazz, "notifyFocus",
				"(Ljava/awt/Component;)V");
		if (m == NULL) {
			fail("Cannot find native method on Hook");
			return NULL;
		}
		JNINativeMethod nativeMethods[1];
		nativeMethods[0].fnPtr
				= (void*) Java_es_caib_seycon_sso_windows_Hook13_notifyFocus;
		nativeMethods[0].name = (char*) "notifyFocus";
		nativeMethods[0].signature = (char*) "(Ljava/awt/Component;)V";
		if (jenv->RegisterNatives(clazz, nativeMethods, 1) != 0) {
			fail("Cannot install native code on java Hook");
			return NULL;
		}
		return clazz;
	} else {
		args[0].l = jenv->NewStringUTF("es.caib.seycon.sso.windows.Hook");
		args[1].z = false;
		jclass clazz = (jclass) jenv->CallObjectMethodA(ucl, loadClassMethod, args);
		if (clazz == NULL) {
			fail("Cannot load es.caib.seycon.sso.windows.Hook class");
			return NULL;
		}
		jmethodID m = jenv->GetMethodID(clazz, "notifyFocus",
				"(Ljava/awt/Window;Ljava/awt/Component;)V");
		if (m == NULL) {
			fail("Cannot find native method on Hook");
			return NULL;
		}
		JNINativeMethod nativeMethods[1];
		nativeMethods[0].fnPtr
				= (void*) Java_es_caib_seycon_sso_windows_Hook_notifyFocus;
		nativeMethods[0].name = (char*) "notifyFocus";
		nativeMethods[0].signature = (char*) "(Ljava/awt/Window;Ljava/awt/Component;)V";
		if (jenv->RegisterNatives(clazz, nativeMethods, 1) != 0) {
			fail("Cannot install native code on java Hook");
			return NULL;
		}
		return clazz;
	}
}

int installJvm(JavaVM13 *j) {
	JNIEnv *jenv;

	jenv = JavaVirtualMachine::createDaemonEnv(j);
	if (jenv != NULL) {
		jclass clazz = installJvmClass(jenv, true);
		// es.caib.seycon.sso.windows.Hook.main ();
		if (clazz != NULL) {
			jmethodID hookM = jenv->GetStaticMethodID(clazz, "main", "()V");
			if (hookM == NULL)
				return fail("Cannot get Hook main method");
			jenv->CallStaticObjectMethod(clazz, hookM);
		}
		jthrowable thrown = jenv->ExceptionOccurred();
		if (thrown != NULL)
		{
			MZNSendDebugMessageA("JVM Exception produced");

			JavaNativeComponent c (thrown);

			std::string s;
			c.getAttribute("class", s);
			std::string msg;
			c.getAttribute("Message", msg);
			MZNSendDebugMessageA("JVM Exception produced: %s",
					s.c_str());
			MZNSendDebugMessageA("JVM Exception message: %s",
					msg.c_str());
		}
		jenv->ExceptionClear();
		// Detach
//		JavaVirtualMachine::destroyDaemonEnv();
	}
	MZNSendDebugMessage("JVM: Configured");
	return 0;
}

int installJvm(JavaVM *j, JNIEnv *jenv) {
	int ok = false;
	if (jenv != NULL) {
		if ( isToolkitStarted(jenv) ) {
			jclass clazz = installJvmClass(jenv, true);
			// es.caib.seycon.sso.windows.Hook.main ();
			if (clazz != NULL) {
				jmethodID hookM = jenv->GetStaticMethodID(clazz, "main", "()V");
				if (hookM == NULL)
					return fail("Cannot get Hook main method");
				else {
					jenv->CallStaticObjectMethod(clazz, hookM);
					ok = 1;
				}
			}
		}
		jenv->ExceptionClear();
	}
	MZNSendDebugMessage("JVM: Configured");
	return ok;
}

int installJvm(JavaVM *j) {
	bool ok = 1;
	JNIEnv *jenv;

	jenv = JavaVirtualMachine::createDaemonEnv(j);
	installJvm(j, jenv);
	// Detach
	JavaVirtualMachine::destroyDaemonEnv();
	return ok;
}


int uninstallJvm(void* p) {

	JavaVM* j = (JavaVM*) p;
	JNIEnv *jenv;

	MZNSendDebugMessage("About to configure JVM");
	jenv = JavaVirtualMachine::createDaemonEnv(j);
	if (jenv != NULL) {
		jclass clazz = installJvmClass(jenv, true);
		// es.caib.seycon.sso.windows.Hook.main ();
		if (clazz != NULL) {
			jmethodID hookM = jenv->GetStaticMethodID(clazz, "stop", "()V");
			if (hookM == NULL)
				return fail("JVM: Cannot get Hook stop method");
			jenv->CallStaticObjectMethod(clazz, hookM);
		}
		jenv->ExceptionClear();
		// Detach
		JavaVirtualMachine::destroyDaemonEnv();
	}
	MZNSendDebugMessage("JVM: Cannot configure hook");
	return 0;
}


#define MAXJVM 32
JavaVM *jvm[MAXJVM]; // Max 32 jvm per process
JavaVM13 *jvm13[MAXJVM];
jsize cJvm, cJvm13;

int installedJavaPlugin = 0;

#ifdef WIN32
jint (FAR WINAPI *Detected_GetCreatedJavaVMs)(JavaVM **vmBuf, jsize bufLen, jsize *nVMs) = NULL;

// Localiza si existe una DLL de Java
void installJavaPlugin(HWND hwndFocus) {
	if (installedJavaPlugin)
		return;

	// Verificar si hay SunAwtFrame
	HWND hwndActive = GetActiveWindow();
	if (hwndActive == NULL)
	{
		return;
	}
	char achClassName[512];
	int r = GetClassName(hwndActive, achClassName, 511);
	if (r == 0)
	{
		return;
	}
	bool ok = false;
	// Si el frame es java => OK
	// Si no, verificar si el foco es java
	if (strncmp (achClassName,"SunAwt", 6) == 0 ||
		strncmp (achClassName,"javax.swing.", 11) == 0 )
	{
		ok = true;
	} else
	{
		if (hwndFocus != NULL) {
			if (GetClassName(hwndFocus, achClassName, 511)) {
				if (strncmp (achClassName,"SunAwt", 6) == 0  ||
					strncmp (achClassName,"javax.swing.", 11) == 0 )
					ok = true;
			}
		}
	}

	if (!ok) {
		return;
	}
	MZNSendDebugMessage("JVM: Detected java window [%s]", achClassName);
	installedJavaPlugin = 1;

	HANDLE hProcess = GetCurrentProcess();
	HMODULE hModules[5128];
	DWORD cbNeeded;
	bool oldVersion = true;
	if (EnumProcessModules(hProcess, hModules, sizeof hModules, &cbNeeded)) {
		std::string moduleName;
		HMODULE h = NULL;
		std::vector<HMODULE> candidateModules;
		std::vector<std::string> candidateNames;
		for (unsigned int i = 0; i < cbNeeded / sizeof(HANDLE); i++) {
			char fileName[4096] = "<Unknown>";
			GetModuleFileNameEx(hProcess, hModules[i], fileName,
					sizeof fileName);
			unsigned int len = strlen(fileName);
			const char *match ="jvm.dll";
			const int matchlen = strlen(match);
			if (len >= matchlen && _stricmp(&fileName[len - matchlen], match) == 0) {
				if (false) {
					std::vector<std::string>::iterator namesIterator;
					std::vector<HMODULE>::iterator modulesIterator;
					for (namesIterator = candidateNames.begin(),
						 modulesIterator = candidateModules.begin() ;
						 namesIterator != candidateNames.end();
						 namesIterator ++, modulesIterator ++) {
						std::string n = *namesIterator;
						if (n.length() > len) {
							break;
						}

					}
					candidateNames.insert(namesIterator, fileName);
					candidateModules.insert(modulesIterator, hModules[i]);
				} else {
					candidateNames.push_back(fileName);
					candidateModules.push_back(hModules[i]);
				}
			}
		}


		std::vector<std::string>::iterator namesIterator;
		std::vector<HMODULE>::iterator modulesIterator;

		for (namesIterator = candidateNames.begin(), modulesIterator = candidateModules.begin() ;
			 Detected_GetCreatedJavaVMs == NULL &&namesIterator != candidateNames.end();
			 namesIterator ++, modulesIterator ++) {

			std::string moduleName = *namesIterator;
			HMODULE h = *modulesIterator;

				HRSRC hrsrc = FindResource (h, MAKEINTRESOURCE(1), RT_VERSION);
				if (hrsrc != NULL)
				{
					HGLOBAL hVersionInfo = LoadResource (h, hrsrc);
					if (hVersionInfo != NULL)
					{
						LPBYTE pVersionInfo = (LPBYTE) LockResource (hVersionInfo);
						VS_FIXEDFILEINFO *fi = (VS_FIXEDFILEINFO*)&pVersionInfo[40];
						WORD v1 =  HIWORD(fi->dwProductVersionMS);
						WORD v2 = LOWORD(fi->dwProductVersionMS);
						WORD v3 =  HIWORD(fi->dwProductVersionLS);
						WORD v4 = LOWORD(fi->dwProductVersionLS);
						MZNSendDebugMessageA("JVM: testing library %s version %d.%d.%d.%d",
								moduleName.c_str(),
								(int) v1,
								(int) v2,
								(int) v3,
								(int) v4);
						Detected_GetCreatedJavaVMs	= (jint (FAR WINAPI *)(JavaVM **vmBuf, jsize bufLen, jsize *nVMs))
								GetProcAddress (h, "JNI_GetCreatedJavaVMs");
						if (Detected_GetCreatedJavaVMs != NULL && (v1 > 1 || v2 > 4))  // Superior a JVM 1.4
						{
							oldVersion = false;
						}
						// UnlockResource(hVersionInfo);
					}
				} else {
					MZNSendDebugMessageA("JVM: testing identified library %s",
							moduleName.c_str());
					Detected_GetCreatedJavaVMs	= (jint (FAR WINAPI *)(JavaVM **vmBuf, jsize bufLen, jsize *nVMs))
							GetProcAddress (h, "JNI_GetCreatedJavaVMs");
				}
				// Obtenint la versió
		}
	}
	CloseHandle(hProcess);

	if (Detected_GetCreatedJavaVMs != NULL) {
		MZNSendDebugMessage("JVM: Success");
		if (oldVersion) {
			cJvm = 0;
			jint i = Detected_GetCreatedJavaVMs( (JavaVM**) jvm13, MAXJVM, &cJvm13);
			MZNSendDebugMessage("JVM: Detected %d instances of java 1.3", (int) cJvm13);
			if (i == JNI_OK) {
				for (int j = 0; j < cJvm13; j++) {
					installJvm(jvm13[j]);
				}
			}
		} else {
			cJvm13 = 0;
			jint i = Detected_GetCreatedJavaVMs(jvm, MAXJVM, &cJvm);
			MZNSendDebugMessage("JVM: Detected %d instances of java", (int) cJvm);
			if (i == JNI_OK) {
				for (int j = 0; j < cJvm; j++) {
					installJvm(jvm[j]);
				}
			}
		}
	} else {
		MZNSendDebugMessageA("JVM: Unable to load java");
	}
}

// Localiza si existe una DLL de Java
void uninstallJavaPlugin() {
	if (installedJavaPlugin && Detected_GetCreatedJavaVMs != NULL) {
		MZNSendDebugMessage("Uninstalling jvm");
		jint i = Detected_GetCreatedJavaVMs(jvm, MAXJVM, &cJvm);
		if (i == JNI_OK) {
			for (int j = 0; j < cJvm; j++) {
				uninstallJvm(jvm[j]);
			}
		}
	}
}

const char* getMazingerDir() {
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 0, KEY_READ, &hKey)
			== ERROR_SUCCESS) {
		static char achPath[4096] = "XXXX";
		DWORD dwType;
		DWORD size = -150 + sizeof achPath;
		size = sizeof achPath;
		RegQueryValueEx(hKey, "ProgramFilesDir", NULL, &dwType,
				(LPBYTE) achPath, &size);
		RegCloseKey(hKey);
		strcat(achPath, "\\SoffidESSO");
		return achPath;
	} else {
		return FALSE;
	}

}
#else
static jint (*Detected_GetCreatedJavaVMs)(JavaVM **vmBuf, jsize bufLen, jsize *nVMs) = NULL;

MAZINGERAPI int MZNPatchJvm(JavaVM *j, JNIEnv *jenv) {
	if (MZNIsStarted(MZNC_getUserName()))
	{
		if (jenv == NULL)
			return installJvm(j);
		else
			return installJvm(j, jenv);
	}
	else
		return 0;
}

MAZINGERAPI int MZNPatchJvm2(void *handle) {
	if (! MZNIsStarted(MZNC_getUserName()))
		return 0;

	if (installedJavaPlugin)
		return 0;

	Detected_GetCreatedJavaVMs	= (jint (*)(JavaVM **vmBuf, jsize bufLen, jsize *nVMs)) dlsym (handle, "JNI_GetCreatedJavaVMs");

	if (Detected_GetCreatedJavaVMs != NULL) {
		MZNSendDebugMessage("Probing jvm");
		if ( false ) {
			cJvm = 0;
			jint i = Detected_GetCreatedJavaVMs( (JavaVM**) jvm13, MAXJVM, &cJvm13);
			MZNSendDebugMessage("Probed %d instances of jvm 1.3", (int) cJvm13);
			if (i == JNI_OK) {
				for (int j = 0; j < cJvm13; j++) {
					installJvm(jvm13[j]);
				}
			}
		} else {
			cJvm13 = 0;
			jint i = Detected_GetCreatedJavaVMs(jvm, MAXJVM, &cJvm);
			MZNSendDebugMessage("Probed %d instances of jvm", (int) cJvm);
			if (i == JNI_OK) {
				for (int j = 0; j < cJvm; j++) {
					installJvm(jvm[j]);
					installedJavaPlugin = 1;
				}
			}
		}
	}
	return installedJavaPlugin;
}


const char* getMazingerDir() {
	return "/usr/share/mazinger";
}

/* MAZINGERAPI void __attribute__ ((constructor))  MYlibrary_init () {
}*/

#endif
