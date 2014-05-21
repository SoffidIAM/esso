/*
 * ExplorerElement.cpp
 *
 *  Created on: 11/11/2010
 *      Author: u07286
 */
#define NS_SCRIPTABLE


#include <nsIDOMHTMLElement.h>
#include <nsIDOMHTMLInputElement.h>
#include <nsIDOMHTMLInputElement4.h>
#include "nsCOMPtr.h"
#include <nsIDOMDocument.h>
#include <nsIDOMNodeList.h>
#include <nsIDOMNode.h>
#include <nsIDOMDocumentEvent.h>
#include <nsIDOMEvent.h>
#include <nsIDOMEventTarget.h>
#include <nsEmbedString.h>
#include "FFElement.h"
#undef CreateEvent
#include <stdio.h>

FFElement::FFElement(nsCOMPtr<nsIDOMHTMLElement> &pElement):
	m_Element(pElement)
{
}

FFElement::~FFElement() {
}



void FFElement::getChildren(std::vector<AbstractWebElement*> &children)
{
	children.clear();
	PRUint32 size;
	nsCOMPtr<nsIDOMNodeList> nodeList;

	nsresult rv = m_Element->GetChildNodes(getter_AddRefs(nodeList));
	if (!NS_FAILED(rv))
	{
		rv = nodeList->GetLength(&size);
	}
	if (!NS_FAILED(rv))
	{
		for (PRUint32 i = 0; i < size; i++)
		{
			nsCOMPtr<nsIDOMNode> node;
			rv = nodeList->Item(i, getter_AddRefs(node));
		    nsEmbedString strName;
			node->GetNodeName(strName);
			nsEmbedCString strCName = NS_ConvertUTF16toUTF8(strName);
			nsCOMPtr<nsIDOMHTMLElement> el = do_QueryInterface(node, &rv);
			if ( ! NS_FAILED(rv))
			{
				children.push_back(new FFElement(el));
			}
		}
	}
}



void FFElement::click()
{
	nsresult rv;
	nsCOMPtr<nsIDOMHTMLInputElement> doc;

	doc = do_QueryInterface(m_Element, &rv);
	if (!NS_FAILED(rv))
	{
		rv = doc->Click();
	} else {
		nsCOMPtr<nsIDOMHTMLInputElement4> doc4;

		doc4 = do_QueryInterface(m_Element, &rv);
		if (!NS_FAILED(rv))
		{
			rv = doc4->Click();
		}
	}
}



void FFElement::getAttribute(const char *attribute, std::string & value)
{
    nsEmbedString strName;
    strName.AppendLiteral(attribute);
    nsEmbedString strValue;
	nsresult rv = m_Element->GetAttribute(strName, strValue);
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



void FFElement::blur()
{
	nsresult rv;
	nsCOMPtr<nsIDOMHTMLInputElement> doc;

	doc = do_QueryInterface(m_Element, &rv);
	if (!NS_FAILED(rv))
	{
		rv = doc->Blur();
	} else {
		nsCOMPtr<nsIDOMHTMLInputElement4> doc4;

		doc4 = do_QueryInterface(m_Element, &rv);
		if (!NS_FAILED(rv))
		{
			rv = doc4->Blur();
		}
	}
}



AbstractWebElement *FFElement::getParent()
{
	nsCOMPtr<nsIDOMNode> parent;

	nsresult rv = m_Element->GetParentNode(getter_AddRefs(parent));
	if (!NS_FAILED(rv))
	{
		nsCOMPtr<nsIDOMHTMLElement> el = do_QueryInterface(parent, &rv);
		if ( ! NS_FAILED(rv))
		{
			return new FFElement(el);
		}
	}
	return NULL;
}



void FFElement::setAttribute(const char *attribute, const char*value)
{
	nsEmbedCString strName ;
    strName.AssignLiteral(attribute);
    nsEmbedCString strValue;
    strValue.AssignLiteral(value);
	m_Element->SetAttribute(NS_ConvertUTF8toUTF16(strName),
			NS_ConvertUTF8toUTF16(strValue));
}



void FFElement::focus()
{
	nsresult rv;
	nsCOMPtr<nsIDOMHTMLInputElement> doc;

	doc = do_QueryInterface(m_Element, &rv);
	if (!NS_FAILED(rv))
	{
		rv = doc->Focus();
	} else {
		nsCOMPtr<nsIDOMHTMLInputElement4> doc4;

		doc4 = do_QueryInterface(m_Element, &rv);
		if (!NS_FAILED(rv))
		{
			rv = doc4->Focus();
		}
	}
}



void FFElement::getTagName(std::string & value)
{
    nsEmbedString strValue;
	nsresult rv = m_Element->GetTagName(strValue);
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



AbstractWebElement *FFElement::clone()
{
	return new FFElement(m_Element);
}


