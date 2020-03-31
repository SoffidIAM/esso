var port = chrome.runtime.connect ({name: "soffidesso"});

var pageId;
var selfServiceLink;

port.onMessage.addListener (function (request) {
	try {
		console.log ("Received "+JSON.stringify (request));
		
	    if (request.action == "getInfo")
	    {
	    	pageId = request.pageId;
	    	port.postMessage({message: "selectAction3", pageId: pageId});
	    }
	    if (request.action == "selectAction4")
	    {
	    	var data = request.request;
			document.getElementById("title").innerText = data.title;
			var holder = document.getElementById("actions");
			for (i in data.options)
			{
				var option = data.options [i];
				var div = document.createElement("div");
				div.setAttribute("class", "selector");
				div.innerText = option.name;
				holder.appendChild(div);
				div.addEventListener ("click", function() {
					alert (option.id);
				});
			}
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


