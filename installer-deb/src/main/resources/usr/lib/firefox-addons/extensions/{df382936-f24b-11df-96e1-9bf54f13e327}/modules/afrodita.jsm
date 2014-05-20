var EXPORTED_SYMBOLS = ["AfroditaExtension"];  

var AfroditaExtension = {
  documents: new Array(),
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
       var docid = AfroditaExtension.counter ++;
       AfroditaExtension.documents[docid] = {
         id: docid, 
         document: doc, 
         elements: new Array(),
         counter: 1
       };
       AfroditaExtension.AfrEvaluate (docid);
       AfroditaExtension.documents[docid] = null;
       // add event listener for page unload 
       aEvent.originalTarget.defaultView.addEventListener("unload", 
              function() { AfroditaExtension.onPageUnload(); }, true);
	} else {
       var c = Components.classes["@caib.es/afroditaf;1"];          
       c = c.getService ();
       c = c.QueryInterface(Components.interfaces.caibIAfroditaF);
       c.notify(doc);
    }
    
  },

  onPageUnload: function(docid) {
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
		     path = e.get ("ProgramFiles") + "\\Mazinger\\AfroditaFC.dll";
		   else
		     path = path +"\\Mazinger\\AfroditaFC32.dll"
		}
	
		Components.utils.import("resource://gre/modules/ctypes.jsm");
		// Carregar biblioteca
		AfroditaExtension.lib = ctypes.open(path);
		// Crear funcions
		AfroditaExtension.AfrSetHandler_ = AfroditaExtension.lib . declare ("AFRsetHandler", ctypes.default_abi, ctypes.void_t, ctypes.char.ptr, ctypes.void_t.ptr);
		AfroditaExtension.AfrEvaluate = AfroditaExtension.lib . declare ("AFRevaluate", ctypes.default_abi, ctypes.void_t, ctypes.long);
	
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
		// ------- Element Handlers 
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
			  }
		       } );
		AfroditaExtension.setHandler ("GetParent", ctypes.long, [ctypes.long, ctypes.long],
		       function (docid, elementid, atr, v) {
			  var el = AfroditaExtension.getElement(docid, elementid);    
			  return AfroditaExtension.registerElement(docid, el.parentNode);
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
      } else {
          AfroditaExtension.newInterface = false;
      }
      AfroditaExtension.initalized = true;
   }
};

