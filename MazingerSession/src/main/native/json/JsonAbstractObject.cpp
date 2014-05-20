/*
 * AbstractObject.cpp
 *
 *  Created on: 12/03/2014
 *      Author: bubu
 */

#include "JsonAbstractObject.h"
#include "JsonMap.h"
#include "JsonVector.h"
#include "JsonValue.h"
#include <stdlib.h>
#include <stdio.h>
#include <MazingerInternal.h>

namespace json {

JsonAbstractObject::JsonAbstractObject() {
	// TODO Auto-generated constructor stub

}

JsonAbstractObject::~JsonAbstractObject() {
	// TODO Auto-generated destructor stub
}

const char *JsonAbstractObject::skipSpaces (const char *str)
{
	while (*str <= ' ')
		str ++;
	return str;
}

JsonAbstractObject* JsonAbstractObject::readObject(const char* &str) {
	str = skipSpaces(str);
	JsonAbstractObject *o;
	if (*str == '[')
	{
		o= new JsonVector();
	}
	else if (*str == '{')
	{
		o = new JsonMap ();

	} else {
		o = new JsonValue ();
	}
	str = o->read(str);
	str = skipSpaces(str);

	return o;
}

void JsonAbstractObject::writeIndent(std::string& str, int indent) {
	while (indent --)
	{
		str.append(" ");
	}
}


#ifdef WIN32

#include <windows.h>
#include <direct.h>

#define PLUGIN_NAME "\"AfroditaC\""
#define PLUGIN_EXTENSION "\"dll\""

#else

#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>

#define PLUGIN_NAME "\"libafroditac\""
#define PLUGIN_EXTENSION "\"so\""

#endif

static const char * tags[] = {
		"\"profile\"",
		"\"content_settings\"",
		"\"plugin_whitelist\"",
		PLUGIN_NAME,
		NULL
};

void JsonAbstractObject::ConfigureChromePreferences ()
{
#ifdef WIN32
	const char *appdata = getenv ("LOCALAPPDATA");
	if (appdata == NULL)
		appdata = getenv ("APPDATA");
	if (appdata == NULL)
		return;

	std::string file = appdata;
	file.append ("\\Google\\Chrome\\User Data\\Default\\Preferences");

#else
	const char *user = MZNC_getUserName();

	struct passwd *pwd = getpwnam (user);
	if (pwd == NULL || pwd->pw_dir == NULL || pwd->pw_dir[0] == '\0')
		return ;
	std::string file = pwd->pw_dir;
	file.append ("/.config/google-chrome/Default/Preferences");


#endif

	FILE *f = fopen (file.c_str(), "r");
	std::string s;

	if ( f != NULL )
	{
		char buffer[1024];
		int size = 0;
		while ( (size = fread (buffer, 1, 1024, f)) > 0)
		{
			s.append (buffer, size);
		}
		fclose (f);

		const char *str = s.c_str();

		json::JsonAbstractObject *root =  json::JsonAbstractObject::readObject (str);
		json::JsonMap *map =  dynamic_cast<json::JsonMap*> (root);

		for (int i = 0; tags[i]; i++)
		{
			json::JsonMap *object = dynamic_cast<json::JsonMap*>
				(map->getObject(tags[i]));
			if (object == NULL)
			{
				object = new json::JsonMap();
				map->setObject(tags[i], object);
			}
			map = object;
		}

		s.clear ();
		map->write (s,0);
		bool save = false;
		json::JsonValue *v = dynamic_cast<json::JsonValue*>(map->getObject(PLUGIN_EXTENSION));
		if (v == NULL)
		{
			v = new json::JsonValue();
			v->value = "true";
			save = true;
		}
		else if ( v->value != "true" )
		{
			save = true;
			v->value = "true";
		}

		if (save)
		{
			std::string s;
			root->write(s, 0);
			FILE *f = fopen ((file).c_str(), "w");
			if ( f != NULL)
			{
				fwrite (s.c_str(), 1, s.length(), f);
				fclose (f);
			}
		}
	} else {
#ifdef WIN32
		std::string file = appdata;
		file.append ("\\Google");
		mkdir (file.c_str());
		file.append ("\\Chrome");
		mkdir (file.c_str());
		file.append ("\\User Data");
		mkdir (file.c_str());
		file.append ("\\Default");
		mkdir (file.c_str());
		file.append ("\\Preferences");
#else
		std::string file = pwd->pw_dir;
		file.append ("/.config");
		mkdir (file.c_str(), 0700);
		chown (file.c_str(), pwd->pw_uid, pwd->pw_gid);
		file.append ("/google-chrome");
		mkdir (file.c_str(), 0700);
		chown (file.c_str(), pwd->pw_uid, pwd->pw_gid);
		file.append ("/Default");
		mkdir (file.c_str(), 0700);
		chown (file.c_str(), pwd->pw_uid, pwd->pw_gid);
		file.append ("/Preferences");
#endif
		FILE *f = fopen ((file).c_str(), "w");
		if ( f != NULL)
		{
			s = "{\n"
				"   \"profile\": {\n"
				"      \"content_settings\": {\n"
				"         \"plugin_whitelist\": {\n"
				"            "PLUGIN_NAME": {\n"
				"               "PLUGIN_EXTENSION": true\n"
				"            }\n"
				"         }\n"
				"      }\n"
				"   }\n"
				"}\n";
			fwrite (s.c_str(), 1, s.length(), f);
			fclose (f);
		}
	}


}

} /* namespace json */
