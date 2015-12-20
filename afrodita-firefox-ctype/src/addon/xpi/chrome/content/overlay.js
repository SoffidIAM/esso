Components.utils.import("resource://SoffidESSOExtension/afrodita.jsm");  
   
window.addEventListener("load", function() {
    // initialization code
//    this.initialized = true;
    
    var appcontent = document.getElementById("appcontent");   // browser
    if(appcontent)
      appcontent.addEventListener("load", AfroditaExtension.onPageLoad, true);

 }, false);
       
