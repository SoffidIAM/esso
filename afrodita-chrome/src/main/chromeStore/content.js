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

function parsePageData () {
	var pageData = {};
	pageData.url = document.URL;
	pageData.title = document.tile;
	pageData.forms = [];
	pageData.inputs = [];
//	console.log("FORMS="+JSON.stringify(document.forms));
	// Parse forms
	for (var i = 0; i < document.forms.length; i++)
	{
		var form = document.forms[i];
		var formData = {};
		formData.action = form.action;
		formData.id = form.id;
		formData.name = form.name;
		formData.soffidId = soffidRegisterElement(form);
		formData.method = form.method;
		formData.inputs = [];
		pageData.forms.push (formData);
	}
	
	// Parse inputs
	var inputs = document.getElementsByTagName("input");
//	console.log("INPUTS="+JSON.stringify(inputs));
	for (var i = 0; i < inputs.length; i++)
	{
		var input = inputs[i];
		var inputData = {};
		var cs= {};
		try {
			cs = window.getComputesStyle(input)
		} catch (e) {
		}
		inputData.clientHeight = input.clientHeight;
		inputData.clientWitdh  = input.clientWidth;
		inputData.data_bind    = input.getAttribute("data-bind");
		inputData.display      = cs["display"];
		inputData.id           = input.id;
		inputData.name         = input.name;
		inputData.offsetHeight = input.offsetHeight;
		inputData.offsetLeft   = input.offsetLeft;
		inputData.offsetTop    = input.offsetTop;
		inputData.offsetWidth  = input.offsetWidth;
		inputData.style        = input.getAttribute("style");
		inputData.soffidId     = soffidRegisterElement(input);
		inputData.textAlign    = cs["text-align"] ;
		inputData.type         = input.type;
		inputData.visibility   = cs["visibility"];
		inputData.mirrorOf     = input.soffidMirrorOf;
		inputData.inputType    = input.soffidInputType;
		// Check parent visibility
		var parent = input.parentElement;
		try {
			while (parent != null)
			{
				var cs = window.getComputedStyle(parent);
				if (cs.visibility == "hidden")
					inputData.visibility = "hidden";
				if (cs.display == "none")
					inputData.display = "none";
				parent = parent.parentElement;
			}
		} catch (e) {
			// Ignore failure to get computed style for htmldocument
		}
		// Add to form
		var form = input.form;
		var found = false;
		if ( form ) {
			var soffidId = soffidRegisterElement(form);
			for (f in pageData.forms)
			{
				var formData = pageData.forms[f];
				if (formData.soffidId == soffidId)
				{
					found = true;
					formData.inputs.push (inputData);
					break;
				}
			}
			if (! found)
			{
				formData = {inputs:[]};
				formData.action = form.getAttribute("action");
				formData.method = form.getAttribute ("method");
				formData.id = form.getAttribute("id");
				formData.name = form.getAttribute("name");
				formData.soffidId = soffidId;
				formData.inputs.push (inputData);
				pageData.forms.push (formData);
			}
		}
		else {
			pageData.inputs.push (inputData);
		}
	}
	
	return pageData;
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
					var pageId = request.pageId;
					var soffidPageId = pageId;
					try {
						window.addEventListener("unload", function () {
							try {
								port.postMessage ({message: "onUnload", pageId: soffidPageId});
							} catch (e) {
								console.log ("ERRR :"+e);
							}
//							port.disconnect();
						}, true);
					} catch (e ) {
						
					}
				    port.postMessage({url: document.URL, title: document.title, message: "onLoad", pageId: pageId,
				    	pageData: parsePageData()});
 				    var observer = new MutationObserver ( function (mutations) {
// 					   console.log("MUTATION");
 				       var launched = false;
					   mutations.forEach (function (mutation) {
						  if (mutation.type == 'childList' && mutation.addedNodes.length > 0 && ! launched )
						  {
							  for (var i = 0; !launched && i < mutation.addedNodes.length; i++)
							  {
//			 					   console.log("MUTATION "+i);
								  var node = mutation.addedNodes.item(i);
								  if (soffidHasInputInside(node))
								  {
//				 					   console.log("MUTATION with input");
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
									  soffidTimer = window.setTimeout(function () {
										port.postMessage({url: document.URL, title: document.title, message: "onLoad", pageId: pageId, pageData: parsePageData()});},
										1000);
//									  console.log("Created timer "+soffidTimer);
									  launched = true;
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
			    		var e = elements[i];
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
			    		var e = elements[i];
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
			    		var e = elements[i];
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
			    		var e = elements[i];
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
			    		var e = elements[i];
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
			    			if (event == "click")
			    				ev.stopPropagation();
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
							evt  = document.createEvent ("HTMLEvents");
							evt.initEvent ("input", true, true);
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
				    		var e = elements[i];
							response[i]  = soffidRegisterElement(e); 
				    	}
					    port.postMessage({response: response, requestId: request.requestId, pageId: request.pageId});
			    	}
			    	else
			    		port.postMessage({error: true, requestId: request.requestId, pageId: request.pageId});
			    }
			    else if (request.action == "selectAction1")
			    {
			    	console.log("en selectAction1");
			    	var elementId = request.request.element;
			    	var element = soffidCachedElements[elementId];
			    	
			    	var windowRect = { left: window.screenLeft, top: window.screenTop,
			    			innerHeight: window.innerHeight,
			    			outerHeight: window.outerHeight,
			    			innerWidth: window.innerWidth,
			    			outerWidth: window.outerWidth
			    			};
			    	if (typeof element != 'undefined')
			    	{
			    		console.log("element:"+element);
			    		var rect = element.getBoundingClientRect();
					    port.postMessage({message: "selectAction2", request: request.request, 
					    	rect: {left: rect.left, 
					    		top: rect.top,
					    		right: rect.right,
					    		bottom: rect.bottom,
					    		width: rect.width,
					    		height: rect.height},
					    	window: windowRect});
			    	}
			    	else
					    port.postMessage({message: "selectAction2", request: request.request,
					    	window: windowRect});
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

