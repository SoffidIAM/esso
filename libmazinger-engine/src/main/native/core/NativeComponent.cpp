#include <MazingerInternal.h>
#include "NativeComponent.h"

NativeComponent::NativeComponent() {

}

NativeComponent::~NativeComponent()
{
}

void NativeComponent::freeze()
{
}

void NativeComponent::unfreeze()
{
}

void NativeComponent::dumpAttribute  (const char *attribute) {
//	MZNSendDebugMessage("Dumping %s", attribute);
	std::string value;
	getAttribute(attribute, value);
	if (value.length() != 0)
	{
		MZNSendDebugMessage("%s: [%s]", attribute, value.c_str());
	}
}

void NativeComponent::dump () {

	dumpAttribute ("class");
	dumpAttribute ("name");
	dumpAttribute ("title");
	dumpAttribute ("text");
	dumpAttribute ("dlgId");
}

void NativeComponent::dumpXML (int depth) {
	std::string className;
	std::string name;
	std::string title;
	std::string dlgId;
	std::string text;
	getAttribute ("class",className);
	getAttribute ("name",name);
	getAttribute ("title",title);
	getAttribute ("text",text);
	getAttribute ("dlgId",dlgId);
	std::string prefix;
	for (int i = 0; i < depth; i++)
		prefix.append(" ");
	std::string line;
	line.append ("<Component class='");
	line.append (xmlEncode(className.c_str()));
	line.append ("'");
	std::vector<NativeComponent*> children;
	getChildren(children);
	const char *terminator;
	if (children.size() == 0)
		terminator = "/>";
	else
		terminator = ">";
	if (name.length() == 0 && title.length() == 0 && text.length() == 0
			&& dlgId.length() == 0)
	{
		MZNSendSpyMessage ("%s%s%s", prefix.c_str(),
					line.c_str(),
					terminator);
	} else {
		MZNSendSpyMessage ("%s%s", prefix.c_str(), line.c_str());
		if (name.length() > 0)
			MZNSendSpyMessage ("%s name='^%s$'%s",
					prefix.c_str(),
					xmlEncode(name.c_str()),
					(title.length() == 0 && text.length() == 0 && dlgId.length() == 0) ? terminator: "");
		if (dlgId.length() > 0)
			MZNSendSpyMessage ("%s dlgId='^%s$'%s",
					prefix.c_str(),
					xmlEncode(dlgId.c_str()),
					(title.length() == 0 && text.length() == 0) ? terminator: "");
		if (title.length() > 0)
			MZNSendSpyMessage ("%s title='^%s$'%s",
					prefix.c_str(),
					xmlEncode(title.c_str()),
					(text.length() == 0) ? terminator: "");
		if (text.length() > 0)
			MZNSendSpyMessage ("%s text='^%s$'%s",
					prefix.c_str(),
					xmlEncode(text.c_str()),
					terminator);
	}

	if (children.size() > 0)
	{
		for (unsigned int i = 0 ; i < children.size(); i++)
		{
			children[i]->dumpXML(depth + 2);
			delete children[i];
		}
		MZNSendSpyMessage ("%s</Component>", prefix.c_str());
	}

}

