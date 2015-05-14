var version = "2.0.0";
//alert ("Started 2");


var mazingerBridge = {
	ports: [],
	pageId: 0,
	onConnect: function (port) {
		  try {
			  var pageId = mazingerBridge.pageId ++;
			  mazingerBridge.port[pageId] = port;
			  port.onMessage.addListener (mazingerBridge.pageEventReceived);
			  port.postMessage({action: "getInfo", pageId: pageId});
			  port.onDisconnect.addListener ( function (port) {
				  delete mazingerBridge.port[pageId];
			  });
		  } catch (e) { 
		  	alert ("Error on Soffid ESSO extension: "+e+" for "+document.URL);
		  }
	},
	pageEventReceived: function (msg, port) {
		var pageId = msg.pageId;
		if (typeof (msg.requestId) != 'undefined')
			msg.message = "response";
		mazingerBridge.port.postMessage(msg);
	},
	ssoEventReceived: function (response) {
		var result = "";
		for (var key in response) {
			result = result + key+":"+response[key]+" ";
		}
		var pageId = parseInt(response.pageId);
		port = mazingerBridge.port[pageId];
		port.postMessage(response);
	}
};


chrome.tabs.onUpdated
		.addListener(function(tabindex, changeinfo, tab) {
			if (changeinfo.status == "complete") {
<<<<<<< HEAD
=======
				chrome.tabs.executeScript(
								{
									code : 
        'var btn = document.createElement("BUTTON"); '
	+ 'document.body.appendChild(btn); '
	+ 'btn.appendChild(document.createTextNode("HOLA"));'
	+ 'var embed = document.createElement("OBJECT"); '
	+ 'document.body.appendChild(embed); '
	+ 'embed.setAttribute("id", "soffid-sso-plugin");'
	+ 'embed.setAttribute("type", "application/soffid-sso-plugin");'
	+ ' try {' 
	+ 'document.getElementById("soffid-sso-plugin").run();'
	+ '} catch (e) { }' 
	});

>>>>>>> e59e2f63083a066b4ec2e0ded8ff3da6da89e946
			}
		});


chrome.runtime.onConnect.addListener(mazingerBridge.onConnect);
mazingerBridge.port = chrome.runtime.connectNative ("com.soffid.esso_chrome1");
mazingerBridge.port.onMessage.addListener(mazingerBridge.ssoEventReceived);

