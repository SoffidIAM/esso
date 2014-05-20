/* Header for class es_caib_seycon_sso_windows_Hook */


#ifndef MAZINGERHOOK_H_

#define MAZINGERHOOK_H_

#include <wchar.h>

#ifndef MAZINGERAPI
  #ifdef __cplusplus
    #define MAZINGERAPI extern "C"
  #else
    #define MAZINGERAPI extern
  #endif
#endif

class AbstractWebApplication;

typedef void (*MAZINGER_STOP_CALLBACK) (const char *user);

MAZINGERAPI void MZNSetSecrets (const char *user, const wchar_t* szSecrets);
MAZINGERAPI void MZNSetDebugLevel (const char *user, int debuglevel);
MAZINGERAPI void MZNEnableSpy (const char *user, int debuglevel);
MAZINGERAPI void MZNStart (const char *user);
MAZINGERAPI void MZNStatus (const char *user);
MAZINGERAPI int MZNIsStarted (const char *user);
MAZINGERAPI void MZNSetStopNotifyCallback (MAZINGER_STOP_CALLBACK stopCallback);
MAZINGERAPI void MZNCheckPasswords(const char *user);
MAZINGERAPI bool MZNLoadConfiguration (const char *user, const wchar_t* lpszFile);
MAZINGERAPI void MZNStop (const char *user);


#endif


