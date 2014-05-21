#include "../MazingerHookImpl.h"
#include <string>
#include <jni.h>
#include <MazingerInternal.h>
#include <MazingerHook.h>
#include <NativeComponent.h>
#include "JavaNativeComponent.h"
#include "JavaVirtualMachine.h"
#include <string.h>
#include <malloc.h>

JavaNativeComponent::JavaNativeComponent(jobject objValue) {
	this->object = objValue;
	freeze();
}

JavaNativeComponent::~JavaNativeComponent()
{
	removeFromFridge();
}

const char *JavaNativeComponent::getClass() {
	return "JavaNativeComponent";
}

int JavaNativeComponent::equals (NativeComponent &component) {
	JavaNativeComponent &c = (JavaNativeComponent&) component;
	c.unfreeze();
	if (c.object == NULL)
		return 0;
	if ( getClass() != component.getClass ())
		return 0;
	return JavaVirtualMachine::getCurrent()->IsSameObject(object, ((JavaNativeComponent&)component).object);
}

NativeComponent* JavaNativeComponent::clone() {
	JavaNativeComponent *result = new JavaNativeComponent(this->object);
	return result;
}

NativeComponent *JavaNativeComponent::getParent() {
	JNIEnv *pEnv = JavaVirtualMachine::getCurrent();
	jclass windowC = pEnv->FindClass("java/awt/Window");

	if (windowC != NULL && pEnv->IsInstanceOf(object, windowC)) {
		std::string className;
		getAttribute("class", className);
		return NULL;
	}

	jclass objectC = pEnv->GetObjectClass(object);
	if (objectC == NULL) {
		MZNSendDebugMessage("Object is null");
		return NULL;
	}
	jmethodID getParent = pEnv->GetMethodID(objectC, "getParent",
			"()Ljava/awt/Container;");
	if (getParent == NULL) {
		std::string s;
		getAttribute("class", s);
		pEnv->ExceptionClear();
		return NULL;
	}
	jobject parent = pEnv->CallObjectMethod(object, getParent, NULL);
	if (parent == NULL)
	{
		return NULL;
	}
	else
	{
		return new JavaNativeComponent (parent);
	}

}

void JavaNativeComponent::getAttribute(const char* attributeName, std::string &result) {
	JNIEnv *pEnv = JavaVirtualMachine::getCurrent();
	jstring value = NULL;
	result.clear ();
	if (strcmp(attributeName, "class") != 0) {
		jclass clazz = pEnv->GetObjectClass(object);
		if (clazz != NULL) {
			char achMethodName[1024] = "get";
			strcat(achMethodName, attributeName);
			achMethodName[3] = toupper(achMethodName[3]);

			jmethodID m = pEnv->GetMethodID(clazz, achMethodName,
					"()Ljava/lang/String;");

			if (m != NULL) {
				value = (jstring) pEnv->CallObjectMethod(object, m, NULL);
			} else {
				pEnv->ExceptionClear();
			}
		}
	} else {
		value = getObjectClass();
	}

	if (value != NULL) {
		const char* str = pEnv->GetStringUTFChars(value, NULL);
		result = str;
		pEnv->ReleaseStringUTFChars(value, str);
	}

}

jstring JavaNativeComponent::getObjectClass() {
	JNIEnv *pEnv = JavaVirtualMachine::getCurrent();
	jclass objectC = pEnv->GetObjectClass(object);
	if (objectC == NULL) {
		MZNSendDebugMessageA("WARNING WARNING: Object %x without class\n");
		return NULL;
	}
	jmethodID getClass = pEnv->GetMethodID(objectC, "getClass",
			"()Ljava/lang/Class;");
	if (getClass == NULL) {
		pEnv->ExceptionClear();
		MZNSendDebugMessageA("WARNING WARNING: Object %x without getClass method\n");
		return NULL;
	}
	jobject objectClass = pEnv->CallObjectMethod(object, getClass, NULL);
	if (objectClass == NULL) {
		MZNSendDebugMessageA("WARNING WARNING: getClass method on %x returns NULL\n");
		return NULL;
	}
	// name = objectClass.getName();
	jclass classC = pEnv->GetObjectClass(objectClass);
	jmethodID getName = pEnv->GetMethodID(classC, "getName",
			"()Ljava/lang/String;");
	if (getName == NULL) {
		pEnv->ExceptionClear();
		MZNSendDebugMessageA("WARNING WARNING: %x->getClass does not have getName method\n");
		return NULL;
	}

	jstring name = (jstring) pEnv->CallObjectMethod(objectClass, getName, NULL);
	return name;

}

void JavaNativeComponent::getChildren(std::vector<NativeComponent*> &children) {
	JNIEnv *pEnv = JavaVirtualMachine::getCurrent();
	children.clear ();
	jclass objectC = pEnv->GetObjectClass(object);
	if (objectC != NULL) {
		jmethodID getComponents = pEnv->GetMethodID(objectC, "getComponents",
				"()[Ljava/awt/Component;");
		if (getComponents != NULL) {
			jobjectArray componentArray = (jobjectArray)(pEnv->CallObjectMethod(object, getComponents, NULL));
			jsize elements = pEnv->GetArrayLength(componentArray);
			for(jsize i = 0;i < elements;i++) {
				jobject component = pEnv->GetObjectArrayElement(componentArray, i);
				JavaNativeComponent *child = new JavaNativeComponent(component);
				children.push_back( child );
			}
		} else {
			pEnv->ExceptionClear();
		}
	}
}

static jstring JNU_NewStringNative(JNIEnv *env, const char *str)
{
    jstring result;
    jbyteArray bytes = 0;
    int len;
    if (env->EnsureLocalCapacity( 2) < 0) {
        return NULL;
    }
    len = strlen(str);
    bytes = env->NewByteArray(len);
    if (bytes != NULL) {

        env->SetByteArrayRegion(bytes, 0, len,
                                   (jbyte *)str);

        jclass strcl = env->FindClass("java/lang/String");
        jmethodID constructor = env->GetMethodID(strcl, "<init>",
				"([B)V");
        if (constructor != NULL) {
			result = (jstring) env->NewObject(strcl, constructor, bytes);
			env->DeleteLocalRef(bytes);
	        return result;
        } else {
        	env -> ExceptionClear();
            return NULL;
        }
    }
    return NULL;
}

void JavaNativeComponent::setAttribute(const char*attributeName, const char* value) {
	JNIEnv *pEnv = JavaVirtualMachine::getCurrent();
	jstring jstr = JNU_NewStringNative(pEnv, value);
	char achMethodName[1024] = "set";
	strcat(achMethodName, attributeName);
	achMethodName[3] = toupper(achMethodName[3]);
	jclass cl = pEnv->GetObjectClass(object);
	jmethodID m = pEnv->GetMethodID(cl, achMethodName, "(Ljava/lang/String;)V");
	if (m != NULL)
	{
		pEnv->CallObjectMethod(object, m, jstr, NULL);
	}
	else
	{
		pEnv->ExceptionClear();
		MZNSendDebugMessageA("Warning: Component does not have %s method", achMethodName);
	}
}

void JavaNativeComponent::setFocus() {
	JNIEnv *pEnv = JavaVirtualMachine::getCurrent();
	jclass clazz = pEnv->GetObjectClass(object);
	jmethodID m = pEnv->GetMethodID(clazz, "requestFocusInWindow",
				"()Z");
	if (m == NULL) {
		MZNSendDebugMessage("Method requestFocusInWindow not found");
		pEnv->ExceptionClear();
	} else {
		pEnv->CallObjectMethod(object, m, NULL);
	}
}

void JavaNativeComponent::click() {
	JNIEnv *pEnv = JavaVirtualMachine::getCurrent();

	jclass clazz = pEnv->GetObjectClass(object);
	jmethodID doClickMethod = pEnv->GetMethodID(clazz, "doClick", "()V");
	if (doClickMethod != NULL) {
		pEnv->CallObjectMethod(object, doClickMethod, NULL);
	} else {
		pEnv->ExceptionClear();
		jclass aecl = pEnv->FindClass("java/awt/event/ActionEvent");
		if (aecl != NULL)
		{
			jmethodID aec = pEnv->GetMethodID(aecl, "<init>", "(Ljava/lang/Object;ILjava/lang/String;)V");
			if (aec == NULL) {
				pEnv->ExceptionClear();
				MZNSendDebugMessageA("Cannot get java.awt.event.ActionEvent constructor");
			} else {
				jstring clickString = pEnv->NewStringUTF("click");
				jobject ae = pEnv->NewObject(aecl, aec, object, 1001, clickString); //1001 = ACTION_PERFORMED
				if (ae == NULL) {
					MZNSendDebugMessageA("Cannot create java.awt.event.ActionEvent object");
				} else {
					jmethodID m = pEnv->GetMethodID(clazz, "dispatchEvent", "(Ljava/awt/AWTEvent;)V");
					if (m != NULL) {
						pEnv->CallObjectMethod(object, m, ae, NULL);
					} else {
						pEnv->ExceptionClear();
						std::string s;
						getAttribute ("class", s);
						MZNSendDebugMessage("Missing dispatchEvent method (class = %s)", s.c_str());
					}
				}
			}
		}
		else
			MZNSendDebugMessage("Unable to locate java.awt.event.ActionEvent class");
	}
	pEnv->ExceptionClear();
}

void JavaNativeComponent::freeze() {
	JNIEnv *pEnv = JavaVirtualMachine::getCurrent();
	jobject hook = JavaVirtualMachine::getHookObject();
	if (hook != NULL)
	{
		jclass hookC = pEnv->GetObjectClass(hook);
		jmethodID put = pEnv->GetMethodID(hookC, "putObject", "(JLjava/lang/Object;)V");
		if (put != NULL) {
			jvalue values [2];
			values[0].j = (jlong) this;
			values[1].l = object;
			pEnv->CallObjectMethodA(hook, put, values);
		}
		else {
			pEnv->ExceptionClear();
			MZNSendDebugMessage("Unable to locate putObject method on hook");
		}
	}
}

void JavaNativeComponent::removeFromFridge() {
	JNIEnv *pEnv = JavaVirtualMachine::getCurrent();
	jobject hook = JavaVirtualMachine::getHookObject();
	if (hook != NULL)
	{
		jclass hookC = pEnv->GetObjectClass(hook);
		jmethodID remove = pEnv->GetMethodID(hookC, "removeObject", "(J)V");
		if (remove != NULL) {
			jvalue values [1];
			values[0].j = (jlong) this;
			pEnv->CallObjectMethodA(hook, remove, values);
		} else {
			pEnv->ExceptionClear();
		}
	}
}


void JavaNativeComponent::unfreeze() {
	JNIEnv *pEnv = JavaVirtualMachine::getCurrent();
	jobject hook = JavaVirtualMachine::getHookObject();

	if (hook != NULL)
	{
		jclass hookC = pEnv->GetObjectClass(hook);
		jmethodID put = pEnv->GetMethodID(hookC, "getObject", "(J)Ljava/lang/Object;");
		if (put != NULL) {
			jvalue values [1];
			values[0].j = (jlong) this;
			object = pEnv->CallObjectMethodA(hook, put, values);
		}
		else {
			pEnv->ExceptionClear();
			MZNSendDebugMessage("Unable to locate getObject method on hook");
		}
	}
}
