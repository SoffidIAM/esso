/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM mozIStorageBindingParams.idl
 */

#ifndef __gen_mozIStorageBindingParams_h__
#define __gen_mozIStorageBindingParams_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIVariant; /* forward declaration */


/* starting interface:    mozIStorageBindingParams */
#define MOZISTORAGEBINDINGPARAMS_IID_STR "a8d4827c-641c-45e3-a9ea-493570b4106b"

#define MOZISTORAGEBINDINGPARAMS_IID \
  {0xa8d4827c, 0x641c, 0x45e3, \
    { 0xa9, 0xea, 0x49, 0x35, 0x70, 0xb4, 0x10, 0x6b }}

class NS_NO_VTABLE NS_SCRIPTABLE mozIStorageBindingParams : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(MOZISTORAGEBINDINGPARAMS_IID)

  /**
   * Binds aValue to the parameter with the name aName.
   *
   * @param aName
   *        The name of the parameter to bind aValue to.
   * @param aValue
   *        The value to bind.
   */
  /* void bindByName (in AUTF8String aName, in nsIVariant aValue); */
  NS_SCRIPTABLE NS_IMETHOD BindByName(const nsACString & aName, nsIVariant *aValue) = 0;

  /* [noscript] void bindUTF8StringByName (in AUTF8String aName, in AUTF8String aValue); */
  NS_IMETHOD BindUTF8StringByName(const nsACString & aName, const nsACString & aValue) = 0;

  /* [noscript] void bindStringByName (in AUTF8String aName, in AString aValue); */
  NS_IMETHOD BindStringByName(const nsACString & aName, const nsAString & aValue) = 0;

  /* [noscript] void bindDoubleByName (in AUTF8String aName, in double aValue); */
  NS_IMETHOD BindDoubleByName(const nsACString & aName, double aValue) = 0;

  /* [noscript] void bindInt32ByName (in AUTF8String aName, in long aValue); */
  NS_IMETHOD BindInt32ByName(const nsACString & aName, PRInt32 aValue) = 0;

  /* [noscript] void bindInt64ByName (in AUTF8String aName, in long long aValue); */
  NS_IMETHOD BindInt64ByName(const nsACString & aName, PRInt64 aValue) = 0;

  /* [noscript] void bindNullByName (in AUTF8String aName); */
  NS_IMETHOD BindNullByName(const nsACString & aName) = 0;

  /* void bindBlobByName (in AUTF8String aName, [array, size_is (aValueSize), const] in octet aValue, in unsigned long aValueSize); */
  NS_SCRIPTABLE NS_IMETHOD BindBlobByName(const nsACString & aName, const PRUint8 *aValue, PRUint32 aValueSize) = 0;

  /**
    * Binds aValue to the parameter with the index aIndex.
    *
    * @param aIndex
    *        The zero-based index of the parameter to bind aValue to.
    * @param aValue
    *        The value to bind.
    */
  /* void bindByIndex (in unsigned long aIndex, in nsIVariant aValue); */
  NS_SCRIPTABLE NS_IMETHOD BindByIndex(PRUint32 aIndex, nsIVariant *aValue) = 0;

  /* [noscript] void bindUTF8StringByIndex (in unsigned long aIndex, in AUTF8String aValue); */
  NS_IMETHOD BindUTF8StringByIndex(PRUint32 aIndex, const nsACString & aValue) = 0;

  /* [noscript] void bindStringByIndex (in unsigned long aIndex, in AString aValue); */
  NS_IMETHOD BindStringByIndex(PRUint32 aIndex, const nsAString & aValue) = 0;

  /* [noscript] void bindDoubleByIndex (in unsigned long aIndex, in double aValue); */
  NS_IMETHOD BindDoubleByIndex(PRUint32 aIndex, double aValue) = 0;

  /* [noscript] void bindInt32ByIndex (in unsigned long aIndex, in long aValue); */
  NS_IMETHOD BindInt32ByIndex(PRUint32 aIndex, PRInt32 aValue) = 0;

  /* [noscript] void bindInt64ByIndex (in unsigned long aIndex, in long long aValue); */
  NS_IMETHOD BindInt64ByIndex(PRUint32 aIndex, PRInt64 aValue) = 0;

  /* [noscript] void bindNullByIndex (in unsigned long aIndex); */
  NS_IMETHOD BindNullByIndex(PRUint32 aIndex) = 0;

  /* void bindBlobByIndex (in unsigned long aIndex, [array, size_is (aValueSize), const] in octet aValue, in unsigned long aValueSize); */
  NS_SCRIPTABLE NS_IMETHOD BindBlobByIndex(PRUint32 aIndex, const PRUint8 *aValue, PRUint32 aValueSize) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(mozIStorageBindingParams, MOZISTORAGEBINDINGPARAMS_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_MOZISTORAGEBINDINGPARAMS \
  NS_SCRIPTABLE NS_IMETHOD BindByName(const nsACString & aName, nsIVariant *aValue); \
  NS_IMETHOD BindUTF8StringByName(const nsACString & aName, const nsACString & aValue); \
  NS_IMETHOD BindStringByName(const nsACString & aName, const nsAString & aValue); \
  NS_IMETHOD BindDoubleByName(const nsACString & aName, double aValue); \
  NS_IMETHOD BindInt32ByName(const nsACString & aName, PRInt32 aValue); \
  NS_IMETHOD BindInt64ByName(const nsACString & aName, PRInt64 aValue); \
  NS_IMETHOD BindNullByName(const nsACString & aName); \
  NS_SCRIPTABLE NS_IMETHOD BindBlobByName(const nsACString & aName, const PRUint8 *aValue, PRUint32 aValueSize); \
  NS_SCRIPTABLE NS_IMETHOD BindByIndex(PRUint32 aIndex, nsIVariant *aValue); \
  NS_IMETHOD BindUTF8StringByIndex(PRUint32 aIndex, const nsACString & aValue); \
  NS_IMETHOD BindStringByIndex(PRUint32 aIndex, const nsAString & aValue); \
  NS_IMETHOD BindDoubleByIndex(PRUint32 aIndex, double aValue); \
  NS_IMETHOD BindInt32ByIndex(PRUint32 aIndex, PRInt32 aValue); \
  NS_IMETHOD BindInt64ByIndex(PRUint32 aIndex, PRInt64 aValue); \
  NS_IMETHOD BindNullByIndex(PRUint32 aIndex); \
  NS_SCRIPTABLE NS_IMETHOD BindBlobByIndex(PRUint32 aIndex, const PRUint8 *aValue, PRUint32 aValueSize); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_MOZISTORAGEBINDINGPARAMS(_to) \
  NS_SCRIPTABLE NS_IMETHOD BindByName(const nsACString & aName, nsIVariant *aValue) { return _to BindByName(aName, aValue); } \
  NS_IMETHOD BindUTF8StringByName(const nsACString & aName, const nsACString & aValue) { return _to BindUTF8StringByName(aName, aValue); } \
  NS_IMETHOD BindStringByName(const nsACString & aName, const nsAString & aValue) { return _to BindStringByName(aName, aValue); } \
  NS_IMETHOD BindDoubleByName(const nsACString & aName, double aValue) { return _to BindDoubleByName(aName, aValue); } \
  NS_IMETHOD BindInt32ByName(const nsACString & aName, PRInt32 aValue) { return _to BindInt32ByName(aName, aValue); } \
  NS_IMETHOD BindInt64ByName(const nsACString & aName, PRInt64 aValue) { return _to BindInt64ByName(aName, aValue); } \
  NS_IMETHOD BindNullByName(const nsACString & aName) { return _to BindNullByName(aName); } \
  NS_SCRIPTABLE NS_IMETHOD BindBlobByName(const nsACString & aName, const PRUint8 *aValue, PRUint32 aValueSize) { return _to BindBlobByName(aName, aValue, aValueSize); } \
  NS_SCRIPTABLE NS_IMETHOD BindByIndex(PRUint32 aIndex, nsIVariant *aValue) { return _to BindByIndex(aIndex, aValue); } \
  NS_IMETHOD BindUTF8StringByIndex(PRUint32 aIndex, const nsACString & aValue) { return _to BindUTF8StringByIndex(aIndex, aValue); } \
  NS_IMETHOD BindStringByIndex(PRUint32 aIndex, const nsAString & aValue) { return _to BindStringByIndex(aIndex, aValue); } \
  NS_IMETHOD BindDoubleByIndex(PRUint32 aIndex, double aValue) { return _to BindDoubleByIndex(aIndex, aValue); } \
  NS_IMETHOD BindInt32ByIndex(PRUint32 aIndex, PRInt32 aValue) { return _to BindInt32ByIndex(aIndex, aValue); } \
  NS_IMETHOD BindInt64ByIndex(PRUint32 aIndex, PRInt64 aValue) { return _to BindInt64ByIndex(aIndex, aValue); } \
  NS_IMETHOD BindNullByIndex(PRUint32 aIndex) { return _to BindNullByIndex(aIndex); } \
  NS_SCRIPTABLE NS_IMETHOD BindBlobByIndex(PRUint32 aIndex, const PRUint8 *aValue, PRUint32 aValueSize) { return _to BindBlobByIndex(aIndex, aValue, aValueSize); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_MOZISTORAGEBINDINGPARAMS(_to) \
  NS_SCRIPTABLE NS_IMETHOD BindByName(const nsACString & aName, nsIVariant *aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindByName(aName, aValue); } \
  NS_IMETHOD BindUTF8StringByName(const nsACString & aName, const nsACString & aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindUTF8StringByName(aName, aValue); } \
  NS_IMETHOD BindStringByName(const nsACString & aName, const nsAString & aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindStringByName(aName, aValue); } \
  NS_IMETHOD BindDoubleByName(const nsACString & aName, double aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindDoubleByName(aName, aValue); } \
  NS_IMETHOD BindInt32ByName(const nsACString & aName, PRInt32 aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindInt32ByName(aName, aValue); } \
  NS_IMETHOD BindInt64ByName(const nsACString & aName, PRInt64 aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindInt64ByName(aName, aValue); } \
  NS_IMETHOD BindNullByName(const nsACString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindNullByName(aName); } \
  NS_SCRIPTABLE NS_IMETHOD BindBlobByName(const nsACString & aName, const PRUint8 *aValue, PRUint32 aValueSize) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindBlobByName(aName, aValue, aValueSize); } \
  NS_SCRIPTABLE NS_IMETHOD BindByIndex(PRUint32 aIndex, nsIVariant *aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindByIndex(aIndex, aValue); } \
  NS_IMETHOD BindUTF8StringByIndex(PRUint32 aIndex, const nsACString & aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindUTF8StringByIndex(aIndex, aValue); } \
  NS_IMETHOD BindStringByIndex(PRUint32 aIndex, const nsAString & aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindStringByIndex(aIndex, aValue); } \
  NS_IMETHOD BindDoubleByIndex(PRUint32 aIndex, double aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindDoubleByIndex(aIndex, aValue); } \
  NS_IMETHOD BindInt32ByIndex(PRUint32 aIndex, PRInt32 aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindInt32ByIndex(aIndex, aValue); } \
  NS_IMETHOD BindInt64ByIndex(PRUint32 aIndex, PRInt64 aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindInt64ByIndex(aIndex, aValue); } \
  NS_IMETHOD BindNullByIndex(PRUint32 aIndex) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindNullByIndex(aIndex); } \
  NS_SCRIPTABLE NS_IMETHOD BindBlobByIndex(PRUint32 aIndex, const PRUint8 *aValue, PRUint32 aValueSize) { return !_to ? NS_ERROR_NULL_POINTER : _to->BindBlobByIndex(aIndex, aValue, aValueSize); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class _MYCLASS_ : public mozIStorageBindingParams
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEBINDINGPARAMS

  _MYCLASS_();

private:
  ~_MYCLASS_();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(_MYCLASS_, mozIStorageBindingParams)

_MYCLASS_::_MYCLASS_()
{
  /* member initializers and constructor code */
}

_MYCLASS_::~_MYCLASS_()
{
  /* destructor code */
}

/* void bindByName (in AUTF8String aName, in nsIVariant aValue); */
NS_IMETHODIMP _MYCLASS_::BindByName(const nsACString & aName, nsIVariant *aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void bindUTF8StringByName (in AUTF8String aName, in AUTF8String aValue); */
NS_IMETHODIMP _MYCLASS_::BindUTF8StringByName(const nsACString & aName, const nsACString & aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void bindStringByName (in AUTF8String aName, in AString aValue); */
NS_IMETHODIMP _MYCLASS_::BindStringByName(const nsACString & aName, const nsAString & aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void bindDoubleByName (in AUTF8String aName, in double aValue); */
NS_IMETHODIMP _MYCLASS_::BindDoubleByName(const nsACString & aName, double aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void bindInt32ByName (in AUTF8String aName, in long aValue); */
NS_IMETHODIMP _MYCLASS_::BindInt32ByName(const nsACString & aName, PRInt32 aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void bindInt64ByName (in AUTF8String aName, in long long aValue); */
NS_IMETHODIMP _MYCLASS_::BindInt64ByName(const nsACString & aName, PRInt64 aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void bindNullByName (in AUTF8String aName); */
NS_IMETHODIMP _MYCLASS_::BindNullByName(const nsACString & aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void bindBlobByName (in AUTF8String aName, [array, size_is (aValueSize), const] in octet aValue, in unsigned long aValueSize); */
NS_IMETHODIMP _MYCLASS_::BindBlobByName(const nsACString & aName, const PRUint8 *aValue, PRUint32 aValueSize)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void bindByIndex (in unsigned long aIndex, in nsIVariant aValue); */
NS_IMETHODIMP _MYCLASS_::BindByIndex(PRUint32 aIndex, nsIVariant *aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void bindUTF8StringByIndex (in unsigned long aIndex, in AUTF8String aValue); */
NS_IMETHODIMP _MYCLASS_::BindUTF8StringByIndex(PRUint32 aIndex, const nsACString & aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void bindStringByIndex (in unsigned long aIndex, in AString aValue); */
NS_IMETHODIMP _MYCLASS_::BindStringByIndex(PRUint32 aIndex, const nsAString & aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void bindDoubleByIndex (in unsigned long aIndex, in double aValue); */
NS_IMETHODIMP _MYCLASS_::BindDoubleByIndex(PRUint32 aIndex, double aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void bindInt32ByIndex (in unsigned long aIndex, in long aValue); */
NS_IMETHODIMP _MYCLASS_::BindInt32ByIndex(PRUint32 aIndex, PRInt32 aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void bindInt64ByIndex (in unsigned long aIndex, in long long aValue); */
NS_IMETHODIMP _MYCLASS_::BindInt64ByIndex(PRUint32 aIndex, PRInt64 aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void bindNullByIndex (in unsigned long aIndex); */
NS_IMETHODIMP _MYCLASS_::BindNullByIndex(PRUint32 aIndex)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void bindBlobByIndex (in unsigned long aIndex, [array, size_is (aValueSize), const] in octet aValue, in unsigned long aValueSize); */
NS_IMETHODIMP _MYCLASS_::BindBlobByIndex(PRUint32 aIndex, const PRUint8 *aValue, PRUint32 aValueSize)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_mozIStorageBindingParams_h__ */
