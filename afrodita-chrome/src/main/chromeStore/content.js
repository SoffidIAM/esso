var i = 0;
var found = false;
while ( document.getElementById ("soffid-sso-plugin-"+i) != null)
   i++;
var embed = document.createElement("OBJECT");
embed.setAttribute("style", "visibility: hidden; width:1px; height: 1px;");
embed.setAttribute("id", "soffid-sso-plugin-"+i);
embed.setAttribute("type", "application/soffid-sso-plugin");
document.body.appendChild(embed); 
try {
	plugin = document.getElementById("soffid-sso-plugin-"+i);
        if ( ! (typeof plugin === 'undefined'))
        {
	   plugin.run();
	   document.body.removeChild(embed);
        }
} catch (e) { 
//	alert ("Error on Soffid ESSO plugin:"+e+" for "+document.URL);
}


