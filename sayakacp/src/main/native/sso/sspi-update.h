// Sspi missing methods

extern "C"  {

typedef PVOID PSEC_WINNT_AUTH_IDENTITY_OPAQUE; // the credential structure is opaque

typedef struct _SEC_WINNT_CREDUI_CONTEXT
{
    USHORT cbHeaderLength;
    HANDLE CredUIContextHandle; // the handle to call SspiGetCredUIContext()
#ifdef _CREDUI_INFO_DEFINED
    PCREDUI_INFOW UIInfo; // input from SspiPromptForCredentials()
#else
    PVOID UIInfo;
#endif // _CREDUI_INFO_DEFINED
    ULONG dwAuthError; // the authentication error
    PSEC_WINNT_AUTH_IDENTITY_OPAQUE pInputAuthIdentity;
    PUNICODE_STRING TargetName;
} SEC_WINNT_CREDUI_CONTEXT, *PSEC_WINNT_CREDUI_CONTEXT;
#define CREDUIWIN_ENUMERATE_ADMINS 0x100


SECURITY_STATUS
SEC_ENTRY
SspiUnmarshalCredUIContext(
    _In_reads_bytes_(MarshaledCredUIContextLength) PUCHAR MarshaledCredUIContext,
    _In_ ULONG MarshaledCredUIContextLength,
    _Outptr_ PSEC_WINNT_CREDUI_CONTEXT* CredUIContext
    );

}
