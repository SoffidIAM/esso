/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM c:/dev/mozilla/mozilla/dom/public/idl/base/nsIDOMWindowUtils.idl
 */

#ifndef __gen_nsIDOMWindowUtils_h__
#define __gen_nsIDOMWindowUtils_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIDOMWindowUtils */
#define NS_IDOMWINDOWUTILS_IID_STR "76cdfff8-6f13-43e8-9ed4-36066cd83742"

#define NS_IDOMWINDOWUTILS_IID \
  {0x76cdfff8, 0x6f13, 0x43e8, \
    { 0x9e, 0xd4, 0x36, 0x06, 0x6c, 0xd8, 0x37, 0x42 }}

/**
 * nsIDOMWindowUtils is intended for infrequently-used methods related
 * to the current nsIDOMWindow.  Some of the methods may require
 * elevated privileges; the method implementations should contain the
 * necessary security checks.  Access this interface by calling
 * getInterface on a DOMWindow.
 */
class NS_NO_VTABLE nsIDOMWindowUtils : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMWINDOWUTILS_IID)

  /**
   * Image animation mode of the window. When this attribute's value
   * is changed, the implementation should set all images in the window
   * to the given value. That is, when set to kDontAnimMode, all images
   * will stop animating. The attribute's value must be one of the
   * animationMode values from imgIContainer.
   * @note Images may individually override the window's setting after
   *       the window's mode is set. Therefore images given different modes
   *       since the last setting of the window's mode may behave
   *       out of line with the window's overall mode.
   * @note The attribute's value is the window's overall mode. It may
   *       for example continue to report kDontAnimMode after all images
   *       have subsequently been individually animated.
   * @note Only images immediately in this window are affected;
   *       this is not recursive to subwindows.
   * @see imgIContainer
   */
  /* attribute unsigned short imageAnimationMode; */
  NS_IMETHOD GetImageAnimationMode(PRUint16 *aImageAnimationMode) = 0;
  NS_IMETHOD SetImageAnimationMode(PRUint16 aImageAnimationMode) = 0;

  /**
   * Function to get metadata associated with the window's current document
   * @param aName the name of the metadata.  This should be all lowercase.
   * @return the value of the metadata, or the empty string if it's not set
   *
   * Will throw a DOM security error if called without UniversalXPConnect
   * privileges.
   */
  /* AString getDocumentMetadata (in AString aName); */
  NS_IMETHOD GetDocumentMetadata(const nsAString & aName, nsAString & _retval) = 0;

  /**
   * Force an immediate redraw of this window.
   */
  /* void redraw (); */
  NS_IMETHOD Redraw(void) = 0;

  /** Synthesize a mouse event for a window. The event types supported
   *  are: 
   *    mousedown, mouseup, mousemove, mouseover, mouseout, contextmenu
   *
   * Events are sent in coordinates offset by aX and aY from the window.
   *
   * Note that additional events may be fired as a result of this call. For
   * instance, typically a click event will be fired as a result of a
   * mousedown and mouseup in sequence.
   *
   * Normally at this level of events, the mouseover and mouseout events are
   * only fired when the window is entered or exited. For inter-element
   * mouseover and mouseout events, a movemove event fired on the new element
   * should be sufficient to generate the correct over and out events as well.
   *
   * @param aType event type
   * @param aX x offset
   * @param aY y offset
   * @param aButton button to synthesize
   * @param aClickCount number of clicks that have been performed
   * @param aModifiers modifiers pressed, using constants defined in nsIDOMNSEvent
   */
  /* void sendMouseEvent (in AString aType, in long aX, in long aY, in long aButton, in long aClickCount, in long aModifiers); */
  NS_IMETHOD SendMouseEvent(const nsAString & aType, PRInt32 aX, PRInt32 aY, PRInt32 aButton, PRInt32 aClickCount, PRInt32 aModifiers) = 0;

  /**
   * Synthesize a key event to the window. The event types supported are:
   *   keydown, keyup, keypress
   *
   * Key events generally end up being sent to the focused node.
   *
   * @param aType event type
   * @param aKeyCode key code
   * @param aCharCode character code
   * @param aModifiers modifiers pressed, using constants defined in nsIDOMNSEvent
   */
  /* void sendKeyEvent (in AString aType, in long aKeyCode, in long aCharCode, in long aModifiers); */
  NS_IMETHOD SendKeyEvent(const nsAString & aType, PRInt32 aKeyCode, PRInt32 aCharCode, PRInt32 aModifiers) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMWindowUtils, NS_IDOMWINDOWUTILS_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDOMWINDOWUTILS \
  NS_IMETHOD GetImageAnimationMode(PRUint16 *aImageAnimationMode); \
  NS_IMETHOD SetImageAnimationMode(PRUint16 aImageAnimationMode); \
  NS_IMETHOD GetDocumentMetadata(const nsAString & aName, nsAString & _retval); \
  NS_IMETHOD Redraw(void); \
  NS_IMETHOD SendMouseEvent(const nsAString & aType, PRInt32 aX, PRInt32 aY, PRInt32 aButton, PRInt32 aClickCount, PRInt32 aModifiers); \
  NS_IMETHOD SendKeyEvent(const nsAString & aType, PRInt32 aKeyCode, PRInt32 aCharCode, PRInt32 aModifiers); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDOMWINDOWUTILS(_to) \
  NS_IMETHOD GetImageAnimationMode(PRUint16 *aImageAnimationMode) { return _to GetImageAnimationMode(aImageAnimationMode); } \
  NS_IMETHOD SetImageAnimationMode(PRUint16 aImageAnimationMode) { return _to SetImageAnimationMode(aImageAnimationMode); } \
  NS_IMETHOD GetDocumentMetadata(const nsAString & aName, nsAString & _retval) { return _to GetDocumentMetadata(aName, _retval); } \
  NS_IMETHOD Redraw(void) { return _to Redraw(); } \
  NS_IMETHOD SendMouseEvent(const nsAString & aType, PRInt32 aX, PRInt32 aY, PRInt32 aButton, PRInt32 aClickCount, PRInt32 aModifiers) { return _to SendMouseEvent(aType, aX, aY, aButton, aClickCount, aModifiers); } \
  NS_IMETHOD SendKeyEvent(const nsAString & aType, PRInt32 aKeyCode, PRInt32 aCharCode, PRInt32 aModifiers) { return _to SendKeyEvent(aType, aKeyCode, aCharCode, aModifiers); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDOMWINDOWUTILS(_to) \
  NS_IMETHOD GetImageAnimationMode(PRUint16 *aImageAnimationMode) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetImageAnimationMode(aImageAnimationMode); } \
  NS_IMETHOD SetImageAnimationMode(PRUint16 aImageAnimationMode) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetImageAnimationMode(aImageAnimationMode); } \
  NS_IMETHOD GetDocumentMetadata(const nsAString & aName, nsAString & _retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDocumentMetadata(aName, _retval); } \
  NS_IMETHOD Redraw(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Redraw(); } \
  NS_IMETHOD SendMouseEvent(const nsAString & aType, PRInt32 aX, PRInt32 aY, PRInt32 aButton, PRInt32 aClickCount, PRInt32 aModifiers) { return !_to ? NS_ERROR_NULL_POINTER : _to->SendMouseEvent(aType, aX, aY, aButton, aClickCount, aModifiers); } \
  NS_IMETHOD SendKeyEvent(const nsAString & aType, PRInt32 aKeyCode, PRInt32 aCharCode, PRInt32 aModifiers) { return !_to ? NS_ERROR_NULL_POINTER : _to->SendKeyEvent(aType, aKeyCode, aCharCode, aModifiers); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDOMWindowUtils : public nsIDOMWindowUtils
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMWINDOWUTILS

  nsDOMWindowUtils();

private:
  ~nsDOMWindowUtils();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDOMWindowUtils, nsIDOMWindowUtils)

nsDOMWindowUtils::nsDOMWindowUtils()
{
  /* member initializers and constructor code */
}

nsDOMWindowUtils::~nsDOMWindowUtils()
{
  /* destructor code */
}

/* attribute unsigned short imageAnimationMode; */
NS_IMETHODIMP nsDOMWindowUtils::GetImageAnimationMode(PRUint16 *aImageAnimationMode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMWindowUtils::SetImageAnimationMode(PRUint16 aImageAnimationMode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* AString getDocumentMetadata (in AString aName); */
NS_IMETHODIMP nsDOMWindowUtils::GetDocumentMetadata(const nsAString & aName, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void redraw (); */
NS_IMETHODIMP nsDOMWindowUtils::Redraw()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void sendMouseEvent (in AString aType, in long aX, in long aY, in long aButton, in long aClickCount, in long aModifiers); */
NS_IMETHODIMP nsDOMWindowUtils::SendMouseEvent(const nsAString & aType, PRInt32 aX, PRInt32 aY, PRInt32 aButton, PRInt32 aClickCount, PRInt32 aModifiers)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void sendKeyEvent (in AString aType, in long aKeyCode, in long aCharCode, in long aModifiers); */
NS_IMETHODIMP nsDOMWindowUtils::SendKeyEvent(const nsAString & aType, PRInt32 aKeyCode, PRInt32 aCharCode, PRInt32 aModifiers)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDOMWindowUtils_h__ */
