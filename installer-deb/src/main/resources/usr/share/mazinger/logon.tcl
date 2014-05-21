catch {
  wm withdraw .
}
package require registry
set profile $env(USERPROFILE)
set key {HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders}
registry set $key Desktop "$profile\\Escritorio"
registry set $key "Start Menu" "$profile\\Menú Inicio"
catch {
  registry delete {HKEY_LOCAL_MACHINE\Software\Policies\Microsoft\Windows NT\DNSClient} NameServer
}
tk_messageBox -title {Atenció} -icon warning -message {Es troba desconectat de la xarxa. No tots els serveis estaràn disponibles}
exit
