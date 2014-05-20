/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM nsIDOMNSElement.idl
 */

#ifndef __gen_nsIDOMNSElement_h__
#define __gen_nsIDOMNSElement_h__


#ifndef __gen_domstubs_h__
#include "domstubs.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIDOMNSElement */
#define NS_IDOMNSELEMENT_IID_STR "c9da11bc-32d4-425e-a91f-7e0939c39251"

#define NS_IDOMNSELEMENT_IID \
  {0xc9da11bc, 0x32d4, 0x425e, \
    { 0xa9, 0x1f, 0x7e, 0x09, 0x39, 0xc3, 0x92, 0x51 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIDOMNSElement : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMNSELEMENT_IID)

  /* nsIDOMNodeList getElementsByClassName (in DOMString classes); */
  NS_SCRIPTABLE NS_IMETHOD GetElementsByClassName(const nsAString & classes, nsIDOMNodeList **_retval NS_OUTPARAM) = 0;

  /* nsIDOMClientRectList getClientRects (); */
  NS_SCRIPTABLE NS_IMETHOD GetClientRects(nsIDOMClientRectList **_retval NS_OUTPARAM) = 0;

  /**
   * Returns the union of all rectangles in the getClientRects() list. Empty
   * rectangles are ignored, except that if all rectangles are empty,
   * we return an empty rectangle positioned at the top-left of the first
   * rectangle in getClientRects().
   */
  /* nsIDOMClientRect getBoundingClientRect (); */
  NS_SCRIPTABLE NS_IMETHOD GetBoundingClientRect(nsIDOMClientRect **_retval NS_OUTPARAM) = 0;

  /**
   * The vertical scroll position of the element, or 0 if the element is not
   * scrollable. This property may be assigned a value to change the
   * vertical scroll position.
   */
  /* attribute long scrollTop; */
  NS_SCRIPTABLE NS_IMETHOD GetScrollTop(PRInt32 *aScrollTop) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetScrollTop(PRInt32 aScrollTop) = 0;

  /**
   * The horizontal scroll position of the element, or 0 if the element is not
   * scrollable. This property may be assigned a value to change the
   * horizontal scroll position.
   */
  /* attribute long scrollLeft; */
  NS_SCRIPTABLE NS_IMETHOD GetScrollLeft(PRInt32 *aScrollLeft) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetScrollLeft(PRInt32 aScrollLeft) = 0;

  /**
   * The height of the scrollable area of the element. If the element is not
   * scrollable, scrollHeight is equivalent to the offsetHeight.
   */
  /* readonly attribute long scrollHeight; */
  NS_SCRIPTABLE NS_IMETHOD GetScrollHeight(PRInt32 *aScrollHeight) = 0;

  /**
   * The width of the scrollable area of the element. If the element is not
   * scrollable, scrollWidth is equivalent to the offsetWidth.
   */
  /* readonly attribute long scrollWidth; */
  NS_SCRIPTABLE NS_IMETHOD GetScrollWidth(PRInt32 *aScrollWidth) = 0;

  /**
   * The height in CSS pixels of the element's top border.
   */
  /* readonly attribute long clientTop; */
  NS_SCRIPTABLE NS_IMETHOD GetClientTop(PRInt32 *aClientTop) = 0;

  /**
   * The width in CSS pixels of the element's left border and scrollbar
   * if it is present on the left side.
   */
  /* readonly attribute long clientLeft; */
  NS_SCRIPTABLE NS_IMETHOD GetClientLeft(PRInt32 *aClientLeft) = 0;

  /**
   * The width in CSS pixels of the element's padding box. If the element is
   * scrollable, the scroll bars are included inside this height.
   */
  /* readonly attribute long clientHeight; */
  NS_SCRIPTABLE NS_IMETHOD GetClientHeight(PRInt32 *aClientHeight) = 0;

  /**
   * The height in CSS pixels of the element's padding box. If the element is
   * scrollable, the scroll bars are included inside this width.
   */
  /* readonly attribute long clientWidth; */
  NS_SCRIPTABLE NS_IMETHOD GetClientWidth(PRInt32 *aClientWidth) = 0;

  /**
   * Similar as the attributes on nsIDOMNode, but navigates just elements
   * rather than all nodes.
   *
   * Defined by the ElementTraversal spec.
   */
  /* readonly attribute nsIDOMElement firstElementChild; */
  NS_SCRIPTABLE NS_IMETHOD GetFirstElementChild(nsIDOMElement * *aFirstElementChild) = 0;

  /* readonly attribute nsIDOMElement lastElementChild; */
  NS_SCRIPTABLE NS_IMETHOD GetLastElementChild(nsIDOMElement * *aLastElementChild) = 0;

  /* readonly attribute nsIDOMElement previousElementSibling; */
  NS_SCRIPTABLE NS_IMETHOD GetPreviousElementSibling(nsIDOMElement * *aPreviousElementSibling) = 0;

  /* readonly attribute nsIDOMElement nextElementSibling; */
  NS_SCRIPTABLE NS_IMETHOD GetNextElementSibling(nsIDOMElement * *aNextElementSibling) = 0;

  /**
   * Returns the number of child nodes that are nsIDOMElements.
   *
   * Defined by the ElementTraversal spec.
   */
  /* readonly attribute unsigned long childElementCount; */
  NS_SCRIPTABLE NS_IMETHOD GetChildElementCount(PRUint32 *aChildElementCount) = 0;

  /**
   * Returns a live nsIDOMNodeList of the current child elements.
   */
  /* readonly attribute nsIDOMNodeList children; */
  NS_SCRIPTABLE NS_IMETHOD GetChildren(nsIDOMNodeList * *aChildren) = 0;

  /**
   * Returns a DOMTokenList object reflecting the class attribute.
   */
  /* readonly attribute nsIDOMDOMTokenList classList; */
  NS_SCRIPTABLE NS_IMETHOD GetClassList(nsIDOMDOMTokenList * *aClassList) = 0;

  /**
   * Returns whether this element would be selected by the given selector string.
   *
   * See http://dev.w3.org/2006/webapi/selectors-api2/#matchtesting
   */
  /* boolean mozMatchesSelector (in DOMString selector); */
  NS_SCRIPTABLE NS_IMETHOD MozMatchesSelector(const nsAString & selector, PRBool *_retval NS_OUTPARAM) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMNSElement, NS_IDOMNSELEMENT_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDOMNSELEMENT \
  NS_SCRIPTABLE NS_IMETHOD GetElementsByClassName(const nsAString & classes, nsIDOMNodeList **_retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GetClientRects(nsIDOMClientRectList **_retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GetBoundingClientRect(nsIDOMClientRect **_retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GetScrollTop(PRInt32 *aScrollTop); \
  NS_SCRIPTABLE NS_IMETHOD SetScrollTop(PRInt32 aScrollTop); \
  NS_SCRIPTABLE NS_IMETHOD GetScrollLeft(PRInt32 *aScrollLeft); \
  NS_SCRIPTABLE NS_IMETHOD SetScrollLeft(PRInt32 aScrollLeft); \
  NS_SCRIPTABLE NS_IMETHOD GetScrollHeight(PRInt32 *aScrollHeight); \
  NS_SCRIPTABLE NS_IMETHOD GetScrollWidth(PRInt32 *aScrollWidth); \
  NS_SCRIPTABLE NS_IMETHOD GetClientTop(PRInt32 *aClientTop); \
  NS_SCRIPTABLE NS_IMETHOD GetClientLeft(PRInt32 *aClientLeft); \
  NS_SCRIPTABLE NS_IMETHOD GetClientHeight(PRInt32 *aClientHeight); \
  NS_SCRIPTABLE NS_IMETHOD GetClientWidth(PRInt32 *aClientWidth); \
  NS_SCRIPTABLE NS_IMETHOD GetFirstElementChild(nsIDOMElement * *aFirstElementChild); \
  NS_SCRIPTABLE NS_IMETHOD GetLastElementChild(nsIDOMElement * *aLastElementChild); \
  NS_SCRIPTABLE NS_IMETHOD GetPreviousElementSibling(nsIDOMElement * *aPreviousElementSibling); \
  NS_SCRIPTABLE NS_IMETHOD GetNextElementSibling(nsIDOMElement * *aNextElementSibling); \
  NS_SCRIPTABLE NS_IMETHOD GetChildElementCount(PRUint32 *aChildElementCount); \
  NS_SCRIPTABLE NS_IMETHOD GetChildren(nsIDOMNodeList * *aChildren); \
  NS_SCRIPTABLE NS_IMETHOD GetClassList(nsIDOMDOMTokenList * *aClassList); \
  NS_SCRIPTABLE NS_IMETHOD MozMatchesSelector(const nsAString & selector, PRBool *_retval NS_OUTPARAM); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDOMNSELEMENT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetElementsByClassName(const nsAString & classes, nsIDOMNodeList **_retval NS_OUTPARAM) { return _to GetElementsByClassName(classes, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientRects(nsIDOMClientRectList **_retval NS_OUTPARAM) { return _to GetClientRects(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetBoundingClientRect(nsIDOMClientRect **_retval NS_OUTPARAM) { return _to GetBoundingClientRect(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollTop(PRInt32 *aScrollTop) { return _to GetScrollTop(aScrollTop); } \
  NS_SCRIPTABLE NS_IMETHOD SetScrollTop(PRInt32 aScrollTop) { return _to SetScrollTop(aScrollTop); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollLeft(PRInt32 *aScrollLeft) { return _to GetScrollLeft(aScrollLeft); } \
  NS_SCRIPTABLE NS_IMETHOD SetScrollLeft(PRInt32 aScrollLeft) { return _to SetScrollLeft(aScrollLeft); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollHeight(PRInt32 *aScrollHeight) { return _to GetScrollHeight(aScrollHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollWidth(PRInt32 *aScrollWidth) { return _to GetScrollWidth(aScrollWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientTop(PRInt32 *aClientTop) { return _to GetClientTop(aClientTop); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientLeft(PRInt32 *aClientLeft) { return _to GetClientLeft(aClientLeft); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientHeight(PRInt32 *aClientHeight) { return _to GetClientHeight(aClientHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientWidth(PRInt32 *aClientWidth) { return _to GetClientWidth(aClientWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetFirstElementChild(nsIDOMElement * *aFirstElementChild) { return _to GetFirstElementChild(aFirstElementChild); } \
  NS_SCRIPTABLE NS_IMETHOD GetLastElementChild(nsIDOMElement * *aLastElementChild) { return _to GetLastElementChild(aLastElementChild); } \
  NS_SCRIPTABLE NS_IMETHOD GetPreviousElementSibling(nsIDOMElement * *aPreviousElementSibling) { return _to GetPreviousElementSibling(aPreviousElementSibling); } \
  NS_SCRIPTABLE NS_IMETHOD GetNextElementSibling(nsIDOMElement * *aNextElementSibling) { return _to GetNextElementSibling(aNextElementSibling); } \
  NS_SCRIPTABLE NS_IMETHOD GetChildElementCount(PRUint32 *aChildElementCount) { return _to GetChildElementCount(aChildElementCount); } \
  NS_SCRIPTABLE NS_IMETHOD GetChildren(nsIDOMNodeList * *aChildren) { return _to GetChildren(aChildren); } \
  NS_SCRIPTABLE NS_IMETHOD GetClassList(nsIDOMDOMTokenList * *aClassList) { return _to GetClassList(aClassList); } \
  NS_SCRIPTABLE NS_IMETHOD MozMatchesSelector(const nsAString & selector, PRBool *_retval NS_OUTPARAM) { return _to MozMatchesSelector(selector, _retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDOMNSELEMENT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetElementsByClassName(const nsAString & classes, nsIDOMNodeList **_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetElementsByClassName(classes, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientRects(nsIDOMClientRectList **_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetClientRects(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetBoundingClientRect(nsIDOMClientRect **_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetBoundingClientRect(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollTop(PRInt32 *aScrollTop) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetScrollTop(aScrollTop); } \
  NS_SCRIPTABLE NS_IMETHOD SetScrollTop(PRInt32 aScrollTop) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetScrollTop(aScrollTop); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollLeft(PRInt32 *aScrollLeft) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetScrollLeft(aScrollLeft); } \
  NS_SCRIPTABLE NS_IMETHOD SetScrollLeft(PRInt32 aScrollLeft) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetScrollLeft(aScrollLeft); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollHeight(PRInt32 *aScrollHeight) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetScrollHeight(aScrollHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetScrollWidth(PRInt32 *aScrollWidth) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetScrollWidth(aScrollWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientTop(PRInt32 *aClientTop) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetClientTop(aClientTop); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientLeft(PRInt32 *aClientLeft) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetClientLeft(aClientLeft); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientHeight(PRInt32 *aClientHeight) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetClientHeight(aClientHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetClientWidth(PRInt32 *aClientWidth) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetClientWidth(aClientWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetFirstElementChild(nsIDOMElement * *aFirstElementChild) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetFirstElementChild(aFirstElementChild); } \
  NS_SCRIPTABLE NS_IMETHOD GetLastElementChild(nsIDOMElement * *aLastElementChild) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLastElementChild(aLastElementChild); } \
  NS_SCRIPTABLE NS_IMETHOD GetPreviousElementSibling(nsIDOMElement * *aPreviousElementSibling) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPreviousElementSibling(aPreviousElementSibling); } \
  NS_SCRIPTABLE NS_IMETHOD GetNextElementSibling(nsIDOMElement * *aNextElementSibling) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetNextElementSibling(aNextElementSibling); } \
  NS_SCRIPTABLE NS_IMETHOD GetChildElementCount(PRUint32 *aChildElementCount) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetChildElementCount(aChildElementCount); } \
  NS_SCRIPTABLE NS_IMETHOD GetChildren(nsIDOMNodeList * *aChildren) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetChildren(aChildren); } \
  NS_SCRIPTABLE NS_IMETHOD GetClassList(nsIDOMDOMTokenList * *aClassList) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetClassList(aClassList); } \
  NS_SCRIPTABLE NS_IMETHOD MozMatchesSelector(const nsAString & selector, PRBool *_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->MozMatchesSelector(selector, _retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDOMNSElement : public nsIDOMNSElement
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMNSELEMENT

  nsDOMNSElement();

private:
  ~nsDOMNSElement();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDOMNSElement, nsIDOMNSElement)

nsDOMNSElement::nsDOMNSElement()
{
  /* member initializers and constructor code */
}

nsDOMNSElement::~nsDOMNSElement()
{
  /* destructor code */
}

/* nsIDOMNodeList getElementsByClassName (in DOMString classes); */
NS_IMETHODIMP nsDOMNSElement::GetElementsByClassName(const nsAString & classes, nsIDOMNodeList **_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMClientRectList getClientRects (); */
NS_IMETHODIMP nsDOMNSElement::GetClientRects(nsIDOMClientRectList **_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMClientRect getBoundingClientRect (); */
NS_IMETHODIMP nsDOMNSElement::GetBoundingClientRect(nsIDOMClientRect **_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute long scrollTop; */
NS_IMETHODIMP nsDOMNSElement::GetScrollTop(PRInt32 *aScrollTop)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMNSElement::SetScrollTop(PRInt32 aScrollTop)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute long scrollLeft; */
NS_IMETHODIMP nsDOMNSElement::GetScrollLeft(PRInt32 *aScrollLeft)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMNSElement::SetScrollLeft(PRInt32 aScrollLeft)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long scrollHeight; */
NS_IMETHODIMP nsDOMNSElement::GetScrollHeight(PRInt32 *aScrollHeight)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long scrollWidth; */
NS_IMETHODIMP nsDOMNSElement::GetScrollWidth(PRInt32 *aScrollWidth)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long clientTop; */
NS_IMETHODIMP nsDOMNSElement::GetClientTop(PRInt32 *aClientTop)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long clientLeft; */
NS_IMETHODIMP nsDOMNSElement::GetClientLeft(PRInt32 *aClientLeft)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long clientHeight; */
NS_IMETHODIMP nsDOMNSElement::GetClientHeight(PRInt32 *aClientHeight)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long clientWidth; */
NS_IMETHODIMP nsDOMNSElement::GetClientWidth(PRInt32 *aClientWidth)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMElement firstElementChild; */
NS_IMETHODIMP nsDOMNSElement::GetFirstElementChild(nsIDOMElement * *aFirstElementChild)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMElement lastElementChild; */
NS_IMETHODIMP nsDOMNSElement::GetLastElementChild(nsIDOMElement * *aLastElementChild)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMElement previousElementSibling; */
NS_IMETHODIMP nsDOMNSElement::GetPreviousElementSibling(nsIDOMElement * *aPreviousElementSibling)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMElement nextElementSibling; */
NS_IMETHODIMP nsDOMNSElement::GetNextElementSibling(nsIDOMElement * *aNextElementSibling)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute unsigned long childElementCount; */
NS_IMETHODIMP nsDOMNSElement::GetChildElementCount(PRUint32 *aChildElementCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMNodeList children; */
NS_IMETHODIMP nsDOMNSElement::GetChildren(nsIDOMNodeList * *aChildren)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMDOMTokenList classList; */
NS_IMETHODIMP nsDOMNSElement::GetClassList(nsIDOMDOMTokenList * *aClassList)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean mozMatchesSelector (in DOMString selector); */
NS_IMETHODIMP nsDOMNSElement::MozMatchesSelector(const nsAString & selector, PRBool *_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDOMNSElement_h__ */
