Components.utils.import("resource://SoffidESSOExtension/afrodita.jsm");  

AfroditaExtension.checkInit ();

window.addEventListener("load", function () {
	var lv = document.getElementById("labelVersion");
	console.log("doc="+document);
	console.log("lv="+lv);
	var version = AfroditaExtension.currentVersion();
	lv.value = (version.version);
	document.getElementById("selfServiceLink").href = version.url;
	document.getElementById("searchBox").focus();

	var domWindowUtils = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
    	.getInterface(Components.interfaces.nsIDOMWindowUtils);

	var rect = document.getElementById("vbox").getBoundingClientRect();
	window.moveTo (window.screenX - rect.width * domWindowUtils.screenPixelsPerCSSPixel, window.screenY);
});

function onBlur ()
{
	window.setTimeout (window.close, 500);
}
function onChangeText ()
{
	var t = document.getElementById("searchBox").value;
	console.log("Input ..."+t);
	accounts = AfroditaExtension.searchAccounts(t);

	var div = document.getElementById("searchResults"); 
	var c = div.childNodes;
	while (div.childNodes.length > 0)
	{
		div.removeChild(div.firstChild);
	}

	for (i in accounts)
	{
		if (accounts[i].url.length > 0)
		{
			var anchor = document.createElement("label");
			anchor.setAttribute("href", accounts[i].url);
			anchor.setAttribute("class", "text-link");
			anchor.setAttribute("target", "_blank");
			anchor.setAttribute("value", accounts[i].name);
			div.appendChild(anchor);
		}
	}
	
	var rect = document.getElementById("vbox").getBoundingClientRect();
	window.resizeTo (window.width, rect.height);
}