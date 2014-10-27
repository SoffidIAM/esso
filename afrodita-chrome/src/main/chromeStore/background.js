chrome.tabs.onCreated.addListener(function(tab) {
	// alert('Hola tab '+tab);
	// chrome.debugger.attach({tabId:tab.id}, version,
	// onAttach.bind(null, tab.id));
});

if (true)
chrome.tabs.onUpdated
		.addListener(function(tabindex, changeinfo, tab) {
			if (changeinfo.status == "complete") {
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

			}
		});

var version = "1.0";

function onAttach(tabId) {
	if (chrome.runtime.lastError) {
		return;
	}

	chrome.windows.create({
		url : "headers.html?" + tabId,
		type : "popup",
		width : 800,
		height : 600
	});
}
