var version = "3.0.0";
//alert ("Started 2");


var mazingerBridge = {
	ports: [],
	pageId: 0,
	onConnect: function (port) {
		  try {
			  var pageId = mazingerBridge.pageId ++;
			  mazingerBridge.ports[pageId] = port;
			  port.onMessage.addListener (mazingerBridge.pageEventReceived);
 			  port.postMessage({action: "getInfo", pageId: pageId});
			  port.onDisconnect.addListener ( function (port) {
				  mazingerBridge.ports[pageId] = undefined;
				  mazingerBridge.port.postMessage({
						"pageId": response.pageId,
						"message": "onUnload"
					});

			  });
		  } catch (e) { 
		  	alert ("Error on Soffid ESSO extension: "+e+" for "+document.URL);
		  }
	},
	pageEventReceived: function (msg, port) {
		var pageId = msg.pageId;
		if (msg.message == "onUnload")
		{
			var p = mazingerBridge.port[pageId];
			mazingerBridge.ports[pageId] = undefined;
			p.disconnect ();
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
 	ssoEventReceived: function (response) {
		var result = "";
		for (var key in response) {
			result = result + key+":"+response[key]+" ";
		}
		var pageId = parseInt(response.pageId);
		port = mazingerBridge.ports[pageId];
		if (port == undefined) {
				if ( response.requestId != undefined )
				{
					mazingerBridge.port.postMessage({
						"requestId": response.requestId,
						"pageId": response.pageId,
						"message": "response",
						error: true,
						"exception": "Page already unloaded"
					});
				}
		} else {
			port.postMessage(response);
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
