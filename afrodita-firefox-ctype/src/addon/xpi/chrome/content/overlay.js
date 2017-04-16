Components.utils.import("resource://SoffidESSOExtension/afrodita.jsm");  
   
window.addEventListener("load", function() {
    // initialization code
//    this.initialized = true;
    
    var appcontent = document.getElementById("appcontent");   // browser
    if(appcontent)
      appcontent.addEventListener("load", AfroditaExtension.onPageLoad, true);

 }, false);
       

function installButton(toolbarId, id, afterId) {
    if (!document.getElementById(id)) {
        var toolbar = document.getElementById(toolbarId);

        // If no afterId is given, then append the item to the toolbar
        var before = null;
        if (afterId) {
            let elem = document.getElementById(afterId);
            if (elem && elem.parentNode == toolbar)
                before = elem.nextElementSibling;
        }

        toolbar.insertItem(id, before);
        toolbar.setAttribute("currentset", toolbar.currentSet);
        document.persist(toolbar.id, "currentset");

        if (toolbarId == "addon-bar")
            toolbar.collapsed = false;
    }
}			

var statusIcon  = "mazinger-status";
var element = null;

// Determine if Australis
if (document.getElementById("PanelUI-menu-button"))
{
    var cmButtonLocation = CustomizableUI.getPlacementOfWidget(statusIcon);    
	console.log("cmButtonLocation="+ cmButtonLocation);
	if (cmButtonLocation)
	{
		element = document.getElementById(statusIcon);								
	}
	else
	{
    	// Since first run, place on nav-bar bar
		console.log("Addtonavbar");
    	CustomizableUI.addWidgetToArea(statusIcon, "nav-bar");
		element = document.getElementById(statusIcon);
	}
}
else
{
	console.log("Firefox running Old School");

	element = document.getElementById(statusIcon);
	
	if (!element )
	{
    	// Since first run, place on nav-bar bar
    	installButton("nav-bar", statusIcon);
		element = document.getElementById(statusIcon);				
	}
}


var sdWindow = null;

function openSearchDialog (event)
{
	if (sdWindow)
		sdWindow.close();

	var url = ""
	try {
		var windowsService = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService(Components.interfaces.nsIWindowMediator);

		// window object representing the most recent (active) instance of Firefox
		var currentWindow = windowsService.getMostRecentWindow('navigator:browser');

		// most recent (active) browser object - that's the document frame inside the chrome
		var browser = currentWindow.getBrowser();

		// object containing all the data about an address displayed in the browser
		var uri = browser.currentURI;

		// textual representation of the actual full URL displayed in the browser
		url = uri.spec;
	} catch (e) {
		console.log("Error getting tabs: "+e)
	}
	sdWindow = window.open("chrome://SoffidEssoExtension/content/search.xul?"+url, 
            "doNothing", "chrome,resizable,dialog=no");
	var element = event.target;
	var rect = element.getBoundingClientRect();


	var domWindowUtils = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
    	.getInterface(Components.interfaces.nsIDOMWindowUtils);

	sdWindow.moveTo (window.screenX+ (rect.left+rect.width)* domWindowUtils.screenPixelsPerCSSPixel,
			window.screenY+(rect.top+rect.height)* domWindowUtils.screenPixelsPerCSSPixel);
}