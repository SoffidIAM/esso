/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM nsIPluginHost.idl
 */

#ifndef __gen_nsIPluginHost_h__
#define __gen_nsIPluginHost_h__


#ifndef __gen_nspluginroot_h__
#include "nspluginroot.h"
#endif

#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

#ifndef __gen_nsIPluginInstanceOwner_h__
#include "nsIPluginInstanceOwner.h"
#endif

#ifndef __gen_nsIStreamListener_h__
#include "nsIStreamListener.h"
#endif

#ifndef __gen_nsIStringStream_h__
#include "nsIStringStream.h"
#endif

#ifndef __gen_nsIPluginTag_h__
#include "nsIPluginTag.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
#include "nsPluginNativeWindow.h"
#include "nsplugindefs.h"
#ifdef MOZILLA_INTERNAL_API
#include "nsString.h"
#include "nsNetUtil.h"
#endif
#include "prlink.h"  // for PRLibrary
#define MOZ_PLUGIN_HOST_CONTRACTID \
  "@mozilla.org/plugin/host;1"
class nsIPlugin; /* forward declaration */

class nsIURI; /* forward declaration */

class nsIDOMPlugin; /* forward declaration */

class nsIChannel; /* forward declaration */

class nsIPluginStreamListener; /* forward declaration */


/* starting interface:    nsIPluginHost */
#define NS_IPLUGINHOST_IID_STR "30c7c529-b05c-4950-b5b8-9af673e46521"

#define NS_IPLUGINHOST_IID \
  {0x30c7c529, 0xb05c, 0x4950, \
    { 0xb5, 0xb8, 0x9a, 0xf6, 0x73, 0xe4, 0x65, 0x21 }}

class NS_SCRIPTABLE nsIPluginHost : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPLUGINHOST_IID)

  /* [noscript] void init (); */
  NS_IMETHOD Init(void) = 0;

  /* [noscript] void destroy (); */
  NS_IMETHOD Destroy(void) = 0;

  /* [noscript] void loadPlugins (); */
  NS_IMETHOD LoadPlugins(void) = 0;

  /**
   * Causes the plugins directory to be searched again for new plugin 
   * libraries.
   *
   * @param reloadPages - indicates whether currently visible pages should 
   * also be reloaded
   */
  /* void reloadPlugins (in boolean reloadPages); */
  NS_SCRIPTABLE NS_IMETHOD ReloadPlugins(PRBool reloadPages) = 0;

  /* [noscript] nsIPlugin getPlugin (in string aMimeType); */
  NS_IMETHOD GetPlugin(const char *aMimeType, nsIPlugin **_retval NS_OUTPARAM) = 0;

  /* [noscript] void instantiateEmbeddedPlugin (in string aMimeType, in nsIURI aURL, in nsIPluginInstanceOwner aOwner); */
  NS_IMETHOD InstantiateEmbeddedPlugin(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner) = 0;

  /* [noscript] void instantiateFullPagePlugin (in string aMimeType, in nsIURI aURI, in nsIStreamListenerRef aStreamListener, in nsIPluginInstanceOwner aOwner); */
  NS_IMETHOD InstantiateFullPagePlugin(const char *aMimeType, nsIURI *aURI, nsIStreamListener * & aStreamListener, nsIPluginInstanceOwner *aOwner) = 0;

  /**
   * Instantiate an embedded plugin for an existing channel. The caller is
   * responsible for opening the channel. It may or may not be already opened
   * when this function is called.
   */
  /* [noscript] nsIStreamListener instantiatePluginForChannel (in nsIChannel aChannel, in nsIPluginInstanceOwner aOwner); */
  NS_IMETHOD InstantiatePluginForChannel(nsIChannel *aChannel, nsIPluginInstanceOwner *aOwner, nsIStreamListener **_retval NS_OUTPARAM) = 0;

  /* [noscript] void setUpPluginInstance (in string aMimeType, in nsIURI aURL, in nsIPluginInstanceOwner aOwner); */
  NS_IMETHOD SetUpPluginInstance(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner) = 0;

  /* [noscript] void isPluginEnabledForType (in string aMimeType); */
  NS_IMETHOD IsPluginEnabledForType(const char *aMimeType) = 0;

  /* [noscript] void isPluginEnabledForExtension (in string aExtension, in constCharStarRef aMimeType); */
  NS_IMETHOD IsPluginEnabledForExtension(const char *aExtension, const char * & aMimeType) = 0;

  /* [noscript] readonly attribute unsigned long pluginCount; */
  NS_IMETHOD GetPluginCount(PRUint32 *aPluginCount) = 0;

  /* [noscript] void getPlugins (in unsigned long aPluginCount, out nsIDOMPlugin aPluginArray); */
  NS_IMETHOD GetPlugins(PRUint32 aPluginCount, nsIDOMPlugin **aPluginArray NS_OUTPARAM) = 0;

  /* void getPluginTags (out unsigned long aPluginCount, [array, retval, size_is (aPluginCount)] out nsIPluginTag aResults); */
  NS_SCRIPTABLE NS_IMETHOD GetPluginTags(PRUint32 *aPluginCount NS_OUTPARAM, nsIPluginTag ***aResults NS_OUTPARAM) = 0;

  /* [noscript] void stopPluginInstance (in nsIPluginInstance aInstance); */
  NS_IMETHOD StopPluginInstance(nsIPluginInstance *aInstance) = 0;

  /* [noscript] void handleBadPlugin (in PRLibraryPtr aLibrary, in nsIPluginInstance instance); */
  NS_IMETHOD HandleBadPlugin(PRLibrary * aLibrary, nsIPluginInstance *instance) = 0;

  NS_IMETHOD
  GetURL(nsISupports* pluginInst,
         const char* url,
         const char* target = NULL,
         nsIPluginStreamListener* streamListener = NULL,
         const char* altHost = NULL,
         const char* referrer = NULL,
         PRBool forceJSEnabled = PR_FALSE) = 0;
  NS_IMETHOD
  PostURL(nsISupports* pluginInst,
          const char* url,
          PRUint32 postDataLen,
          const char* postData,
          PRBool isFile = PR_FALSE,
          const char* target = NULL,
          nsIPluginStreamListener* streamListener = NULL,
          const char* altHost = NULL,
          const char* referrer = NULL,
          PRBool forceJSEnabled = PR_FALSE,
          PRUint32 postHeadersLength = 0,
          const char* postHeaders = NULL) = 0;
  /**
   * Fetches a URL.
   *
   * (Corresponds to NPN_GetURL and NPN_GetURLNotify.)
   *
   * @param pluginInst - the plugin making the request. If NULL, the URL
   *  is fetched in the background.
   * @param url - the URL to fetch
   * @param target - the target window into which to load the URL, or NULL if
   *  the data should be returned to the plugin via streamListener.
   * @param streamListener - a stream listener to be used to return data to
   *  the plugin. May be NULL if target is not NULL.
   * @param altHost - an IP-address string that will be used instead of the 
   *  host specified in the URL. This is used to prevent DNS-spoofing 
   *  attacks. Can be defaulted to NULL meaning use the host in the URL.
   * @param referrer - the referring URL (may be NULL)
   * @param forceJSEnabled - forces JavaScript to be enabled for 'javascript:'
   *  URLs, even if the user currently has JavaScript disabled (usually 
   *  specify PR_FALSE) 
   * @result - NS_OK if this operation was successful
   */
/**
   * Posts to a URL with post data and/or post headers.
   *
   * (Corresponds to NPN_PostURL and NPN_PostURLNotify.)
   *
   * @param pluginInst - the plugin making the request. If NULL, the URL
   *  is fetched in the background.
   * @param url - the URL to fetch
   * @param postDataLength - the length of postData (if non-NULL)
   * @param postData - the data to POST. NULL specifies that there is not post
   *  data
   * @param isFile - whether the postData specifies the name of a file to 
   *  post instead of data. The file will be deleted afterwards.
   * @param target - the target window into which to load the URL, or NULL if
   *  the data should be returned to the plugin via streamListener.
   * @param streamListener - a stream listener to be used to return data to
   *  the plugin. May be NULL if target is not NULL.
   * @param altHost - an IP-address string that will be used instead of the 
   *  host specified in the URL. This is used to prevent DNS-spoofing 
   *  attacks. Can be defaulted to NULL meaning use the host in the URL.
   * @param referrer - the referring URL (may be NULL)
   * @param forceJSEnabled - forces JavaScript to be enabled for 'javascript:'
   *  URLs, even if the user currently has JavaScript disabled (usually 
   *  specify PR_FALSE) 
   * @param postHeadersLength - the length of postHeaders (if non-NULL)
   * @param postHeaders - the headers to POST. Must be in the form of
   * "HeaderName: HeaderValue\r\n".  Each header, including the last,
   * must be followed by "\r\n".  NULL specifies that there are no
   * post headers
   * @result - NS_OK if this operation was successful
   */
/**
   * Returns the proxy info for a given URL. The caller is required to
   * free the resulting memory with nsIMalloc::Free. The result will be in the
   * following format
   * 
   *   i)   "DIRECT"  -- no proxy
   *   ii)  "PROXY xxx.xxx.xxx.xxx"   -- use proxy
   *   iii) "SOCKS xxx.xxx.xxx.xxx"  -- use SOCKS
   *   iv)  Mixed. e.g. "PROXY 111.111.111.111;PROXY 112.112.112.112",
   *                    "PROXY 111.111.111.111;SOCKS 112.112.112.112"....
   *
   * Which proxy/SOCKS to use is determined by the plugin.
   */
  /* void findProxyForURL (in string aURL, out string aResult); */
  NS_SCRIPTABLE NS_IMETHOD FindProxyForURL(const char *aURL, char **aResult NS_OUTPARAM) = 0;

  /* [noscript] void UserAgent (in nativeChar resultingAgentString); */
  NS_IMETHOD UserAgent(const char * * resultingAgentString) = 0;

  /**
   * To notify the plugin manager that the plugin created a script object 
   */
  /* [noscript] void setIsScriptableInstance (in nsIPluginInstance aInstance, in boolean aScriptable); */
  NS_IMETHOD SetIsScriptableInstance(nsIPluginInstance *aInstance, PRBool aScriptable) = 0;

  /**
   * This method parses post buffer to find out case insensitive "Content-length" string
   * and CR or LF some where after that, then it assumes there is http headers in
   * the input buffer and continue to search for end of headers (CRLFCRLF or LFLF).
   * It will *always malloc()* output buffer (caller is responsible to free it) 
   * if input buffer starts with LF, which comes from 4.x spec 
   * http://developer.netscape.com/docs/manuals/communicator/plugin/pgfn2.htm#1007754
   * "If no custom headers are required, simply add a blank
   * line ('\n') to the beginning of the file or buffer.",
   * it skips that '\n' and considers rest of the input buffer as data.
   * If "Content-length" string and end of headers is found 
   *   it substitutes single LF with CRLF in the headers, so the end of headers
   *   always will be CRLFCRLF (single CR in headers, if any, remain untouched)
   * else
   *   it puts "Content-length: "+size_of_data+CRLFCRLF at the beginning of the output buffer
   * and memcpy data to the output buffer 
   *
   * On failure outPostData and outPostDataLen will be set in 0.  
   * @param aInPostData      - the post data
   * @param aInPostDataLen   - the length aInPostData
   * @param aOutPostData     - the buffer
   * @param aOutPostDataLen  - the length of aOutPostData
   */
  /* [noscript] void parsePostBufferToFixHeaders (in string aInPostData, in unsigned long aInPostDataLen, out string aOutPostData, out unsigned long aOutPostDataLen); */
  NS_IMETHOD ParsePostBufferToFixHeaders(const char *aInPostData, PRUint32 aInPostDataLen, char **aOutPostData NS_OUTPARAM, PRUint32 *aOutPostDataLen NS_OUTPARAM) = 0;

  /**
   * To create tmp file with Content len header in, it will use by http POST
   */
  /* [noscript] void createTmpFileToPost (in string aPostDataURL, out string aTmpFileName); */
  NS_IMETHOD CreateTmpFileToPost(const char *aPostDataURL, char **aTmpFileName NS_OUTPARAM) = 0;

  /**
   *  Creates a new plugin native window object
   */
  /* [noscript] void newPluginNativeWindow (out nsPluginNativeWindowPtr aPluginNativeWindow); */
  NS_IMETHOD NewPluginNativeWindow(nsPluginNativeWindow * *aPluginNativeWindow NS_OUTPARAM) = 0;

  /**
   *  Deletes plugin native window object created by NewPluginNativeWindow
   */
  /* [noscript] void deletePluginNativeWindow (in nsPluginNativeWindowPtr aPluginNativeWindow); */
  NS_IMETHOD DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow) = 0;

  /**
   * Instantiate a "dummy" java plugin if a java plugin that supports
   * NPRuntime is installed. This plugin is used for exposing
   * window.java and window.Packages. If the java plugin supports
   * NPRuntime and instantiation was successful, aOwners instance will
   * be non-null, if not, it will be null.
   */
  /* [noscript] void instantiateDummyJavaPlugin (in nsIPluginInstanceOwner aOwner); */
  NS_IMETHOD InstantiateDummyJavaPlugin(nsIPluginInstanceOwner *aOwner) = 0;

  /**
   * Get the plugin name for the plugin instance.
   * @param aInstance the plugin instance object
   * @param aPluginName returns a pointer to a shared readonly string value,
   *        it's only valid for the lifetime of the plugin instance - you must
   *        copy the string value if you need it longer than that.
   */
  /* [noscript] void getPluginName (in nsIPluginInstance aInstance, [shared] out string aPluginName); */
  NS_IMETHOD GetPluginName(nsIPluginInstance *aInstance, const char **aPluginName NS_OUTPARAM) = 0;

  /**
   * Get the plugin tag associated with a given plugin instance.
   * @param aInstance the plugin instance object
   * @return plugin tag object
   */
  /* [noscript] nsIPluginTag getPluginTagForInstance (in nsIPluginInstance aInstance); */
  NS_IMETHOD GetPluginTagForInstance(nsIPluginInstance *aInstance, nsIPluginTag **_retval NS_OUTPARAM) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIPluginHost, NS_IPLUGINHOST_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIPLUGINHOST \
  NS_IMETHOD Init(void); \
  NS_IMETHOD Destroy(void); \
  NS_IMETHOD LoadPlugins(void); \
  NS_SCRIPTABLE NS_IMETHOD ReloadPlugins(PRBool reloadPages); \
  NS_IMETHOD GetPlugin(const char *aMimeType, nsIPlugin **_retval NS_OUTPARAM); \
  NS_IMETHOD InstantiateEmbeddedPlugin(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner); \
  NS_IMETHOD InstantiateFullPagePlugin(const char *aMimeType, nsIURI *aURI, nsIStreamListener * & aStreamListener, nsIPluginInstanceOwner *aOwner); \
  NS_IMETHOD InstantiatePluginForChannel(nsIChannel *aChannel, nsIPluginInstanceOwner *aOwner, nsIStreamListener **_retval NS_OUTPARAM); \
  NS_IMETHOD SetUpPluginInstance(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner); \
  NS_IMETHOD IsPluginEnabledForType(const char *aMimeType); \
  NS_IMETHOD IsPluginEnabledForExtension(const char *aExtension, const char * & aMimeType); \
  NS_IMETHOD GetPluginCount(PRUint32 *aPluginCount); \
  NS_IMETHOD GetPlugins(PRUint32 aPluginCount, nsIDOMPlugin **aPluginArray NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GetPluginTags(PRUint32 *aPluginCount NS_OUTPARAM, nsIPluginTag ***aResults NS_OUTPARAM); \
  NS_IMETHOD StopPluginInstance(nsIPluginInstance *aInstance); \
  NS_IMETHOD HandleBadPlugin(PRLibrary * aLibrary, nsIPluginInstance *instance); \
  NS_SCRIPTABLE NS_IMETHOD FindProxyForURL(const char *aURL, char **aResult NS_OUTPARAM); \
  NS_IMETHOD UserAgent(const char * * resultingAgentString); \
  NS_IMETHOD SetIsScriptableInstance(nsIPluginInstance *aInstance, PRBool aScriptable); \
  NS_IMETHOD ParsePostBufferToFixHeaders(const char *aInPostData, PRUint32 aInPostDataLen, char **aOutPostData NS_OUTPARAM, PRUint32 *aOutPostDataLen NS_OUTPARAM); \
  NS_IMETHOD CreateTmpFileToPost(const char *aPostDataURL, char **aTmpFileName NS_OUTPARAM); \
  NS_IMETHOD NewPluginNativeWindow(nsPluginNativeWindow * *aPluginNativeWindow NS_OUTPARAM); \
  NS_IMETHOD DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow); \
  NS_IMETHOD InstantiateDummyJavaPlugin(nsIPluginInstanceOwner *aOwner); \
  NS_IMETHOD GetPluginName(nsIPluginInstance *aInstance, const char **aPluginName NS_OUTPARAM); \
  NS_IMETHOD GetPluginTagForInstance(nsIPluginInstance *aInstance, nsIPluginTag **_retval NS_OUTPARAM); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIPLUGINHOST(_to) \
  NS_IMETHOD Init(void) { return _to Init(); } \
  NS_IMETHOD Destroy(void) { return _to Destroy(); } \
  NS_IMETHOD LoadPlugins(void) { return _to LoadPlugins(); } \
  NS_SCRIPTABLE NS_IMETHOD ReloadPlugins(PRBool reloadPages) { return _to ReloadPlugins(reloadPages); } \
  NS_IMETHOD GetPlugin(const char *aMimeType, nsIPlugin **_retval NS_OUTPARAM) { return _to GetPlugin(aMimeType, _retval); } \
  NS_IMETHOD InstantiateEmbeddedPlugin(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner) { return _to InstantiateEmbeddedPlugin(aMimeType, aURL, aOwner); } \
  NS_IMETHOD InstantiateFullPagePlugin(const char *aMimeType, nsIURI *aURI, nsIStreamListener * & aStreamListener, nsIPluginInstanceOwner *aOwner) { return _to InstantiateFullPagePlugin(aMimeType, aURI, aStreamListener, aOwner); } \
  NS_IMETHOD InstantiatePluginForChannel(nsIChannel *aChannel, nsIPluginInstanceOwner *aOwner, nsIStreamListener **_retval NS_OUTPARAM) { return _to InstantiatePluginForChannel(aChannel, aOwner, _retval); } \
  NS_IMETHOD SetUpPluginInstance(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner) { return _to SetUpPluginInstance(aMimeType, aURL, aOwner); } \
  NS_IMETHOD IsPluginEnabledForType(const char *aMimeType) { return _to IsPluginEnabledForType(aMimeType); } \
  NS_IMETHOD IsPluginEnabledForExtension(const char *aExtension, const char * & aMimeType) { return _to IsPluginEnabledForExtension(aExtension, aMimeType); } \
  NS_IMETHOD GetPluginCount(PRUint32 *aPluginCount) { return _to GetPluginCount(aPluginCount); } \
  NS_IMETHOD GetPlugins(PRUint32 aPluginCount, nsIDOMPlugin **aPluginArray NS_OUTPARAM) { return _to GetPlugins(aPluginCount, aPluginArray); } \
  NS_SCRIPTABLE NS_IMETHOD GetPluginTags(PRUint32 *aPluginCount NS_OUTPARAM, nsIPluginTag ***aResults NS_OUTPARAM) { return _to GetPluginTags(aPluginCount, aResults); } \
  NS_IMETHOD StopPluginInstance(nsIPluginInstance *aInstance) { return _to StopPluginInstance(aInstance); } \
  NS_IMETHOD HandleBadPlugin(PRLibrary * aLibrary, nsIPluginInstance *instance) { return _to HandleBadPlugin(aLibrary, instance); } \
  NS_SCRIPTABLE NS_IMETHOD FindProxyForURL(const char *aURL, char **aResult NS_OUTPARAM) { return _to FindProxyForURL(aURL, aResult); } \
  NS_IMETHOD UserAgent(const char * * resultingAgentString) { return _to UserAgent(resultingAgentString); } \
  NS_IMETHOD SetIsScriptableInstance(nsIPluginInstance *aInstance, PRBool aScriptable) { return _to SetIsScriptableInstance(aInstance, aScriptable); } \
  NS_IMETHOD ParsePostBufferToFixHeaders(const char *aInPostData, PRUint32 aInPostDataLen, char **aOutPostData NS_OUTPARAM, PRUint32 *aOutPostDataLen NS_OUTPARAM) { return _to ParsePostBufferToFixHeaders(aInPostData, aInPostDataLen, aOutPostData, aOutPostDataLen); } \
  NS_IMETHOD CreateTmpFileToPost(const char *aPostDataURL, char **aTmpFileName NS_OUTPARAM) { return _to CreateTmpFileToPost(aPostDataURL, aTmpFileName); } \
  NS_IMETHOD NewPluginNativeWindow(nsPluginNativeWindow * *aPluginNativeWindow NS_OUTPARAM) { return _to NewPluginNativeWindow(aPluginNativeWindow); } \
  NS_IMETHOD DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow) { return _to DeletePluginNativeWindow(aPluginNativeWindow); } \
  NS_IMETHOD InstantiateDummyJavaPlugin(nsIPluginInstanceOwner *aOwner) { return _to InstantiateDummyJavaPlugin(aOwner); } \
  NS_IMETHOD GetPluginName(nsIPluginInstance *aInstance, const char **aPluginName NS_OUTPARAM) { return _to GetPluginName(aInstance, aPluginName); } \
  NS_IMETHOD GetPluginTagForInstance(nsIPluginInstance *aInstance, nsIPluginTag **_retval NS_OUTPARAM) { return _to GetPluginTagForInstance(aInstance, _retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIPLUGINHOST(_to) \
  NS_IMETHOD Init(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Init(); } \
  NS_IMETHOD Destroy(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Destroy(); } \
  NS_IMETHOD LoadPlugins(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->LoadPlugins(); } \
  NS_SCRIPTABLE NS_IMETHOD ReloadPlugins(PRBool reloadPages) { return !_to ? NS_ERROR_NULL_POINTER : _to->ReloadPlugins(reloadPages); } \
  NS_IMETHOD GetPlugin(const char *aMimeType, nsIPlugin **_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPlugin(aMimeType, _retval); } \
  NS_IMETHOD InstantiateEmbeddedPlugin(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner) { return !_to ? NS_ERROR_NULL_POINTER : _to->InstantiateEmbeddedPlugin(aMimeType, aURL, aOwner); } \
  NS_IMETHOD InstantiateFullPagePlugin(const char *aMimeType, nsIURI *aURI, nsIStreamListener * & aStreamListener, nsIPluginInstanceOwner *aOwner) { return !_to ? NS_ERROR_NULL_POINTER : _to->InstantiateFullPagePlugin(aMimeType, aURI, aStreamListener, aOwner); } \
  NS_IMETHOD InstantiatePluginForChannel(nsIChannel *aChannel, nsIPluginInstanceOwner *aOwner, nsIStreamListener **_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->InstantiatePluginForChannel(aChannel, aOwner, _retval); } \
  NS_IMETHOD SetUpPluginInstance(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetUpPluginInstance(aMimeType, aURL, aOwner); } \
  NS_IMETHOD IsPluginEnabledForType(const char *aMimeType) { return !_to ? NS_ERROR_NULL_POINTER : _to->IsPluginEnabledForType(aMimeType); } \
  NS_IMETHOD IsPluginEnabledForExtension(const char *aExtension, const char * & aMimeType) { return !_to ? NS_ERROR_NULL_POINTER : _to->IsPluginEnabledForExtension(aExtension, aMimeType); } \
  NS_IMETHOD GetPluginCount(PRUint32 *aPluginCount) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPluginCount(aPluginCount); } \
  NS_IMETHOD GetPlugins(PRUint32 aPluginCount, nsIDOMPlugin **aPluginArray NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPlugins(aPluginCount, aPluginArray); } \
  NS_SCRIPTABLE NS_IMETHOD GetPluginTags(PRUint32 *aPluginCount NS_OUTPARAM, nsIPluginTag ***aResults NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPluginTags(aPluginCount, aResults); } \
  NS_IMETHOD StopPluginInstance(nsIPluginInstance *aInstance) { return !_to ? NS_ERROR_NULL_POINTER : _to->StopPluginInstance(aInstance); } \
  NS_IMETHOD HandleBadPlugin(PRLibrary * aLibrary, nsIPluginInstance *instance) { return !_to ? NS_ERROR_NULL_POINTER : _to->HandleBadPlugin(aLibrary, instance); } \
  NS_SCRIPTABLE NS_IMETHOD FindProxyForURL(const char *aURL, char **aResult NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->FindProxyForURL(aURL, aResult); } \
  NS_IMETHOD UserAgent(const char * * resultingAgentString) { return !_to ? NS_ERROR_NULL_POINTER : _to->UserAgent(resultingAgentString); } \
  NS_IMETHOD SetIsScriptableInstance(nsIPluginInstance *aInstance, PRBool aScriptable) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetIsScriptableInstance(aInstance, aScriptable); } \
  NS_IMETHOD ParsePostBufferToFixHeaders(const char *aInPostData, PRUint32 aInPostDataLen, char **aOutPostData NS_OUTPARAM, PRUint32 *aOutPostDataLen NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->ParsePostBufferToFixHeaders(aInPostData, aInPostDataLen, aOutPostData, aOutPostDataLen); } \
  NS_IMETHOD CreateTmpFileToPost(const char *aPostDataURL, char **aTmpFileName NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->CreateTmpFileToPost(aPostDataURL, aTmpFileName); } \
  NS_IMETHOD NewPluginNativeWindow(nsPluginNativeWindow * *aPluginNativeWindow NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->NewPluginNativeWindow(aPluginNativeWindow); } \
  NS_IMETHOD DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow) { return !_to ? NS_ERROR_NULL_POINTER : _to->DeletePluginNativeWindow(aPluginNativeWindow); } \
  NS_IMETHOD InstantiateDummyJavaPlugin(nsIPluginInstanceOwner *aOwner) { return !_to ? NS_ERROR_NULL_POINTER : _to->InstantiateDummyJavaPlugin(aOwner); } \
  NS_IMETHOD GetPluginName(nsIPluginInstance *aInstance, const char **aPluginName NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPluginName(aInstance, aPluginName); } \
  NS_IMETHOD GetPluginTagForInstance(nsIPluginInstance *aInstance, nsIPluginTag **_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetPluginTagForInstance(aInstance, _retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsPluginHost : public nsIPluginHost
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGINHOST

  nsPluginHost();

private:
  ~nsPluginHost();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsPluginHost, nsIPluginHost)

nsPluginHost::nsPluginHost()
{
  /* member initializers and constructor code */
}

nsPluginHost::~nsPluginHost()
{
  /* destructor code */
}

/* [noscript] void init (); */
NS_IMETHODIMP nsPluginHost::Init()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void destroy (); */
NS_IMETHODIMP nsPluginHost::Destroy()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void loadPlugins (); */
NS_IMETHODIMP nsPluginHost::LoadPlugins()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void reloadPlugins (in boolean reloadPages); */
NS_IMETHODIMP nsPluginHost::ReloadPlugins(PRBool reloadPages)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] nsIPlugin getPlugin (in string aMimeType); */
NS_IMETHODIMP nsPluginHost::GetPlugin(const char *aMimeType, nsIPlugin **_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void instantiateEmbeddedPlugin (in string aMimeType, in nsIURI aURL, in nsIPluginInstanceOwner aOwner); */
NS_IMETHODIMP nsPluginHost::InstantiateEmbeddedPlugin(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void instantiateFullPagePlugin (in string aMimeType, in nsIURI aURI, in nsIStreamListenerRef aStreamListener, in nsIPluginInstanceOwner aOwner); */
NS_IMETHODIMP nsPluginHost::InstantiateFullPagePlugin(const char *aMimeType, nsIURI *aURI, nsIStreamListener * & aStreamListener, nsIPluginInstanceOwner *aOwner)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] nsIStreamListener instantiatePluginForChannel (in nsIChannel aChannel, in nsIPluginInstanceOwner aOwner); */
NS_IMETHODIMP nsPluginHost::InstantiatePluginForChannel(nsIChannel *aChannel, nsIPluginInstanceOwner *aOwner, nsIStreamListener **_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void setUpPluginInstance (in string aMimeType, in nsIURI aURL, in nsIPluginInstanceOwner aOwner); */
NS_IMETHODIMP nsPluginHost::SetUpPluginInstance(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void isPluginEnabledForType (in string aMimeType); */
NS_IMETHODIMP nsPluginHost::IsPluginEnabledForType(const char *aMimeType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void isPluginEnabledForExtension (in string aExtension, in constCharStarRef aMimeType); */
NS_IMETHODIMP nsPluginHost::IsPluginEnabledForExtension(const char *aExtension, const char * & aMimeType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] readonly attribute unsigned long pluginCount; */
NS_IMETHODIMP nsPluginHost::GetPluginCount(PRUint32 *aPluginCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void getPlugins (in unsigned long aPluginCount, out nsIDOMPlugin aPluginArray); */
NS_IMETHODIMP nsPluginHost::GetPlugins(PRUint32 aPluginCount, nsIDOMPlugin **aPluginArray NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getPluginTags (out unsigned long aPluginCount, [array, retval, size_is (aPluginCount)] out nsIPluginTag aResults); */
NS_IMETHODIMP nsPluginHost::GetPluginTags(PRUint32 *aPluginCount NS_OUTPARAM, nsIPluginTag ***aResults NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void stopPluginInstance (in nsIPluginInstance aInstance); */
NS_IMETHODIMP nsPluginHost::StopPluginInstance(nsIPluginInstance *aInstance)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void handleBadPlugin (in PRLibraryPtr aLibrary, in nsIPluginInstance instance); */
NS_IMETHODIMP nsPluginHost::HandleBadPlugin(PRLibrary * aLibrary, nsIPluginInstance *instance)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void findProxyForURL (in string aURL, out string aResult); */
NS_IMETHODIMP nsPluginHost::FindProxyForURL(const char *aURL, char **aResult NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void UserAgent (in nativeChar resultingAgentString); */
NS_IMETHODIMP nsPluginHost::UserAgent(const char * * resultingAgentString)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void setIsScriptableInstance (in nsIPluginInstance aInstance, in boolean aScriptable); */
NS_IMETHODIMP nsPluginHost::SetIsScriptableInstance(nsIPluginInstance *aInstance, PRBool aScriptable)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void parsePostBufferToFixHeaders (in string aInPostData, in unsigned long aInPostDataLen, out string aOutPostData, out unsigned long aOutPostDataLen); */
NS_IMETHODIMP nsPluginHost::ParsePostBufferToFixHeaders(const char *aInPostData, PRUint32 aInPostDataLen, char **aOutPostData NS_OUTPARAM, PRUint32 *aOutPostDataLen NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void createTmpFileToPost (in string aPostDataURL, out string aTmpFileName); */
NS_IMETHODIMP nsPluginHost::CreateTmpFileToPost(const char *aPostDataURL, char **aTmpFileName NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void newPluginNativeWindow (out nsPluginNativeWindowPtr aPluginNativeWindow); */
NS_IMETHODIMP nsPluginHost::NewPluginNativeWindow(nsPluginNativeWindow * *aPluginNativeWindow NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void deletePluginNativeWindow (in nsPluginNativeWindowPtr aPluginNativeWindow); */
NS_IMETHODIMP nsPluginHost::DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void instantiateDummyJavaPlugin (in nsIPluginInstanceOwner aOwner); */
NS_IMETHODIMP nsPluginHost::InstantiateDummyJavaPlugin(nsIPluginInstanceOwner *aOwner)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] void getPluginName (in nsIPluginInstance aInstance, [shared] out string aPluginName); */
NS_IMETHODIMP nsPluginHost::GetPluginName(nsIPluginInstance *aInstance, const char **aPluginName NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] nsIPluginTag getPluginTagForInstance (in nsIPluginInstance aInstance); */
NS_IMETHODIMP nsPluginHost::GetPluginTagForInstance(nsIPluginInstance *aInstance, nsIPluginTag **_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif

#ifdef MOZILLA_INTERNAL_API
/**
 * Used for creating the correct input stream for plugins
 * We can either have raw data (with or without \r\n\r\n) or a path to a file (but it must be native!)
 * When making an nsIInputStream stream for the plugins POST data, be sure to take into 
 * account that it could be binary and full of nulls, see bug 105417. Also, we need 
 * to make a copy of the buffer because the plugin may have allocated it on the stack.
 * For an example of this, see Shockwave registration or bug 108966
 * We malloc only for headers here, buffer for data itself is malloced by ParsePostBufferToFixHeaders()
 */
inline nsresult
NS_NewPluginPostDataStream(nsIInputStream **result,
                           const char *data,
                           PRUint32 contentLength,
                           PRBool isFile = PR_FALSE,
                           PRBool headers = PR_FALSE)
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  if (!data)
    return rv;
  if (!isFile) { // do raw data case first
    if (contentLength < 1)
      return rv;
    
    char *buf = (char*) data;
    if (headers) {
      // in assumption we got correctly formated headers just passed in
      if (!(buf = (char*)nsMemory::Alloc(contentLength)))
        return NS_ERROR_OUT_OF_MEMORY;
      memcpy(buf, data, contentLength);
    }
    nsCOMPtr<nsIStringInputStream> sis = do_CreateInstance("@mozilla.org/io/string-input-stream;1",&rv);
    if (NS_SUCCEEDED(rv)) {
      sis->AdoptData(buf, contentLength);  // let the string stream manage our data
      rv = CallQueryInterface(sis, result);
    }
    else if (headers) {
      nsMemory::Free(buf); // Cleanup the memory if the data was copied.
    }
  } else {
    nsCOMPtr<nsILocalFile> file; // tmp file will be deleted on release of stream
    nsCOMPtr<nsIInputStream> fileStream;
    if (NS_SUCCEEDED(rv = NS_NewNativeLocalFile(nsDependentCString(data), PR_FALSE, getter_AddRefs(file))) &&
        NS_SUCCEEDED(rv = NS_NewLocalFileInputStream(getter_AddRefs(fileStream),
                                                     file,
                                                     PR_RDONLY,
                                                     0600,
                                                     nsIFileInputStream::DELETE_ON_CLOSE |
                                                     nsIFileInputStream::CLOSE_ON_EOF))) {
      // wrap the file stream with a buffered input stream
      return NS_NewBufferedInputStream(result, fileStream, 8192);
    }
  }
  return rv;
}
#endif

#endif /* __gen_nsIPluginHost_h__ */
