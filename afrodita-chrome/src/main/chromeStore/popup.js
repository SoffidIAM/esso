var port = chrome.runtime.connect ({name: "soffidesso"});

port.onMessage.addListener (function (request) {
	try {
		if (request.action == "version")
	    {
			document.getElementById("labelError").style.display = "none";
			document.getElementById("labelSuccess").style.display = "block";
			document.getElementById("labelVersion").innerText = request.version;
	    }
	    else if (request.action == "getInfo")
	    {
		    port.postMessage({message: "info", pageId: request.pageId});
	    }
	} catch (e) {
		alert ("Error "+e);
	}
});


