/*
 * MazingerFF.cpp
 *
 *  Created on: 22/11/2010
 *      Author: u07286
 */

#define NS_SCRIPTABLE

#include <xpcom-config.h>
#include <mozilla-config.h>
#include <prtypes.h>
#include <nsIGenericFactory.h>
#include <nsISupportsImpl.h>
#include <nsIComponentManager.h>
#include <nsIComponentRegistrar.h>
#include <nsEmbedString.h>
#include <nsIDOMDocument.h>
#include <nsIDOMHTMLDocument.h>
#include <nsCOMPtr.h>
#include <iAfroditaF.h>
#include "AfroditaF.h"
#include "FFWebApplication.h"
#include <MazingerInternal.h>
#include <mozilla/Module.h>

#define AFRODITA_CID {0x2df6575d,0x1260,0x4874, {0xb7,0x04,0xd2,0x0b,0xdc,0x20,0xb5,0xe8}}

static const nsCID kIComponentRegistrarIID = NS_ICOMPONENTREGISTRAR_IID;
static const nsCID kIModuleIID = NS_IMODULE_IID;

static const nsCID kAfroditaCID = AFRODITA_CID;
static const nsCID kIAfroditaIID = CAIBIAFRODITAF_IID;

AfroditaF::AfroditaF() {
	NS_INIT_ISUPPORTS();
}

AfroditaF::~AfroditaF() {
}

NS_IMPL_ISUPPORTS1(AfroditaF, caibIAfroditaF)
;

// #include <windows.h>
NS_IMETHODIMP AfroditaF::Notify(nsIDOMDocument *doc) {

//	MessageBox (NULL, NS_IDOMHTMLDOCUMENT_IID_STR, "AFrodita", MB_OK);
	nsresult rv;
	nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(doc, &rv);
	if (!NS_FAILED(rv)) {
//		MessageBox (NULL, "Success", "AFrodita", MB_OK);
		FFWebApplication app(htmlDoc);

		MZNWebMatch(&app);
	} else {
//		MessageBox (NULL, "Failure", "AFrodita", MB_OK);
	}
	return NS_OK;
}



static nsresult
AfroditaF2Constructor(nsISupports *aOuter, const nsIID& aIID,
                            void **aResult)
{
    nsresult rv;

    AfroditaF * inst;

    *aResult = 0;
    if (0 != aOuter) {
        rv = ((nsresult) 0x80040110L);
        return rv;
    }

    do {
    inst = new AfroditaF();
  } while (0);
    if (0 == inst) {
        rv = ((nsresult) 0x8007000eL);
        return rv;
    }
    (inst)->AddRef();
    rv = inst->QueryInterface(aIID, aResult);
    do {
    (inst)->Release();
    (inst) = 0;
  } while (0);

    return rv;
}

// Build a table of ClassIDs (CIDs) which are implemented by this module. CIDs
// should be completely unique UUIDs.
// each entry has the form { CID, service, factoryproc, constructorproc }
// where factoryproc is usually NULL.
static const mozilla::Module::CIDEntry kSampleCIDs[] = {
    { &kAfroditaCID, false, NULL, AfroditaF2Constructor },
    { NULL }
};

// Build a table which maps contract IDs to CIDs.
// A contract is a string which identifies a particular set of functionality. In some
// cases an extension component may override the contract ID of a builtin gecko component
// to modify or extend functionality.
static const mozilla::Module::ContractIDEntry kSampleContracts[] = {
	{ "@caib.es/afroditaf;1", &kAfroditaCID },
    { NULL }
};

// Category entries are category/key/value triples which can be used
// to register contract ID as content handlers or to observe certain
// notifications. Most modules do not need to register any category
// entries: this is just a sample of how you'd do it.
// @see nsICategoryManager for information on retrieving category data.
static const mozilla::Module::CategoryEntry kSampleCategories[] = {
    { NULL }
};

static const mozilla::Module kSampleModule = {
    mozilla::Module::kVersion,
    kSampleCIDs,
    kSampleContracts,
    kSampleCategories
};

// The following line implements the one-and-only "NSModule" symbol exported from this
// shared library.
NSMODULE_DEFN(nsSampleModule) = &kSampleModule;

#if FF_VERSION >= 5
#else
NS_GENERIC_FACTORY_CONSTRUCTOR (AfroditaF) ;


static const nsModuleComponentInfo components[] = { {
		"Mazinger Firefox plugin",
		AFRODITA_CID,
		"@caib.es/afroditaf;1",
		(NSConstructorProcPtr)AfroditaFConstructor } };


NS_IMPL_NSGETMODULE(nsMazingerFFModule, components)

#endif


#ifdef WIN32
extern "C" BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD dwReason,
		LPVOID lpvReserved) {

	if (dwReason == DLL_PROCESS_ATTACH) {
		hMazingerInstance = hinstDLL;
	}
	return TRUE;
}

#endif
