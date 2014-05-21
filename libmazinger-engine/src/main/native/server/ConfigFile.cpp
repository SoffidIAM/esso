/*
 * ConfigFile.cpp
 *
 *  Created on: 29/04/2011
 *      Author: u07286
 */

#include "ConfigFile.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <string>

static void clean (ConfigProperty * &properties, int &num, int &allocated) {
	if (properties != NULL) {
		for (int i = 0;i < num; i++)
		{
			if (properties[i].name != NULL) free (properties[i].name);
			if (properties[i].value != NULL) free (properties[i].value);
		}
	}
	free (properties);
	properties = NULL;
	num = 0;
	allocated = 0;
}

void ConfigFile::ensureCapacity () {
	if (allocated == 0)
	{
		allocated = 10;
		properties = (ConfigProperty*) malloc ( allocated * sizeof (ConfigProperty));
	}
	else if (num + 1 > allocated)
	{
		allocated += 10;
		properties = (ConfigProperty*) realloc (properties, allocated * sizeof (ConfigProperty));
	}

}


static char* dupString (const char* str) {
	char *r = (char*) malloc(strlen(str)+1);
	strcpy (r, str);
	return r;
}

void ConfigFile::load (const char*file) {
	clean (properties, num, allocated);

	FILE* f = fopen (file, "r");
	if (f != NULL) {
		char line[1024];

		while ( fgets(line, 1023, f) != NULL)
		{
			for (int i = 0; line [i] ; i++)
			{
				if (line[i] == '#') {
					line [i] = '\0';
					break;
				}
			}
			char *igual = strstr(line, ":");
			if (igual != NULL) {
				ensureCapacity ();
				int l = strlen(line);
				while (l > 0 && line[l-1] == '\n') line [--l] = '\0';
				*igual = '\0';
				char *name = line;
				char *value = igual+1;
				properties[num].name = dupString(name);
				properties[num].value = dupString(value);
				num ++;
			}
		}
	}
}

void ConfigFile::save (const char*file) {
	std::string newFile;
	newFile.assign(file);
	newFile.append (".tmp");
	FILE* f = fopen (newFile.c_str(), "w");
	if (f != NULL) {
		time_t t;
		time(&t);
		struct tm *tm = localtime(&t);
		char achTime[35];
		strftime (achTime, 34, "%d-%m-%Y %H:%M:%S", tm);
		fprintf (f, "# Fitxer autogenerat a les %s\n", achTime);

		for (int i = 0; i < num; i++){
			if (properties[i].value != NULL)
				fprintf(f, "%s:%s\n", properties[i].name, properties[i].value);
		}
		fclose (f);
		std::string backupFile = file;
		backupFile.append (".bak");
		remove(backupFile.c_str());
		rename(file, backupFile.c_str());
		rename(newFile.c_str(), file);

	}
}


const char *ConfigFile::getValue (const char* name) {
	for (int i = 0; i < num; i++)
	{
		if (strcmp (name, properties[i].name) == 0)
			return properties[i].value;

	}
	return NULL;
}


void ConfigFile::setValue(const char* name, const char* value) {
	bool found = false;
	for (int i = 0; i < num; i++)
	{
		if (strcmp (name, properties[i].name) == 0)
		{
			found = true;
			if (properties[i].value != NULL)
				free (properties[i].value);
			if (value == NULL)
				properties[i].value = NULL;
			else
				properties[i].value = dupString(value);
		}
	}
	if (! found ) {
		ensureCapacity ();
		properties[num].name = dupString(name);
		properties[num].value = dupString(value);
		num++;
	}
}


ConfigFile::ConfigFile() {
	num = 0;
	properties = NULL;
	allocated = 0;
}

ConfigFile::~ConfigFile() {
	clean (properties, num, allocated);
}
