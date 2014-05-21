
var soffidTester = {

  test: function(document) {
        try {
		document.getElementById("pluginId").test();
		document.getElementById("labelSuccess").style.display='block';
        } catch (e) {	
		document.getElementById("labelError").style.display='block';
		document.getElementById("textError").textContent = e;
	}
	document.getElementById("label").style.display='none';
  }

};

// Run our kitten generation script as soon as the document's DOM is ready.
document.addEventListener('DOMContentLoaded', function () {
  soffidTester.test(document);
});


