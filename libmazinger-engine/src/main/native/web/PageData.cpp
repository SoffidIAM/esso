#include <PageData.h>
#include <stddef.h>
#include <stdio.h>
#include <AbstractWebElement.h>
#include <MazingerInternal.h>
#include <json/JsonMap.h>
#include <json/JsonValue.h>
#include <json/JsonVector.h>
#include <json/Encoder.h>

#include <vector>


using namespace json;


static long parseInt (JsonMap *map, const char *tag)
{
	JsonValue *v = dynamic_cast<JsonValue*>(map->getObject(tag));
	if (v == NULL)
		return 0;
	else
	{
		long result = 0;
		sscanf (v->value.c_str(), " %ld", &result);
		return result;
	}
}

static std::string parseString (JsonMap *map, const char *tag)
{
	JsonValue *v = dynamic_cast<JsonValue*>(map->getObject(tag));
	if (v == NULL)
		return std::string("");
	else
	{
		return v->value;
	}
}

static InputData parseInput (JsonMap* value)
{
	InputData d;
	d.clientHeight = parseInt (value, "clientHeight");
	d.clientWidth = parseInt (value, "clientWidth");
	if (d.clientWidth == 0)
		d.clientWidth = parseInt (value, "clientWitdh");
	d.data_bind = parseString (value, "data_bind");
	d.display = parseString (value, "display");
	d.visibility = parseString (value, "visibility");
	d.id = parseString (value, "id");
	d.name = parseString(value, "name");
	d.offsetHeight = parseInt (value, "offsetHeight");
	d.offsetLeft = parseInt (value, "offsetLeft");
	d.offsetTop = parseInt (value, "offsetTop");
	d.offsetWidth = parseInt (value, "offsetWidth");
	d.soffidId = parseString(value,"soffidId");
	d.style = parseString (value, "style");
	d.text_align = parseString(value, "text_align");
	d.type = parseString(value,"type");
	d.rightAlign = d.text_align == "right";
	d.mirrorOf = parseString(value, "mirrorOf");
	d.inputType = parseString(value, "inputType");
	return d;
}

static FormData parseForm (JsonMap* value)
{
	FormData d;
	d.action = parseString(value, "action");
	d.id = parseString(value, "id");
	d.method = parseString(value, "method");
	d.name = parseString(value, "name");
	d.soffidId = parseString(value,"soffidId");
	JsonVector *v = dynamic_cast<JsonVector*> (value->getObject("inputs"));
	if (v != NULL)
	{
		std::vector<JsonAbstractObject*> values = v->objects;
		for (std::vector<JsonAbstractObject*>::iterator it = values.begin();
				it != values.end(); it++)
		{
			JsonMap *input =  dynamic_cast<JsonMap*>(*it);
			if (input != NULL)
				d.inputs.push_back( parseInput(input));
		}
	}
	return d;
}

static void parsePageData (JsonMap* map, PageData *pd)
{
	if (map == NULL)
		return;

	JsonValue *val = dynamic_cast<JsonValue*> (map->getObject("title"));
	if (val != NULL)
		pd->title = val->value;
	val = dynamic_cast<JsonValue*> (map->getObject("url"));
	if (val != NULL)
		pd->url = val->value;

	JsonVector *v = dynamic_cast <JsonVector*> (map->getObject("inputs"));
	if (v != NULL)
	{
		std::vector<JsonAbstractObject*> values = v->objects;
		for (std::vector<JsonAbstractObject*>::iterator it = values.begin ();
				it != values.end();
				it ++)
		{
			JsonMap *i = dynamic_cast<JsonMap*> (*it);
			if (i != NULL)
			{
				pd->inputs.push_back(parseInput (i));
			}
		}
	}
	v = dynamic_cast <JsonVector*> (map->getObject("forms"));
	if (v != NULL)
	{
		std::vector<JsonAbstractObject*> values = v->objects;
		for (std::vector<JsonAbstractObject*>::iterator it = values.begin ();
				it != values.end();
				it ++)
		{
			JsonMap *i = dynamic_cast<JsonMap*> (*it);
			if (i != NULL)
			{
				pd->forms.push_back(parseForm (i));
			}
		}
	}
}

InputData::InputData() {
}

InputData::InputData(const InputData &other) {
	this->clientHeight = other.clientHeight;
	this->clientWidth = other.clientWidth;
	this->data_bind = other.data_bind;
	this->display = other.display;
	this->id = other.id;
	this->name = other.name;
	this->offsetHeight = other.offsetHeight;
	this->offsetLeft = other.offsetLeft;
	this->offsetTop = other.offsetTop;
	this->offsetWidth = other.offsetWidth;
	this->rightAlign = other.rightAlign;
	this->soffidId = other.soffidId;
	this->style = other.style;
	this->text_align = other.text_align;
	this->type = other.type;
	this->value = other.value;
	this->visibility = other.visibility;
	this->inputType = other.inputType;
	this->mirrorOf = other.mirrorOf;
}

InputData &InputData::operator =(const InputData &other) {
	this->clientHeight = other.clientHeight;
	this->clientWidth = other.clientWidth;
	this->data_bind = other.data_bind;
	this->display = other.display;
	this->id = other.id;
	this->name = other.name;
	this->offsetHeight = other.offsetHeight;
	this->offsetLeft = other.offsetLeft;
	this->offsetTop = other.offsetTop;
	this->offsetWidth = other.offsetWidth;
	this->rightAlign = other.rightAlign;
	this->soffidId = other.soffidId;
	this->style = other.style;
	this->text_align = other.text_align;
	this->type = other.type;
	this->value = other.value;
	this->visibility = other.visibility;
	this->inputType = other.inputType;
	this->mirrorOf = other.mirrorOf;
	return *this;
}

InputData::~InputData() {
}

FormData::FormData() {
}

FormData::FormData(const FormData &other) {
	this->action = other.action;
	this->id = other.id;
	for (std::vector<InputData>::const_iterator it = other.inputs.begin();
			it != other.inputs.end();
			it++ )
	{
		this->inputs.push_back(*it);
	}
	this->method = other.method;
	this->name = other.name;
	this->soffidId = other.soffidId;
}

FormData &FormData::operator =(const FormData &other) {
	this->action = other.action;
	this->id = other.id;
	for (std::vector<InputData>::const_iterator it = other.inputs.begin();
			it != other.inputs.end();
			it++ )
	{
		this->inputs.push_back(*it);
	}
	this->method = other.method;
	this->name = other.name;
	this->soffidId = other.soffidId;
	return *this;
}


FormData::~FormData() {
}

PageData::PageData() {
}

PageData::~PageData() {
}

PageData::PageData(const PageData& other) {
	this->title = other.title;
	this->url = other.url;
	for (std::vector<InputData>::const_iterator it = other.inputs.begin (); it != other.inputs.end(); it++)
		this->inputs.push_back(*it);
	for (std::vector<FormData>::const_iterator it = other.forms.begin (); it != other.forms.end(); it++)
		this->forms.push_back(*it);
}

PageData& PageData::operator =(const PageData& other) {
	this->title = other.title;
	this->url = other.url;
	for (std::vector<InputData>::const_iterator it = other.inputs.begin (); it != other.inputs.end(); it++)
		this->inputs.push_back(*it);
	for (std::vector<FormData>::const_iterator it = other.forms.begin (); it != other.forms.end(); it++)
		this->forms.push_back(*it);
	return *this;
}

void PageData::loadJson (const char *jsontext) {
	JsonAbstractObject *obj = JsonAbstractObject::readObject(jsontext);
	if (obj != NULL)
	{
		JsonMap *map = dynamic_cast<JsonMap*> (obj);
		if (map != NULL)
			parsePageData(map, this);
		delete obj;
	}
}

void PageData::loadJson (JsonMap *map) {
	parsePageData(map, this);
}

void InputData::dump() {
	MZNSendDebugMessageA("    INPUT  soffidId:     %s", this->soffidId.c_str());
	if (! this->mirrorOf.empty())
		MZNSendDebugMessageA("           soffidMirrorOf: %s",
			mirrorOf.c_str());
	if (! this->inputType.empty())
		MZNSendDebugMessageA("           soffidInputType: %s",
			inputType.c_str());
	if (! this->id.empty())
		MZNSendDebugMessageA("           id:          %s", this->id.c_str());
	if (! this->name.empty())
		MZNSendDebugMessageA("           name:        %s", this->name.c_str());
	if (! this->type.empty())
		MZNSendDebugMessageA("           type:        %s", this->type.c_str());
	if (! this->value.empty())
		MZNSendDebugMessageA("           value:       %s", this->value.c_str());
	if (! this->soffidId.empty())
		MZNSendDebugMessageA("           soffidId:    %s", this->soffidId.c_str());
	if (! this->inputType.empty())
		MZNSendDebugMessageA("           input type:  %s", this->inputType.c_str());
	MZNSendDebugMessageA("           pos:    (%ld, %ld, %ld, %ld) (%ld, %ld)",
			offsetTop, offsetLeft, offsetHeight, offsetWidth,
			clientHeight, clientWidth);
	if (! this->display.empty() || ! this->visibility.empty())
		MZNSendDebugMessageA("           style:  display: %s; visibility: %s",
			display.c_str(), visibility.c_str());

}

void FormData::dump() {
	MZNSendDebugMessageA("  FORM action: %s", this->action.c_str());
	MZNSendDebugMessageA("       id:     %s", this->id.c_str());
	MZNSendDebugMessageA("       name:   %s", this->name.c_str());
	MZNSendDebugMessageA("       method: %s", this->method.c_str());
	for (std::vector<InputData>::iterator it = inputs.begin(); it != inputs.end(); it++)
		it->dump();
}

void PageData::dump() {
	MZNSendDebugMessageA("PAGE : %s", this->url.c_str());
	MZNSendDebugMessageA("Title: %s", this->title.c_str());
	for (std::vector<InputData>::iterator it = inputs.begin(); it != inputs.end(); it++)
		it->dump();
	for (std::vector<FormData>::iterator it = forms.begin(); it != forms.end(); it++)
		it->dump();
}
