var port = chrome.runtime.connect ({name: "soffidesso"});



port.onMessage.addListener (function (request) {
	try {
		if (request.action == "version")
	    {
			document.getElementById("labelError").style.display = "none";
			document.getElementById("labelSuccess").style.display = "block";
			document.getElementById("labelVersion").innerText = request.version;
	    	document.getElementById("selfServiceLink").setAttribute("href", request.url);
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
					var p = document.createElement("p");
					p.appendChild(anchor);
					div.appendChild(p);
				}
			}
	    }
	    else if (request.action == "getInfo")
	    {
	    	document.getElementById("searchBox").addEventListener("input", function () {searchResults(request.pageId)});
//	    	document.getElementById("searchBox").focus ();
	    	port.postMessage({message: "info", pageId: request.pageId});
	    }
	} catch (e) {
		alert ("Error "+e);
	}
});


function searchResults (pageId)
{
	var v = document.getElementById("searchBox").value;
	port.postMessage({message: "search", pageId: pageId, text: v});
}
