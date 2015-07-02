var cachedElements= {};

var nextElement = 0;

function soffidLoadProcedure () {
     // alert("Connect on "+document.URL);
    var port = chrome.runtime.connect ();
    port.onMessage.addListener (function (request) {
		try {
				if (request.action == "getContent")
			    {
			    	var txt = document.documentElement.innerHTML;
				    port.postMessage({response: txt, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getInfo")
			    {
					var pageId = request.pageId;
				    port.postMessage({url: document.URL, title: document.title, message: "onLoad", pageId: pageId});
 				    var observer = new MutationObserver ( function (mutations) {
					   mutations.forEach (function (mutation) {
						  if (mutation.type == 'childList' && mutation.addedNodes.length > 0 )
						  {
							  for (var i = 0; i < mutation.addedNodes.length; i++)
							  {
								  var node = mutation.addedNodes.item(i);
								  if (node.tagName.toLowerCase() === "input")
								  {
									  var doc =  mutation.target.ownerDocument;
									  port.postMessage({url: document.URL, title: document.title, message: "onLoad", pageId: pageId});
									  return;
								  }
							  }
						  }
					    });
				      } );
				    var config = {attributes: false, childList: true, characterData: false, subtree: true };
				    observer.observe (document.documentElement, config);
					window.addEventListener("unload", function () { observer.disconnect();})
			    }
			    else if (request.action == "getUrl")
			    {
			    	var txt = document.location.url;
				    port.postMessage({response: txt, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getTitle")
			    {
			    	var txt = document.title;
				    port.postMessage({response: txt, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getDomain")
			    {
			    	var txt = document.domain;
				    port.postMessage({response: txt, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getCookie")
			    {
			    	var txt = document.cookie;
				    port.postMessage({response: txt, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getDocumentElement")
			    {
			    	element = nextElement ++;
			    	var txt = document.documentElement;
			    	cachedElements[element] = txt;
				    port.postMessage({response: element, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getElementById")
			    {
			    	element = nextElement ++;
			    	var txt = document.getElementById(request.elementId);
			    	if ( typeof txt == 'undefined')
					    port.postMessage({requestId: request.requestId, pageId: request.pageId});
			    	else
			    	{
				    	cachedElements[element] = txt;
					    port.postMessage({response: element, requestId: request.requestId, pageId: request.pageId});
			    	}
			    }
			    else if (request.action == "getElementsByTagName")
			    {
			    	var elements = document.getElementsByTagName (request.tagName);
			    	var response = []; 
			    	for (var i = 0; i < elements.length; i++)
			    	{
			    		e = elements[i];
				    	element = nextElement ++;
				    	cachedElements[element] = e;
				    	response[i] = element;
			    	}
				    port.postMessage({response: response, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getImages")
			    {
			    	var elements = document.images;
			    	var response = []; 
			    	for (var i = 0; i < elements.length; i++)
			    	{
			    		e = elements[i];
				    	element = nextElement ++;
				    	cachedElements[element] = e;
				    	response[i] = element;
			    	}
				    port.postMessage({response: response, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getImages")
			    {
			    	var elements = document.images;
			    	var response = []; 
			    	for (var i = 0; i < elements.length; i++)
			    	{
			    		e = elements[i];
				    	element = nextElement ++;
				    	cachedElements[element] = e;
				    	response[i] = element;
			    	}
				    port.postMessage({response: response, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getAnchors")
			    {
			    	var elements = document.anchors;
			    	var response = []; 
			    	for (var i = 0; i < elements.length; i++)
			    	{
			    		e = elements[i];
				    	element = nextElement ++;
				    	cachedElements[element] = e;
				    	response[i] = element;
			    	}
				    port.postMessage({response: response, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getForms")
			    {
			    	var elements = document.forms;
			    	var response = []; 
			    	for (var i = 0; i < elements.length; i++)
			    	{
			    		e = elements[i];
				    	element = nextElement ++;
				    	cachedElements[element] = e;
				    	response[i] = element;
			    	}
				    port.postMessage({response: response, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "write")
			    {
			    	document.write (request.text);
				    port.postMessage({response: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "writeln")
			    {
			    	document.writeln (request.text);
				    port.postMessage({response: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getAttribute")
			    {
			    	var elementId = request.element;
			    	var element = cachedElements[elementId];
			    	if (typeof element != 'undefined')
			    		port.postMessage({response: element[request.attribute], requestId: request.requestId, pageId: request.pageId});
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getTagName")
			    {
			    	var elementId = request.element;
			    	var element = cachedElements[elementId];
			    	if (typeof element != 'undefined')
			    		port.postMessage({response: element.tagName, requestId: request.requestId, pageId: request.pageId});
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "click")
			    {
			    	var elementId = request.element;
			    	var element = cachedElements[elementId];
			    	if (typeof element != 'undefined')
			    	{
			    		element.click ();
			    		port.postMessage({response: true, requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "focus")
			    {
			    	var elementId = request.element;
			    	var element = cachedElements[elementId];
			    	if (typeof element != 'undefined')
			    	{
			    		element.focus ();
			    		port.postMessage({response: true, requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "blur")
			    {
			    	var elementId = request.element;
			    	var element = cachedElements[elementId];
			    	if (typeof element != 'undefined')
			    	{
			    		element.blur ();
			    		port.postMessage({response: true, requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "click")
			    {
			    	var elementId = request.element;
			    	var element = cachedElements[elementId];
			    	if (typeof element != 'undefined')
			    	{
			    		element.click ();
			    		port.postMessage({response: true, requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getParent")
			    {
			    	var elementId = request.element;
			    	var element = cachedElements[elementId];
			    	
			    	if (typeof element != 'undefined')
			    	{
				    	elementId = nextElement ++;
				    	cachedElements[element] = element.parentNode;
			    		port.postMessage({response: elementId, requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "setAttribute")
			    {
			    	var elementId = request.element;
			    	var element = cachedElements[elementId];
			    	if (typeof element != 'undefined')
			    	{
			    		element[request.attribute] = request.value;
			    		port.postMessage({response: true, requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getChildren")
			    {
			    	var elementId = request.element;
			    	var element = cachedElements[elementId];
			    	
			    	if (typeof element != 'undefined')
			    	{
				    	var elements = element.childNodes;
				    	var response = []; 
				    	for (var i = 0; i < elements.length; i++)
				    	{
				    		e = elements[i];
					    	eid = nextElement ++;
					    	cachedElements[eid] = e;
					    	response[i] = eid;
				    	}
					    port.postMessage({response: response, requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    } else {
					var result = "";
					for (var key in request) {
						result = result + key+":"+request[key]+" ";
					}
			    	alert ("Unexpecte message on page: "+result);
			    }
		} catch (e ) {
			var r = "";
			for (var key in request) {
				r = r + key+":"+request[key]+" ";
			}

			alert ("Error processing message "+r+":"+e);
		}
      }
    );
};


var state = document.readyState ;
if (state == "uninitialized" || state == "loading")
	window.addEventListener("load", soffidLoadProcedure);
else {
	soffidLoadProcedure();
}

