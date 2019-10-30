
#ifndef __PAGE_DATA_H
#define __PAGE_DATA_H

#include <vector>
#include <string>
#include <json/JsonMap.h>


class AbstractWebElement;

struct InputData
{
public:
	InputData ();
	InputData (const InputData &other);
	InputData& operator = (const InputData &other);
	virtual ~InputData();

	std::string soffidId;

	std::string name;
	std::string id;
	std::string value;
	std::string data_bind;
	std::string display;
	std::string visibility;
	std::string type;
	std::string style;
	std::string text_align;
	std::string inputType;
	std::string mirrorOf;

	long offsetLeft;
	long offsetTop;
	long offsetWidth;
	long offsetHeight;
	long clientWidth;
	long clientHeight;
	bool rightAlign;
	void dump();
};

struct FormData
{
	FormData () ;
	FormData (const FormData &other) ;
	FormData& operator = (const FormData &other);
	virtual ~FormData ();
	std::string soffidId;
	AbstractWebElement * getElement ();
	std::vector<InputData> inputs;
	std::string action;
	std::string method;
	std::string id;
	std::string name;
	void dump();
};

struct PageData
{
	PageData () ;
	PageData& operator= (const PageData &other);
	PageData (const PageData &other);
	virtual ~PageData ();

	void loadJson (const char *json);
	void loadJson (json::JsonMap *jsonMap);
	void dump ();

	std::string url;
	std::string title;

	std::vector<InputData> inputs;
	std::vector<FormData> forms;
};


#endif
