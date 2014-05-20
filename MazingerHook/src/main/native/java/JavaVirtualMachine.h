/*
 * JavaVirtualMachine.h
 *
 *  Created on: 25/02/2010
 *      Author: u07286
 */

#ifndef JAVAVIRTUALMACHINE_H_
#define JAVAVIRTUALMACHINE_H_

#include <jni.h>

class JavaVirtualMachine {
public:
	JavaVirtualMachine();
	virtual ~JavaVirtualMachine();
	static JNIEnv *getCurrent();
	static jobject getHookObject();
	static void setCurrent(JNIEnv *env, jobject hook);
	static JNIEnv *createDaemonEnv(JavaVM13 *jvm);
	static JNIEnv *createDaemonEnv(JavaVM *jvm);
	static void destroyDaemonEnv();
	static unsigned int dwTlsIndex;
	static unsigned int dwTlsHookIndex;
	static void adjustPolicy ();
private:

	static void init ();
};

#endif /* JAVAVIRTUALMACHINE_H_ */
