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
//    CustomizableUI.addListener(this._cm.customizableUIListener);	        
	console.log("Firefox running Australis");

/*	CustomizableUI.addListener(function(aNode, aNextNode, aContainer, aWasRemoval)
		{
		    if (aNode.id === statusIcon)
	    	{
				cookieMonsterBys1tony.cookieMonsterButton.checkButtonClick();
	    		//console.log("onWidgetAfterDOMChange - node: " + aNode.id + ", aContainer: " + aContainer.id);    		
	    	}		
		}
	);
	*/
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
console.log("element="+element);
if (element)
{
	element.addEventListener('click',
			function(event) {alert("hola")}, false);

	element.addEventListener('command',
			function(event) {alert("hola")}, false);

}


var sdWindow = null;

function openSearchDialog (event)
{
	if (sdWindow)
		sdWindow.close();
	sdWindow = window.open("chrome://SoffidEssoExtension/content/search.xul", 
            "doNothing", "chrome,resizable,dialog=no");
	var element = event.target;
	var rect = element.getBoundingClientRect();


	var domWindowUtils = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
    	.getInterface(Components.interfaces.nsIDOMWindowUtils);

	sdWindow.moveTo (window.screenX+ (rect.left+rect.width)* domWindowUtils.screenPixelsPerCSSPixel,
			window.screenY+(rect.top+rect.height)* domWindowUtils.screenPixelsPerCSSPixel);
}