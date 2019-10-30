var version = "2.6.0";
//alert ("Started 2");

var currentSelectActionMessage = null;

var mazingerBridge = {
	ports: [],
	pageId: 0,
	onConnect: function (port) {
		  try {
			var console = chrome.extension.getBackgroundPage().console;
			var pageId = mazingerBridge.pageId ++;
			mazingerBridge.ports[pageId] = port;
			port.onMessage.addListener (mazingerBridge.pageEventReceived);
 			port.postMessage({action: "getInfo", pageId: pageId});
			port.onDisconnect.addListener ( function (port) {
				mazingerBridge.ports[pageId] = null;

			});
		  } catch (e) { 
		  	alert ("Error on Soffid ESSO extension: "+e+" for "+document.URL);
		  }
	},
	pageEventReceived: function (msg, port) {
		var console = chrome.extension.getBackgroundPage().console;
		var pageId = msg.pageId;
//		console.log("Got message from "+pageId+": "+JSON.stringify(msg));
		if (msg.message == "onUnload")
		{
//			console.log("UNLOAD 2 "+JSON.stringify(msg));
			var p = mazingerBridge.ports[pageId];
			if (p != null) 
			{
//				console.log("UNLOAD 2 a");
				mazingerBridge.ports[pageId] = null;
//				p.disconnect ();
				try {
//					console.log("UNLOAD 3");
					mazingerBridge.port.postMessage({message: "onUnload2", pageId:pageId});
				} catch (e ) {
//					console.log("UNLOAD 3e "+e);
//					mazingerBridge.port.disconnect();
//					mazingerBridge.onDisconnect();
				}
			} else {
//				console.log("UNLOAD 2 b ");
				
			}
		}
		else if (msg.message == "selectAction2")
		{
//			console.log(JSON.stringify(msg));
			var hborder = (msg.window.outerWidth-msg.window.innerWidth)/2;
			var vborder = (msg.window.outerHeight-msg.window.innerHeight-hborder);
			var left = Math.round(msg.rect.left +
				msg.window.left +
				hborder);
			var top = Math.round(msg.rect.top +
				msg.window.top +
				vborder);
		
			chrome.windows.create({
					url: chrome.extension.getURL("selectAction.html"),
					type: "popup",
					left: left,
					top: top,
					width: 100, 
					height: 100
				});
		}
		else if (msg.message == "selectAction3")
		{
			port.postMessage({action: "selectAction4", request: currentSelectActionMessage});
		}
		else
		{
			if (typeof (msg.requestId) != 'undefined')
				msg.message = "response";
			try {
				mazingerBridge.port.postMessage(msg);
			} catch (e ) {
				mazingerBridge.port.disconnect();
				mazingerBridge.onDisconnect();
			}
		}
	},
 	ssoEventReceived: function (request) {
 		try {
			var console = chrome.extension.getBackgroundPage().console;
			var result = "";
			var pageId = parseInt(request.pageId);
//			console.log("Got message  for "+pageId+"/"+request.pageId+":"+JSON.stringify(request));
			var port = mazingerBridge.ports[pageId];
			if (port == null) {
				if ( request.requestId != null )
				{
					mazingerBridge.port.postMessage({
						"response": "error",
						"requestId": request.requestId,
						"pageId": request.pageId,
						"message": "response",
						error: true,
						"exception": "Page already unloaded"
					});
				}
			} 
			else if (request.action == "selectAction")
			{
				currentSelectActionMessage = request;
				port.postMessage({action: "selectAction1", request: request});
				// ACK 
				mazingerBridge.port.postMessage({response: "", requestId: request.requestId, pageId: request.pageId});
			}
			else
			{
				port.postMessage(request);
			}
 		} catch (error) {
//			console.log("Error generating message: "+error.message);
			mazingerBridge.port.postMessage({
				"requestId": request.requestId,
				"pageId": request.pageId,
				"message": "response",
				"error": true,
				"exception": error.message
			});
 		}
	},
	onDisconnect:  function (port) {
		// Reconnect on failure
		mazingerBridge.port = chrome.runtime.connectNative ("com.soffid.esso_chrome1");
		mazingerBridge.port.onMessage.addListener(mazingerBridge.ssoEventReceived);
		mazingerBridge.port.onDisconnect.addListener (mazingerBridge.onDisconnect);
	}
};


chrome.tabs.onUpdated
		.addListener(function(tabindex, changeinfo, tab) {
			if (changeinfo.status == "complete") {
			}
		});

chrome.runtime.onConnect.addListener(mazingerBridge.onConnect);
mazingerBridge.onDisconnect ();
