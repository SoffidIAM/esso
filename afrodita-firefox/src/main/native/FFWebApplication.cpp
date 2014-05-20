/*
 * ExplorerWebApplication.cpp
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#include <prtypes.h>
#include <nsCOMPtr.h>
#include <nsIDOMNodeList.h>
#include <nsIDOMNode.h>
#include <nsIDOMHTMLCollection.h>
#include <nsIDOMHTMLElement.h>
#include <nsIDOMHTMLDocument.h>
#include <nsEmbedString.h>
#include "FFWebApplication.h"
#include "FFElement.h"


FFWebApplication::~FFWebApplication() {
}


void FFWebApplication::getUrl(std::string & value)
{
    nsEmbedString strValue;
    nsresult rv = m_Document->GetURL(strValue);
	if (!NS_FAILED(rv))
	{

		nsEmbedCString strValue8 = NS_ConvertUTF16toUTF8(strValue);
		value.assign (strValue8.get());
	}
	else
	{
		value.clear();
	}

}



void FFWebApplication::getTitle(std::string & value)
{
    nsEmbedString strValue;
    nsresult rv = m_Document->GetTitle(strValue);
	if (!NS_FAILED(rv))
	{
		nsEmbedCString strValue8 = NS_ConvertUTF16toUTF8(strValue);
		value.assign (strValue8.get());
	}
	else
	{
		value.clear();
	}
}



void FFWebApplication::getContent(std::string & value)
{
	value.assign("<not supported>");
}

FFWebApplication::FFWebApplication(nsCOMPtr<nsIDOMHTMLDocument> &document):
	m_Document(document)
{
}

AbstractWebElement *FFWebApplication::getDocumentElement()
{
	nsCOMPtr<nsIDOMElement> element;
	nsresult rv;
	rv = m_Document->GetDocumentElement(getter_AddRefs(element));
	if (!NS_FAILED(rv) && element.get() != nsnull)
	{
		nsCOMPtr<nsIDOMHTMLElement> htmlElement = do_QueryInterface(element, &rv);

		if (! NS_FAILED(rv) )
			return new FFElement(htmlElement);
	}
	return NULL;
}


void FFWebApplication:: populateVector(
		nsCOMPtr<nsIDOMNodeList> &nodeList,
		std::vector<AbstractWebElement*> & elements)
{
	PRUint32 size;
	nsresult rv = nodeList->GetLength(&size);
	elements.clear();
	for (PRUint32 i = 0; !NS_FAILED(rv) && i < size; i++)
	{
		nsCOMPtr<nsIDOMNode> node;
		rv = nodeList->Item(i, getter_AddRefs(node));
		nsCOMPtr<nsIDOMHTMLElement> el = do_QueryInterface(node, &rv);
		if ( ! NS_FAILED(rv))
		{
			elements.push_back(new FFElement(el));
		}
	}
}

void FFWebApplication:: populateVector(
		nsCOMPtr<nsIDOMHTMLCollection> &nodeList,
		std::vector<AbstractWebElement*> & elements)
{
	PRUint32 size;
	nsresult rv = nodeList->GetLength(&size);
	elements.clear();
	for (PRUint32 i = 0; !NS_FAILED(rv) && i < size; i++)
	{
		nsCOMPtr<nsIDOMNode> node;
		rv = nodeList->Item(i, getter_AddRefs(node));
		nsCOMPtr<nsIDOMHTMLElement> el = do_QueryInterface(node, &rv);
		if ( ! NS_FAILED(rv))
		{
			elements.push_back(new FFElement(el));
		}
	}
}


void FFWebApplication::getElementsByTagName(const char*tag, std::vector<AbstractWebElement*> & elements)
{
	nsCOMPtr<nsIDOMNodeList> list;
	nsresult rv;
	nsEmbedCString strTag;
	strTag.AssignLiteral(tag);
	elements.clear();
	rv = m_Document->GetElementsByTagName(NS_ConvertUTF8toUTF16(strTag), getter_AddRefs(list));
	if (!NS_FAILED(rv))
	{
		populateVector(list, elements);
	}
}



void FFWebApplication::getImages(std::vector<AbstractWebElement*> & elements)
{
	nsCOMPtr<nsIDOMHTMLCollection> list;
	nsresult rv;
	elements.clear();
	rv = m_Document->GetImages(getter_AddRefs(list));
	if (!NS_FAILED(rv))
	{
		populateVector(list, elements);
	}
}




void FFWebApplication::getLinks(std::vector<AbstractWebElement*> & elements)
{
	nsCOMPtr<nsIDOMHTMLCollection> list;
	nsresult rv;
	elements.clear();
	rv = m_Document->GetLinks(getter_AddRefs(list));
	if (!NS_FAILED(rv))
	{
		populateVector(list, elements);
	}
}



void FFWebApplication::getDomain(std::string & value)
{
    nsEmbedString strValue;
	nsresult rv = m_Document->GetDomain(strValue);
	if (!NS_FAILED(rv))
	{
		nsEmbedCString strValue8 = NS_ConvertUTF16toUTF8(strValue);
		value.assign (strValue8.get());
	}
	else
	{
		value.clear();
	}
}



void FFWebApplication::getAnchors(std::vector<AbstractWebElement*> & elements)
{
	nsCOMPtr<nsIDOMHTMLCollection> list;
	nsresult rv;
	elements.clear();
	rv = m_Document->GetAnchors(getter_AddRefs(list));
	if (!NS_FAILED(rv))
	{
		populateVector(list, elements);
	}
}



void FFWebApplication::getCookie(std::string & value)
{
    nsEmbedString strValue;
	nsresult rv = m_Document->GetCookie(strValue);
	if (!NS_FAILED(rv))
	{
		nsEmbedCString strValue8 = NS_ConvertUTF16toUTF8(strValue);
		value.assign (strValue8.get());
	}
	else
	{
		value.clear();
	}
}



AbstractWebElement *FFWebApplication::getElementById(const char *id)
{
	nsCOMPtr<nsIDOMElement> element;
	nsresult rv;
	nsEmbedCString strId;
	strId.AssignLiteral(id);
	rv = m_Document->GetElementById(NS_ConvertUTF8toUTF16(strId), getter_AddRefs(element));
	if (!NS_FAILED(rv) && element.get() != nsnull)
	{
		nsCOMPtr<nsIDOMHTMLElement> htmlElement = do_QueryInterface(element, &rv);

		if (! NS_FAILED(rv) )
			return new FFElement(htmlElement);
	}
	return NULL;
}



void FFWebApplication::getForms(std::vector<AbstractWebElement*> & elements)
{
	nsCOMPtr<nsIDOMHTMLCollection> list;
	nsresult rv;
	elements.clear();
	rv = m_Document->GetForms(getter_AddRefs(list));
	if (!NS_FAILED(rv))
	{
		populateVector(list, elements);
	}
}



void FFWebApplication::write(const char *str)
{
	nsCOMPtr<nsIDOMNodeList> list;
	nsEmbedCString strText;
	strText.AssignLiteral(str);
	m_Document->Writeln(NS_ConvertUTF8toUTF16(strText));
}

void FFWebApplication::writeln(const char *str)
{
	nsCOMPtr<nsIDOMNodeList> list;
	nsEmbedCString strText;
	strText.AssignLiteral(str);
	m_Document->Writeln(NS_ConvertUTF8toUTF16(strText));
}







