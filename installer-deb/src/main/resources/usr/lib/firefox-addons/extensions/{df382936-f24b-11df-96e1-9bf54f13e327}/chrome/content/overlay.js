Components.utils.import("resource://MazingerExtension/afrodita.jsm");  
   
window.addEventListener("load", function() {
    // initialization code
//    this.initialized = true;
    
    var appcontent = document.getElementById("appcontent");   // browser
    if(appcontent)
      appcontent.addEventListener("DOMContentLoaded", AfroditaExtension.onPageLoad, true);

 }, false);
       