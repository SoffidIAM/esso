/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM caibIAfroditaF.idl
 */

#ifndef __gen_caibIAfroditaF_h__
#define __gen_caibIAfroditaF_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIDOMDocument; /* forward declaration */


/* starting interface:    caibIAfroditaF */
#define CAIBIAFRODITAF_IID_STR "d0ac7576-134d-4064-b315-d75eb14d7427"

#define CAIBIAFRODITAF_IID \
  {0xd0ac7576, 0x134d, 0x4064, \
    { 0xb3, 0x15, 0xd7, 0x5e, 0xb1, 0x4d, 0x74, 0x27 }}

class NS_NO_VTABLE caibIAfroditaF : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(CAIBIMAZINGERFF_IID)

  /* void notify (in nsIDOMDocument doc); */
  NS_IMETHOD Notify(nsIDOMDocument *doc) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(caibIMazingerFF, CAIBIMAZINGERFF_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_CAIBIMAZINGERFF \
  NS_IMETHOD Notify(nsIDOMDocument *doc);

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_CAIBIMAZINGERFF(_to) \
  NS_IMETHOD Notify(nsIDOMDocument *doc) { return _to Notify(doc); }

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_CAIBIMAZINGERFF(_to) \
  NS_IMETHOD Notify(nsIDOMDocument *doc) { return !_to ? NS_ERROR_NULL_POINTER : _to->Notify(doc); }

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class _MYCLASS_ : public caibIMazingerFF
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_CAIBIMAZINGERFF

  _MYCLASS_();

private:
  ~_MYCLASS_();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(_MYCLASS_, caibIMazingerFF)

_MYCLASS_::_MYCLASS_()
{
  /* member initializers and constructor code */
}

_MYCLASS_::~_MYCLASS_()
{
  /* destructor code */
}

/* void notify (in nsIDOMDocument doc); */
NS_IMETHODIMP _MYCLASS_::Notify(nsIDOMDocument *doc)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_caibIMazingerFF_h__ */
