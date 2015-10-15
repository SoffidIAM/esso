var version = "3.0.0";
//alert ("Started 2");


var mazingerBridge = {
	ports: [],
	pageId: 0,
	onConnect: function (port) {
		  try {
			var console = chrome.extension.getBackgroundPage().console;
			var pageId = mazingerBridge.pageId ++;
			console.log("Got new connection from "+pageId);
			mazingerBridge.ports[pageId] = port;
			port.onMessage.addListener (mazingerBridge.pageEventReceived);
 			port.postMessage({action: "getInfo", pageId: pageId});
			port.onDisconnect.addListener ( function (port) {
			mazingerBridge.ports[pageId] = null;
//			mazingerBridge.port.postMessage({
//						"pageId": pageId,
//						"message": "onUnload"
//					});

			});
		  } catch (e) { 
		  	alert ("Error on Soffid ESSO extension: "+e+" for "+document.URL);
		  }
	},
	pageEventReceived: function (msg, port) {
		var console = chrome.extension.getBackgroundPage().console;
		var pageId = msg.pageId;
//		console.log("Got message "+msg.requestId+" from "+pageId);
		if (msg.message == "onUnload")
		{
			var p = mazingerBridge.port[pageId];
			if (p != null) 
			{
				mazingerBridge.ports[pageId] = null;
				p.disconnect ();
			}
		}
		else
		{
			if (typeof (msg.requestId) != 'undefined')
				msg.message = "response";
			try {
				mazingerBridge.port.postMessage(msg);
			} catch (e ) {
				alert("Error "+e);
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
//			console.log("Got message "+request.requestId+" for "+pageId+"/"+request.pageId);
			var port = mazingerBridge.ports[pageId];
			if (port == null) {
				console.log("Port is closed for "+pageId);
					if ( request.requestId != null )
					{
						console.log("Sending error for "+request.requestId);
						mazingerBridge.port.postMessage({
							"response": "error",
							"requestId": request.requestId,
							"pageId": request.pageId,
							"message": "response",
							error: true,
							"exception": "Page already unloaded"
						});
						console.log("Sent error for "+request.requestId);
					}
			} else {
				port.postMessage(request);
//				console.log("Forwarded message "+request.requestId+" to "+pageId);
			}
 		} catch (error) {
			console.log("Error generating message: "+error.message);
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
