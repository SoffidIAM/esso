						
function NSGetModule() {
  return {
    registerSelf: function(compMgr, location, loaderStr, type) {
      var libFile = location;
      alert( "hola");
      alert ("libfile = "+libFile);
//      libFile.append("lib");
//      libFile.append(getLibFileName());

      // Note: we register a directory instead of an individual file because
      // Gecko will only load components with a specific file name pattern. We 
      // don't want this file to have to know about that. Luckily, if you
      // register a directory, Gecko will look inside the directory for files
      // to load.
//      var compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
//      compMgr.autoRegister(libFile);
    }
  }
}
								