var port = chrome.runtime.connect ({name: "soffidesso"});

var pageId;
var selfServiceLink;

port.onMessage.addListener (function (request) {
	try {
		console.log ("Received "+JSON.stringify (request));
		
		if (request.action == "version")
	    {
			selfServiceLink = request.url ;
			
			document.getElementById("labelError").style.display = "none";
			document.getElementById("labelSuccess").style.display = "block";
			document.getElementById("labelVersion").innerText = request.version;
	    	document.getElementById("selfServiceLink").setAttribute("href", request.url+"/selfservice/");
	    	chrome.windows.getCurrent(function(w) {
	    	    chrome.tabs.getSelected(w.id,
	    	    	function (response){
	    	    		port.postMessage({message: "searchForServer", pageId: pageId, text: response.url});
	    	    });
	    	});
	    }
		else if (request.action == "searchResult")
	    {
	    	var result = request.result;
			var div = document.getElementById("searchResults"); 
			var c = div.childNodes;
			while (div.childNodes.length > 0)
			{
				div.removeChild(div.firstChild);
			}

			for (i in result)
			{
				if (result[i].url.length > 0)
				{
					var anchor = document.createElement("a");
					anchor.setAttribute("href", result[i].url);
					anchor.setAttribute("target", "_blank");
					var name = document.createTextNode(result[i].name);
					anchor.appendChild(name);
					var img = document.createElement("img");
					img.setAttribute("src", chrome.extension.getURL("img/link.png"))
					anchor.appendChild(img);
					var p = document.createElement("p");
					p.appendChild(anchor);
					div.appendChild(p);
				}
			}
	    }
		else if (request.action == "searchForServerResult")
	    {
			var accounts = request.result;
			var div = document.getElementById("searchResults"); 
			while (div.childNodes.length > 0)
			{
				div.removeChild(div.firstChild);
			}

			for (i in accounts)
			{
				var accountName = accounts[i].account;
				var accountSystem = accounts[i].system;

				var infoUrl = selfServiceLink+
					"/selfservice/index.zul?target=sharedAccounts/sharedAccounts.zul?account="+
					encodeURI(accountName)+
					"&system="+
					encodeURI(accountSystem);

				var anchor = document.createElement("a");
				anchor.setAttribute("href", accounts[i].url);
				anchor.setAttribute("target", "_blank");
				var name = document.createTextNode(accounts[i].name);
				anchor.appendChild(name);
				
				var anchor2 = document.createElement("a");
				anchor2.setAttribute("href", infoUrl);
				anchor2.setAttribute("target", "_blank");
				var img = document.createElement("img");
				img.setAttribute("src", chrome.extension.getURL("img/info.png"))
				anchor2.appendChild(img);
				
				var p = document.createElement("p");
				p.appendChild(anchor);
				p.appendChild(anchor2);
				div.appendChild(p);
			}
	    }
	    else if (request.action == "getInfo")
	    {
	    	var el = document.getElementById("searchBox");
	    	el.addEventListener("input", function () {searchResults(request.pageId)});
	    	pageId = request.pageId;
	    	if (el.value == "")
	    	{
		    	port.postMessage({message: "info", pageId: request.pageId});
	    	}
	    }
	} catch (e) {
		console.log ("Error "+e);
	}
});


function searchResults (pageId)
{
	var v = document.getElementById("searchBox").value;
	port.postMessage({message: "search", pageId: pageId, text: v});
}


