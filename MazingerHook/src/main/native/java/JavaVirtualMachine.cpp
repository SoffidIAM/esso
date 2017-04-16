/*
 * JavaVirtualMachine.cpp
 *
 *  Created on: 25/02/2010
 *      Author: u07286
 */

#include "../MazingerHookImpl.h"
#include "JavaVirtualMachine.h"
#include <time.h>

#ifdef WIN32

unsigned int JavaVirtualMachine::dwTlsIndex = TLS_OUT_OF_INDEXES;
unsigned int JavaVirtualMachine::dwTlsHookIndex = TLS_OUT_OF_INDEXES;
#else
static __thread JNIEnv* currentEnv;
static __thread jobject currentObject;
#include <stdlib.h>
#include <string.h>
#endif

JavaVirtualMachine::JavaVirtualMachine() {
}

JavaVirtualMachine::~JavaVirtualMachine() {
}

JNIEnv* JavaVirtualMachine::getCurrent() {
	init ();
#ifdef WIN32
	JNIEnv* pEnv = (JNIEnv*) TlsGetValue(dwTlsIndex);
	return pEnv;
#else
	return currentEnv;
#endif

}

jobject JavaVirtualMachine::getHookObject() {
	init ();
#ifdef WIN32
	jobject hook = (jobject) TlsGetValue(dwTlsHookIndex);
	return hook;
#else
	return currentObject;
#endif

}

void JavaVirtualMachine::init () {
#ifdef WIN32
	if (dwTlsIndex == TLS_OUT_OF_INDEXES)
	{
	  dwTlsIndex = TlsAlloc();
	}
	if (dwTlsHookIndex == TLS_OUT_OF_INDEXES)
	{
		dwTlsHookIndex = TlsAlloc();
	}
#endif
}

void JavaVirtualMachine::setCurrent(JNIEnv *env, jobject hookObject) {
	init ();
#ifdef WIN32
	TlsSetValue(dwTlsIndex, env);
	TlsSetValue(dwTlsHookIndex, hookObject);
#else
	currentEnv = env;
	currentObject = hookObject;
#endif
}

JNIEnv* JavaVirtualMachine::createDaemonEnv(JavaVM* javavm) {
	JNIEnv *jenv;

	if (javavm->AttachCurrentThreadAsDaemon((void**) &jenv, NULL) == JNI_OK) {
		setCurrent(jenv, NULL);
	}
	return jenv;
}

JNIEnv* JavaVirtualMachine::createDaemonEnv(JavaVM13* javavm) {
	JNIEnv *jenv;

	if (javavm->AttachCurrentThread((void**) &jenv, NULL) == JNI_OK) {
		setCurrent(jenv, NULL);
	}
	return jenv;
}

void JavaVirtualMachine::destroyDaemonEnv() {
	JavaVM* vm;
	if (getCurrent() != NULL)
	{
		getCurrent()->GetJavaVM(&vm);
		vm->DetachCurrentThread();
	}
}


void ensureCapacity (char* & pszUserPolicy, int &policyLength, int &policyAllocated) {
	if (policyLength + 1024 > policyAllocated)
	{
		policyAllocated += 1024;
		pszUserPolicy = (char*) realloc (pszUserPolicy, policyAllocated+1);
	}
}


static void changeDeploymentFile (const char *deploymentFile)
{
	char *pszUserPolicy = (char*) malloc(1025);
	int policyLength = 0;
	int policyAllocated = 1024;

	FILE* f= fopen (deploymentFile, "r");
	if (f != NULL)
	{
		int read = 0;
		do {
			policyLength += read;
			ensureCapacity(pszUserPolicy, policyLength, policyAllocated);
			read = fread(&pszUserPolicy[policyLength], 1, 1024, f);
		} while (read > 0);
		fclose (f);
	}
	pszUserPolicy[policyLength] = '\0';

	const char *line = "deployment.security.use.user.home.java.policy";

	if ( strstr (pszUserPolicy, line) == NULL)
	{
		f = fopen (deploymentFile, "a");
		if ( f != NULL)
		{
			time_t t;
			time (&t);
			struct tm *tm = localtime (&t);
			fprintf (f, "\r\n// Added by Soffid ESSO on %04d-%02d-%d %02d:%02d:%02d\r\n",
					tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
			fprintf (f, "%s=true\r\n", line);
			fclose (f);
		}
	}

}
static void createFile (const char *szPolicyFile, const char *szMazingerUrl) {
	char achPolicy [1024];

#ifdef WIN32

	std::string sz2 ("/");
	while (*szMazingerUrl)
	{
		if (*szMazingerUrl == '\\')
			sz2 += '/';
		else
			sz2 += *szMazingerUrl;
		szMazingerUrl ++;
	}
	szMazingerUrl = sz2.c_str();

#endif

	sprintf (achPolicy, "grant codeBase \"file:%s/*\" {permission java.security.AllPermission;};",
			szMazingerUrl);

	char *pszUserPolicy = (char*) malloc(1025);
	int policyLength = 0;
	int policyAllocated = 1024;
	FILE *f = fopen (szPolicyFile, "rb");
	if (f != NULL) {
		int read = 0;
		do {
			policyLength += read;
			ensureCapacity(pszUserPolicy, policyLength, policyAllocated);
			if (pszUserPolicy == NULL)
				return;
			read = fread(&pszUserPolicy[policyLength], 1, 1024, f);
		} while (read > 0);
		fclose (f);
	}

	pszUserPolicy[policyLength] = '\0';
	if ( strstr (pszUserPolicy, achPolicy) == NULL)
	{
		ensureCapacity(pszUserPolicy, policyLength, policyAllocated);
		time_t t;
		time (&t);
		struct tm *tm = localtime (&t);
		sprintf (&pszUserPolicy[policyLength], "\r\n// Added by Soffid ESSO on %04d-%02d-%d %02d:%02d:%02d\r\n%s\r\n",
				tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
				achPolicy);
		policyLength += strlen (&pszUserPolicy[policyLength] );

		FILE *fout = fopen (szPolicyFile, "wb");
		if (fout == NULL)
		{
			MZNSendDebugMessage ("Cannot create %s file\n", szPolicyFile);
		}
		else
		{
			fwrite (pszUserPolicy, 1, policyLength, fout);
			fclose (fout);
		}
	}
	free (pszUserPolicy);
}

void JavaVirtualMachine::adjustPolicy () {
	char achPolicyFile[MAX_PATH];
	char achDeploymentFile[MAX_PATH];

	const char *szMznDir = getMazingerDir();
	char *szUrl = (char*) malloc (strlen(szMznDir)+1);

	int i;
	for (i = 0; szMznDir[i]; i++)
		if (szMznDir[i] == '\\')
			szUrl[i] = '/';
		else
			szUrl[i] = szMznDir[i];
	szUrl[i] = szMznDir[i];

#ifdef WIN32
	sprintf (achPolicyFile, "%s%s\\.java.policy", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
	createFile (achPolicyFile, szMznDir);

	sprintf (achPolicyFile, "%s\\.java.policy", getenv("USERPROFILE"));
	createFile (achPolicyFile, szMznDir);

	sprintf (achDeploymentFile, "%s%s\\AppData\\LocalLow\\Sun\\Java\\Deployment\\deployment.properties", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
	changeDeploymentFile (achDeploymentFile);

#else
	sprintf (achPolicyFile, "%s/.java.policy", getenv("HOME"));
	createFile (achPolicyFile, szMznDir);

	sprintf (achDeploymentFile, "%s/.java/deployment/deployment.properties", getenv("HOME"));
	changeDeploymentFile (achDeploymentFile);

#endif
}
