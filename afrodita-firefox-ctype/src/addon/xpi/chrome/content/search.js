Components.utils.import("resource://SoffidESSOExtension/afrodita.jsm");  

AfroditaExtension.checkInit ();

window.addEventListener("load", function () {
	var lv = document.getElementById("labelVersion");
	console.log("doc="+document);
	console.log("lv="+lv);
	var version = AfroditaExtension.currentVersion();
	lv.value = (version.version);
	document.getElementById("selfServiceLink").href = version.url+"/selfservice/";
	document.getElementById("searchBox").focus();

	var domWindowUtils = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
    	.getInterface(Components.interfaces.nsIDOMWindowUtils);

	var rect = document.getElementById("vbox").getBoundingClientRect();
	window.moveTo (window.screenX - rect.width * domWindowUtils.screenPixelsPerCSSPixel, window.screenY);
	var u = window.location.search;
	if (u != null)
	{
		u = u.substring(1);
		if (u.length > 0)
		{
			onSearchServer(u, version.url);
		}
	}
	
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

	console.log (accounts);
	for (i in accounts)
	{
		console.log (accounts[i]);
		if (accounts[i].url.length > 0)
		{
			var hbox = document.createElement("hbox");
			div.appendChild(hbox);
			var targetUrl = accounts[i].url;
			
			var anchor = document.createElement("label");
			hbox.appendChild(anchor);
			anchor.setAttribute("href", targetUrl);
			anchor.setAttribute("class", "text-link");
			anchor.setAttribute("target", "_blank");
			anchor.setAttribute("value", accounts[i].name);

			var anchor2 = document.createElement("image")
			var accountName = accounts[i].account;
			var accountSystem = accounts[i].system;
			hbox.appendChild(anchor2);
			anchor2.setAttribute("src", "chrome://SoffidESSOExtension/content/link.png");
			anchor2.setAttribute("width", "20px");
			anchor2.setAttribute("height", "20px");
			anchor2.setAttribute("style", "cursor:pointer");
			anchor2.addEventListener("click", function () {
				window.open ( targetUrl,
					"_blank");
				window.setTimeout (window.close, 500);
			});
		}
	}
	
	var rect = document.getElementById("vbox").getBoundingClientRect();
	window.resizeTo (window.width, rect.height);
}

function onSearchServer (u, ssu)
{
	accounts = AfroditaExtension.searchAccountsForServer(u);

	var div = document.getElementById("searchResults"); 
	var c = div.childNodes;
	while (div.childNodes.length > 0)
	{
		div.removeChild(div.firstChild);
	}

	for (i in accounts)
	{
		var accountName = accounts[i].account;
		var accountSystem = accounts[i].system;

		var hbox = document.createElement("hbox");
		div.appendChild(hbox);
		var infoUrl = ssu+
			"/selfservice/index.zul?target=sharedAccounts/sharedAccounts.zul?account="+
			encodeURI(accountName)+
			"&system="+
			encodeURI(accountSystem);
		
		var anchor = document.createElement("label");
		hbox.appendChild(anchor);
		anchor.setAttribute("href", infoUrl);
		anchor.setAttribute("class", "text-link");
		anchor.setAttribute("target", "_blank");
		anchor.setAttribute("value", accounts[i].name);

		var anchor2 = document.createElement("image")
		hbox.appendChild(anchor2);
		anchor2.setAttribute("src", "chrome://SoffidESSOExtension/content/info.png");
		anchor2.setAttribute("width", "20px");
		anchor2.setAttribute("height", "20px");
		anchor2.setAttribute("style", "cursor:pointer");
		anchor2.addEventListener("click", function () {
			window.open ( infoUrl,
				"_blank");
			window.setTimeout (window.close, 500);
		});
	}
	
	var rect = document.getElementById("vbox").getBoundingClientRect();
	window.resizeTo (window.width, rect.height);
}
