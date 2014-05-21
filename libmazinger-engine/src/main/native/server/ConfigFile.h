/*
 * ConfigFile.h
 *
 *  Created on: 29/04/2011
 *      Author: u07286
 */

#ifndef CONFIGFILE_H_
#define CONFIGFILE_H_

struct ConfigProperty {
	char *name;
	char *value;
	int len;
};

class ConfigFile {
public:
	ConfigFile();
	virtual ~ConfigFile();

	void load (const char *file);
	void save (const char *file);
	const char *getValue (const char* name);
	void setValue(const char* name, const char* value);
private:
	struct ConfigProperty *properties;
	int num;
	int allocated;

	void ensureCapacity();
};

#endif /* CONFIGFILE_H_ */
