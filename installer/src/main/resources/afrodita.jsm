var EXPORTED_SYMBOLS = ["AfroditaExtension"];  

function logea (doc, msg)
{

       doc.documentElement.appendChild (doc.createTextNode(msg));
};


var AfroditaExtension = {
  docids: new Array(),
  documents: new Array(),
  listeners: new Array(),
  callbacks: new Array(),
  counter: 1, 
  initialized: false,

  setHandler: function (name, returntype, params, impl) {
   v = ctypes.char.array(name.length+1)(name);
   AfroditaExtension.callbacks[name] = ctypes.FunctionType(ctypes.default_abi, returntype, params).ptr( impl )
   AfroditaExtension.AfrSetHandler_ (v, AfroditaExtension.callbacks[name]);
  },
  getDocument: function(docid) {
     return AfroditaExtension.documents[docid].document;
  },
  getElement: function(docid, elid) {
     var data = AfroditaExtension.documents[docid];
     if (data == null)
        return null;
     return data.elements[elid];
  },
  getWindow: function(docid) {
     var data = AfroditaExtension.documents[docid];
     if (data == null)
        return null;
     return data.window;
  },
  createStringResult: function (docid, str) {
     var data = AfroditaExtension.documents[docid];
     data.result = ctypes.char.array()(str);
     return data.result;
  },
  createNodeListResult: function (docid, nodeList) {
     var data = AfroditaExtension.documents[docid];
     var ids = new Array ();
     var i;
     for (i = 0; i < nodeList.length; i++) {
         ids[i] = AfroditaExtension.registerElement(docid, nodeList.item(i));
     }
     ids[i] = 0;
     data.result = ctypes.long.array()(ids);
     return data.result;
  },
  registerElement: function (docid, element) {
     if (element == null)
        return 0;

     if (typeof element.___Afr_id == 'undefined' ){
       var data = AfroditaExtension.documents[docid];
       element.___Afr_id = data.counter ++;
       data.elements[element.___Afr_id] = element;
     }
     return element.___Afr_id;
  },
  onPageLoad: function(aEvent) {
    AfroditaExtension.checkInit ();
    var doc = aEvent.originalTarget; // doc is document that triggered "onload" event
    if (AfroditaExtension.newInterface) {

       // Listen for page changes

       window = doc.defaultView;
       var docid = AfroditaExtension.counter ++;
       var observer = new window.MutationObserver ( 
       	function(mutations) { AfroditaExtension.onPageChanged(mutations, docid);} );
       var config = {attributes: false, childList: true, characterData: false, subtree: true };
       observer.observe (doc, config);

       AfroditaExtension.documents[docid] = {
         id: docid, 
         document: doc, 
         elements: new Array(),
         counter: 1,
         window: window
       };
       AfroditaExtension.AfrEvaluate (docid);

       // add event listener for page unload 
       aEvent.originalTarget.defaultView.addEventListener("unload", 
              function() { AfroditaExtension.onPageUnload(docid); }, true);

   } else {
       var c = Components.classes["@caib.es/afroditaf;1"];          
       c = c.getService ();
       c = c.QueryInterface(Components.interfaces.caibIAfroditaF);
       c.notify(doc);
    }
    
  },
  onPageChanged: function(mutations, docid) {
     mutations.forEach (function (mutation) {
       var doc =  mutation.target.ownerDocument;
       if (docid != undefined)
       {
         var ad = AfroditaExtension.getDocument(docid);
         if (ad != doc) 
           docid = undefined; 
       }
       if (docid == undefined)
       {
         docid=AfroditaExtension.counter ++;
         AfroditaExtension.documents[docid] = {
           id: docid, 
           document: doc, 
           elements: new Array(),
           counter: 1
         };
       }
	   if (AfroditaExtension.documents[docid].timeout != undefined)
	   {
		   AfroditaExtension.getWindow(docid).clearTimeout(AfroditaExtension.documents[docid].timeout);
	   }
	   AfroditaExtension.documents[docid].timeout =
			AfroditaExtension.getWindow(docid).setTimeout (
				function(){ AfroditaExtension.AfrEvaluate (docid)}, 1000);
//       AfroditaExtension.AfrEvaluate (docid);
       return;
     });
  },
  onPageUnload: function(docid) {
       AfroditaExtension.documents[docid] = null;
       // AfroditaExtension.Afr
  },

  checkInit: function () { 
       if ( AfroditaExtension.initialized )
          return;
	var c = Components.classes["@caib.es/afroditaf;1"];
	if (c == null) {          
	    AfroditaExtension.newInterface = true;
	    
	    var os = Components.classes["@mozilla.org/system-info;1"].getService(Components.interfaces.nsIPropertyBag).getProperty("name");

		var c = Components.classes["@mozilla.org/process/environment;1"];          
		c = c.getService ();
		e = c.QueryInterface(Components.interfaces.nsIEnvironment);
		// Calcular mazinger dir
		if (os == "Linux") {
		   var path = "/usr/lib/libafroditafc.so"
		} else {
	   	   var path = e.get ("ProgramW6432");
		   if (path == null || path == "")
		     path = e.get ("ProgramFiles") + "\\SoffidESSO\\AfroditaFC.dll";
		   else
		     path = path +"\\SoffidESSO\\AfroditaFC32.dll"
		}
	
		Components.utils.import("resource://gre/modules/ctypes.jsm");
		Components.utils.import("resource://SoffidESSOExtension/Preferences.jsm");
		Preferences.defaults.set("signon.rememberSignons", false);
		// Carregar biblioteca
		AfroditaExtension.lib = ctypes.open(path);
		// Crear funcions
		AfroditaExtension.AfrSetHandler_ = AfroditaExtension.lib . declare ("AFRsetHandler", ctypes.default_abi, ctypes.void_t, ctypes.char.ptr, ctypes.void_t.ptr);
		AfroditaExtension.AfrEvaluate = AfroditaExtension.lib . declare ("AFRevaluate", ctypes.default_abi, ctypes.void_t, ctypes.long);
		AfroditaExtension.AfrEvent = AfroditaExtension.lib . declare ("AFRevent", ctypes.default_abi, ctypes.void_t, ctypes.long);
	
		// Crear handlers
		AfroditaExtension.setHandler ("GetUrl", ctypes.char.ptr, [ctypes.long],
		       function (docid) {
			  var doc = AfroditaExtension.getDocument(docid);
			  return AfroditaExtension.createStringResult(docid, doc.URL);
		       } );
		AfroditaExtension.setHandler ("GetTitle", ctypes.char.ptr, [ctypes.long],
		       function (docid) {
			  var doc = AfroditaExtension.getDocument(docid);
			  return AfroditaExtension.createStringResult(docid, doc.title);
		       } );
		AfroditaExtension.setHandler ("GetDomain", ctypes.char.ptr, [ctypes.long],
		       function (docid) {
			  var doc = AfroditaExtension.getDocument(docid);
			  return AfroditaExtension.createStringResult(docid, doc.domain);
		       } );
		AfroditaExtension.setHandler ("GetCookie", ctypes.char.ptr, [ctypes.long],
		       function (docid, tag) {
			  var doc = AfroditaExtension.getDocument(docid);
			  return AfroditaExtension.createStringResult(docid, doc.cookie);
		       } );
		AfroditaExtension.setHandler ("GetDocumentElement", ctypes.long, [ctypes.long],
		       function (docid) {
			  var doc = AfroditaExtension.getDocument(docid);          
			  return AfroditaExtension.registerElement(docid, doc.documentElement);
		       } );
		AfroditaExtension.setHandler ("GetElementsByTagName", ctypes.long.ptr, [ctypes.long, ctypes.char.ptr],
		       function (docid, tag) {
			  var doc = AfroditaExtension.getDocument(docid);          
			  var tag = tag.readString();
			  var nodelist = doc.getElementsByTagName (tag);
			  return AfroditaExtension.createNodeListResult(docid, nodelist);
		       } );
		AfroditaExtension.setHandler ("GetElementById", ctypes.long, [ctypes.long, ctypes.char.ptr],
		       function (docid, tag) {
			  var doc = AfroditaExtension.getDocument(docid);          
			  var tag = tag.readString();
			  var node = doc.getElementById (tag);
			  return AfroditaExtension.registerElement(docid, node);
		       } );
		AfroditaExtension.setHandler ("GetImages", ctypes.long.ptr, [ctypes.long],
		       function (docid) {
			  var doc = AfroditaExtension.getDocument(docid);          
			  return AfroditaExtension.createNodeListResult(docid, doc.images);
		       } );
		AfroditaExtension.setHandler ("GetForms", ctypes.long.ptr, [ctypes.long],
		       function (docid) {
			  var doc = AfroditaExtension.getDocument(docid);          
			  return AfroditaExtension.createNodeListResult(docid, doc.forms);
		       } );
	
		AfroditaExtension.setHandler ("GetLinks", ctypes.long.ptr, [ctypes.long],
		       function (docid) {
			  var doc = AfroditaExtension.getDocument(docid);          
			  return AfroditaExtension.createNodeListResult(docid, doc.links);
		       } );
	
		AfroditaExtension.setHandler ("GetAnchors", ctypes.long.ptr, [ctypes.long],
		       function (docid) {
			  var doc = AfroditaExtension.getDocument(docid);          
			  return AfroditaExtension.createNodeListResult(docid, doc.anchors);
		       } );
	
		AfroditaExtension.setHandler ("Write", ctypes.void_t, [ctypes.long, ctypes.char.ptr],
		       function (docid, line) {
			  var doc = AfroditaExtension.getDocument(docid);    
			  var line = line.readString();
			  doc.write (line);
		       } );
	
		AfroditaExtension.setHandler ("WriteLn", ctypes.void_t, [ctypes.long, ctypes.char.ptr],
		       function (docid, line) {
			  var doc = AfroditaExtension.getDocument(docid);    
			  var line = line.readString();
			  doc.writeln (line);
		       } );
		AfroditaExtension.setHandler ("CreateElement", ctypes.long, [ctypes.long, ctypes.char.ptr],
		       function (docid, tag) {
			  var doc = AfroditaExtension.getDocument(docid);    
			  var tag = tag.readString();
			  var element = doc.createElement(tag);
			  return AfroditaExtension.registerElement(docid, element);
		       } );
		// ------- Element Handlers 
		AfroditaExtension.setHandler ("AppendChild", ctypes.void_t, [ctypes.long, ctypes.long, ctypes.long],
		       function (docid, elementid, childid) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  var child = AfroditaExtension.getElement(docid, childid);    
			  el.appendChild(child);
		       } );
		AfroditaExtension.setHandler ("InsertBefore", ctypes.void_t, [ctypes.long, ctypes.long, ctypes.long, ctypes.long],
		       function (docid, elementid, childid, beforeid) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  var child = AfroditaExtension.getElement(docid, childid);    
			  var before = AfroditaExtension.getElement(docid, beforeid);    
			  el.insertBefore(child, before);
		       } );
		AfroditaExtension.setHandler ("GetProperty", ctypes.char.ptr, [ctypes.long, ctypes.long, ctypes.char.ptr],
		       function (docid, elementid, atr) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  var atr = atr.readString();
			  return AfroditaExtension.createStringResult(docid, ""+el[atr]);
		       } );
		AfroditaExtension.setHandler ("GetAttribute", ctypes.char.ptr, [ctypes.long, ctypes.long, ctypes.char.ptr],
		       function (docid, elementid, atr) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  var atr = atr.readString();
			  return AfroditaExtension.createStringResult(docid, el.getAttribute(atr));
		       } );
	
		AfroditaExtension.setHandler ("SetAttribute", ctypes.void_t, [ctypes.long, ctypes.long, ctypes.char.ptr, ctypes.char.ptr],
		   function (docid, elementid, atr, v) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  if (el != null) {
			     var atr = atr.readString();
			     var v = v.readString();
			     el.setAttribute(atr,v);
			     if (atr == "value") {
					var evt  = document.createEvent ("HTMLEvents");
					evt.initEvent ("change", true, true);
					el.dispatchEvent(evt);
			     }
			   }
		    } );
		AfroditaExtension.setHandler ("RemoveAttribute", ctypes.void_t, [ctypes.long, ctypes.long, ctypes.char.ptr],
		   function (docid, elementid, atr) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  if (el != null) {
			     var atr = atr.readString();
			     el.removeAttribute(atr);
			   }
		    } );
		AfroditaExtension.setHandler ("RemoveChild", ctypes.void_t, [ctypes.long, ctypes.long, ctypes.long],
		   function (docid, elementid, childid) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  if (el != null) {
				  var el2 = AfroditaExtension.getElement(docid, childid);    
 			      el.removeChild(el2);
			   }
		    } );
		AfroditaExtension.setHandler ("SetProperty", ctypes.void_t, [ctypes.long, ctypes.long, ctypes.char.ptr, ctypes.char.ptr],
		   function (docid, elementid, atr, v) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  if (el != null) {
			     var atr = atr.readString();
			     var v = v.readString();
			     el[atr]=v;
			     if (atr == "value") {
					var evt  = document.createEvent ("HTMLEvents");
					evt.initEvent ("change", true, true);
					el.dispatchEvent(evt);
			     }
			   }
		    } );
		AfroditaExtension.setHandler ("GetParent", ctypes.long, [ctypes.long, ctypes.long],
		       function (docid, elementid, atr, v) {
			   var el = AfroditaExtension.getElement(docid, elementid);    
			   return AfroditaExtension.registerElement(docid, el.parentNode);
		       } );
		AfroditaExtension.setHandler ("GetOffsetParent", ctypes.long, [ctypes.long, ctypes.long],
		       function (docid, elementid, atr, v) {
			   var el = AfroditaExtension.getElement(docid, elementid);    
			   return AfroditaExtension.registerElement(docid, el.offsetParent);
		       } );
		AfroditaExtension.setHandler ("GetPreviousSibling", ctypes.long, [ctypes.long, ctypes.long],
		       function (docid, elementid, atr, v) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  return AfroditaExtension.registerElement(docid, el.previousSibling);
		       } );
		AfroditaExtension.setHandler ("GetNextSibling", ctypes.long, [ctypes.long, ctypes.long],
		       function (docid, elementid, atr, v) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  return AfroditaExtension.registerElement(docid, el.nextSibling);
		       } );
		AfroditaExtension.setHandler ("GetTagName", ctypes.char.ptr, [ctypes.long, ctypes.long],
		       function (docid, elementid, atr, v) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  return AfroditaExtension.createStringResult(docid, el.nodeName);
		       } );
		AfroditaExtension.setHandler ("GetChildren", ctypes.long.ptr, [ctypes.long, ctypes.long],
		       function (docid, elementid) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  return AfroditaExtension.createNodeListResult(docid, el.children);
		       } );
	
		AfroditaExtension.setHandler ("Click", ctypes.void_t, [ctypes.long, ctypes.long],
		       function (docid, elementid) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  el.click();
		       } );
		AfroditaExtension.setHandler ("Focus", ctypes.void_t, [ctypes.long, ctypes.long],
		       function (docid, elementid) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  el.focus();
		       } );
		AfroditaExtension.setHandler ("Blur", ctypes.void_t, [ctypes.long, ctypes.long],
		       function (docid, elementid) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  el.blur();
		       } );
		AfroditaExtension.setHandler ("SubscribeEvent", ctypes.void_t, [ctypes.long, ctypes.long, ctypes.char.ptr, ctypes.long],
		       function (docid, elementid, eventName, eventId) {
				     AfroditaExtension.listeners[eventId] = function () { AfroditaExtension.AfrEvent (eventId); };
				     var en = eventName.readString();
				     if (elementid == 0)
   				     	AfroditaExtension.getWindow(docid).
				     		addEventListener (en,  AfroditaExtension.listeners[eventId]);
   				     else
   				     	AfroditaExtension.getElement(docid, elementid).
   				     		addEventListener (en,  AfroditaExtension.listeners[eventId]);
		       } );
		AfroditaExtension.setHandler ("UnsubscribeEvent", ctypes.void_t, [ctypes.long, ctypes.long, ctypes.char.ptr, ctypes.long],
		       function (docid, elementid) {
				  var en = eventName.readString();
 			      if (elementid == 0)
   				     	AfroditaExtension.getElement(docid, elementid).
   				     		removeEventListener (en,  AfroditaExtension.listeners[eventId]);
   				  else
   				     	AfroditaExtension.getWindow(docid).
				     		removeEventListener (en,  AfroditaExtension.listeners[eventId]);
				  AfroditaExtension.listeners[eventId] = undefined;
		       } );
		AfroditaExtension.setHandler ("Alert", ctypes.void_t, [ctypes.long, ctypes.char.ptr],
		       function (docid,msg) {
				  w = AfroditaExtension.getWindow(docid);    
				  var msg = msg.readString();
				  w.alert (msg);
		       } );
		AfroditaExtension.setHandler ("SetTextContent", ctypes.void_t, [ctypes.long, ctypes.long, ctypes.char.ptr],
		       function (docid, elementid, txt) {
				  var el = AfroditaExtension.getElement(docid, elementid);    
				  var txt = txt.readString();
				  el.textContent=txt;
		       } );
		AfroditaExtension.setHandler ("GetComputedStyle", ctypes.char.ptr, [ctypes.long, ctypes.long, ctypes.char.ptr],
		       function (docid, elementid, txt) {
				  var el = AfroditaExtension.getElement(docid, elementid);
				  var w = AfroditaExtension.getWindow(docid);    
				  var n = txt.readString();
				  var p = w.getComputedStyle (el, null)[n];
				  return AfroditaExtension.createStringResult(docid, p);
		       } );
      } else {
          AfroditaExtension.newInterface = false;
      }
      AfroditaExtension.initalized = true;
   }
};

