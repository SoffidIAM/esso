var soffidCachedElements= {};
var soffidNextElement = 0;
var soffidTimer = undefined;
var soffidTimerTime = undefined;

function soffidRegisterElement(element) {
	if (element == null)
		return "";
	if (typeof (element.__Soffid_Afr_Id) == 'undefined')
	{
		element.__Soffid_Afr_Id = ++soffidNextElement;
		soffidCachedElements [element.__Soffid_Afr_Id] = element;
	}
	return element.__Soffid_Afr_Id;
}

function soffidHasInputInside (node) {
//	console.log("testing node: "+node.tagName);
	if (node.tagName != undefined && node.tagName.toLowerCase() === "input")
	{
		return true;
	}
	var elements = node.childNodes;
	for (var i = 0; i < elements.length; i++)
		if (soffidHasInputInside(elements[i]))
			return true;
	return false;
}

function soffidLoadProcedure () {
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
//					console.log("Parsing page");
					var pageId = request.pageId;
					var soffidPageId = pageId;
					window.addEventListener("unload", function () { 
						port.postMessage ({message: "onUnload", pageId: soffidPageId});
						port.disconnect();
					});
				    port.postMessage({url: document.URL, title: document.title, message: "onLoad", pageId: pageId});
 				    var observer = new MutationObserver ( function (mutations) {
					   mutations.forEach (function (mutation) {
						  if (mutation.type == 'childList' && mutation.addedNodes.length > 0 )
						  {
							  for (var i = 0; i < mutation.addedNodes.length; i++)
							  {
								  var node = mutation.addedNodes.item(i);
								  if (soffidHasInputInside(node))
								  {
									  var doc =  mutation.target.ownerDocument;
									  var newTime;
//									  console.log("timer="+soffidTimer);
//									  console.log("timerTime="+soffidTimerTime);
									  if (soffidTimer != undefined &&
										  new Date().getTime() - soffidTimerTime.getTime() < 6000 )
									  {
										  // Only cancel six times   
										  window.clearTimeout(soffidTimer);
										  newTime = soffidTimerTime;
//										  console.log ("Cancelling previous timer");
									  }
									  else // New timer
									  {
//										console.log ("Creating timer");
 										newTime = new Date();
									  }
//									  console.log("Creating timer");
									  soffidTimerTime = newTime;
									  soffidTimer = window.setTimeout(1000, function () {
//										console.log("TIMEOUT FINISHED");
										port.postMessage({url: document.URL, title: document.title, message: "onLoad", pageId: pageId});});
//									  console.log("Created timer "+soffidTimer);
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
			    	var txt = document.documentElement;
				    port.postMessage({response: soffidRegisterElement(txt), requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getElementById")
			    {
			    	var txt = document.getElementById(request.elementId);
			    	if ( typeof txt == 'undefined')
					    port.postMessage({requestId: request.requestId, pageId: request.pageId});
			    	else
			    	{
					    port.postMessage({response: soffidRegisterElement(txt), requestId: request.requestId, pageId: request.pageId});
			    	}
			    }
			    else if (request.action == "getElementsByTagName")
			    {
			    	var elements = document.getElementsByTagName (request.tagName);
			    	var response = []; 
			    	for (var i = 0; i < elements.length; i++)
			    	{
			    		e = elements[i];
				    	response[i] = soffidRegisterElement(e);
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
				    	response[i] = soffidRegisterElement(e);
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
				    	response[i] = soffidRegisterElement(e);
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
				    	response[i] = soffidRegisterElement(e);
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
				    	response[i] = soffidRegisterElement(e);
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
			    	var element = soffidCachedElements[elementId];
			    	if (typeof element != 'undefined')
			    		port.postMessage({response: element.getAttribute(request.attribute), requestId: request.requestId, pageId: request.pageId});
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getProperty")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
			    	if (typeof element != 'undefined')
			    		port.postMessage({response: element[request.attribute], requestId: request.requestId, pageId: request.pageId});
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getComputedStyle")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
			    	if (typeof element != 'undefined')
			    	{
			    		var s = window.getComputedStyle(element,null);
			    		port.postMessage({response: s[request.style], 
			    			requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getTagName")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
			    	if (typeof element != 'undefined')
			    		port.postMessage({response: element.tagName, requestId: request.requestId, pageId: request.pageId});
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "click")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
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
			    	var element = soffidCachedElements[elementId];
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
			    	var element = soffidCachedElements[elementId];
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
			    	var element = soffidCachedElements[elementId];
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
			    	var element = soffidCachedElements[elementId];
			    	
			    	if (typeof element != 'undefined')
			    	{
			    		port.postMessage({response: soffidRegisterElement(element.parentNode), 
							requestId: request.requestId, 
							pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getOffsetParent")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
			    	
			    	if (typeof element != 'undefined')
			    	{
			    		port.postMessage({response: soffidRegisterElement(element.offsetParent), 
							requestId: request.requestId, 
							pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getPreviousSibling")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
			    	
			    	if (typeof element != 'undefined')
			    	{
			    		port.postMessage({response: soffidRegisterElement(element.previousSibling), 
							requestId: request.requestId, 
							pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getNextSibling")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
			    	
			    	if (typeof element != 'undefined')
			    	{
			    		port.postMessage({response: soffidRegisterElement(element.nextSibling), 
							requestId: request.requestId, 
							pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "createElement")
			    {
			    	var tag = request.tagName;
			    	var element = document.createElement(tag);
		    		port.postMessage({response: soffidRegisterElement(element), 
							requestId: request.requestId, 
							pageId: request.pageId});
			    }
			    else if (request.action == "alert")
			    {
					alert (request.text);
		    		port.postMessage({response: "OK", requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "appendChild")
			    {
			    	var elementId = request.element;
			    	var elementId2 = request.child;
			    	var element = soffidCachedElements[elementId];
			    	var element2 = soffidCachedElements[elementId2];
			    	
			    	if (typeof element != 'undefined' && typeof element2 != 'undefined')
			    	{
			    		element.appendChild (element2);
			    		port.postMessage({response: "OK", requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "removeChild")
			    {
			    	var elementId = request.element;
			    	var elementId2 = request.child;
			    	var element = soffidCachedElements[elementId];
			    	var element2 = soffidCachedElements[elementId2];
			    	
			    	if (typeof element != 'undefined' && typeof element2 != 'undefined')
			    	{
			    		element.removeChild (element2);
			    		port.postMessage({response: "OK", requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "insertBefore")
			    {
			    	var elementId = request.element;
			    	var elementId2 = request.child;
			    	var elementId3 = request.before;
			    	var element = soffidCachedElements[elementId];
			    	var element2 = soffidCachedElements[elementId2];
			    	var element3 = soffidCachedElements[elementId3];
			    	
			    	if (typeof element != 'undefined' && 
						typeof element2 != 'undefined' && 
						typeof element3 != 'undefined')
			    	{
			    		element.insertBefore (element2, element3);
			    		port.postMessage({response: "OK", requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "subscribe")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
			    	var event = request.event;
			    	var listener = request.listener;
			    	
			    	if (typeof element != 'undefined')
			    	{
			    		element.addEventListener (event, function (ev) {
				    		port.postMessage({message: "event", eventId: listener, target: soffidRegisterElement(ev.target), pageId: request.pageId});
			    		}, true);
						port.postMessage({response: "OK", requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
					{
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId,
							"exception": "Unknown element"});
					}
			    }
			    else if (request.action == "unSubscribe")
			    {
		    		port.postMessage({response: "OK", requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "setProperty")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
			    	if (typeof element != 'undefined')
			    	{
			    		element[request.attribute] = request.value;
			    		port.postMessage({response: true, requestId: request.requestId, pageId: request.pageId});
						if (request.attribute == "value")
						{
							var evt  = document.createEvent ("HTMLEvents");
							evt.initEvent ("change", true, true);
							element.dispatchEvent(evt);
						}
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "setTextContent")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
			    	if (typeof element != 'undefined')
			    	{
			    		element.textContent = ( request.text );
			    		port.postMessage({response: true, requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "setAttribute")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
			    	if (typeof element != 'undefined')
			    	{
			    		element.setAttribute(request.attribute, request.value);
			    		port.postMessage({response: true, requestId: request.requestId, pageId: request.pageId});
						if (request.attribute == "value")
						{
							var evt  = document.createEvent ("HTMLEvents");
							evt.initEvent ("change", true, true);
							element.dispatchEvent(evt);
						}
			    	}
			    	else
			    		port.postMessage({error: true, 
							requestId: request.requestId, 
							pageId: request.pageId, 
							"exception": "Attribute not found"});
			    }
			    else if (request.action == "removeAttribute")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
			    	if (typeof element != 'undefined')
			    	{
			    		element.removeAttribute(request.attribute);
			    		port.postMessage({response: true, requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "getChildren")
			    {
			    	var elementId = request.element;
			    	var element = soffidCachedElements[elementId];
			    	
			    	if (typeof element != 'undefined')
			    	{
				    	var elements = element.childNodes;
				    	var response = []; 
				    	for (var i = 0; i < elements.length; i++)
				    	{
				    		e = elements[i];
							response[i]  = soffidRegisterElement(e); 
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
			    	alert ("Unexpected message on page: "+result);
			    }
		} catch (error ) {
    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId,
				"exception": error.message});
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

