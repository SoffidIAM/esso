catch {
  wm withdraw .
}
package require registry
set profile $env(USERPROFILE)
set key {HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders}
registry set $key Desktop "$profile\\Escritorio"
registry set $key "Start Menu" "$profile\\Men� Inicio"
catch {
  registry delete {HKEY_LOCAL_MACHINE\Software\Policies\Microsoft\Windows NT\DNSClient} NameServer
}
tk_messageBox -title {Atenci�} -icon warning -message {Es troba desconectat de la xarxa. No tots els serveis estar�n disponibles}
exit
