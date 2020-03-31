#include "hllapi.h"
#include <stdio.h>
#include <string.h>
#include <stdio.h>

// #define DEBUG

typedef long __stdcall (*HllApiType) (LPINT function, LPSTR data, LPINT len, LPINT rc );

HllApiType oldapi = NULL;


#ifdef DEBUG
FILE *f = NULL;
#endif

extern "C"  __stdcall long hllapi ( LPWORD function, LPSTR data, LPWORD len, LPWORD rc)
{

	INT function2 = *function;
	INT len2 = *len;
	INT rc2 = *rc;
	int status;
	char *data2;

#ifdef DEBUG
	if (f != NULL)
		fprintf (f, "hllapi: Function %d\n", (int) function2);
#endif

	if (oldapi == NULL)
	{
		HINSTANCE hInstance = LoadLibrary ("PCSHLL32.DLL");
		if (hInstance != NULL)
		{
			oldapi = (HllApiType) GetProcAddress (hInstance, "hllapi");
		}
	}
	if( oldapi == NULL)
		return 1;

	switch (function2)
	{
	case 1: // Connect Presentation Space
	{
		char *data3 = strdup("WRITE_WRITE");

		int rc3, len3 = strlen(data3);
		int function3 = 9;
		int result = oldapi(&function3, data3, &len3, &rc3) ;

#ifdef DEBUG
		fprintf (f, "NEW SESSION OPTIONS: [%.*s]\n", len3, data3);
#endif
		free (data3);

		len2 = 4;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		break;
	}
	case 9: // Set Session Parameters
#ifdef DEBUG
		fprintf (f, "SESSION OPTIONS: [%.*s]\n", len2, data);
#endif
	case 2: // Disconnect Presentation Space
	case 3: // Send Key
	case 4: // Wait
	case 5: // Copy Presentation Space
	case 6: // Search Presentation Space
	case 7: // Query Cursor Location
	case 8: // Copy Presentation Space to String
	case 11: // Reserve
	case 12: // Release
	case 14: // Query Field Attribute
	case 15: // Copy String to Presentation Space
	case 16: // Pause
	case 21: // Reset
	case 30: // Search Field
	case 31: // Find Field Length
	case 32: // Copy string to field
	case 33: // Copy field to string
	case 40: // SetCursor
	case 45: // Query Additional Field Attribute
	case 80: // Start communication notification
	case 81: // Query Communication event
	case 82: // Stop Communication event
	case 90: // Send file
	case 91: // Receive file
	case 92: // Cancel file transfer
	case 110: // Start playing macro
		status = oldapi (&function2, data, &len2, &rc2);
		break;
	case 34: // Copy field to String
		status = oldapi (&function2, data, &len2, &rc2);
		*len = len2;
		break;
	case 20: // Query System
		len2 = 36;
		data2 = (char*) malloc(len2);
		status = oldapi (&function2, data2, &len2, &rc2);
		memcpy (data, data2, 29);
		memcpy (&data[29], &data2[30], 6);
		free (data2);
		break;
	case 23: // Start Host Notificatin
		len2 = 16;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		data2[8] = data[2];
		data2[9] = data[3];
		data2[12] = data[7];
		status = oldapi (&function2, data2, &len2, &rc2);
		data[0] = data2[0];
		if (*len >= 4)
		{
			data[2] = data2[8];
			data[3] = data2[9];
		}
		free (data2);
		break;
	case 24: // Query Host Update
		len2 = 4;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		break;
	case 25: // Stop host notification
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		break;
	case 13: // Copy OIA
		// Increase 1 byte
		len2 = 104;
		data2 = (char*) malloc(len2);
		status = oldapi (&function2, data2, &len2, &rc2);
		memcpy (data, data2, *len);
		free (data2);
		break;
	case 22: // Query Session Status
		len2 = 20;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		status = oldapi (&function2, data2, &len2, &rc2);
		data[0] = data2[0];
		memcpy (&data[1], &data2[4], 17);
		free (data2);
		break;
	case 10: // Query Sessions
		len2 = 16;
		data2 = (char*) malloc (16);
		data2[0] = data[0];
		data2[1] = '\0';
		data2[2] = '\0';
		data2[3] = '\0';
		data2[4] = data[1];
		data2[5] = data[2];
		data2[6] = data[3];
		data2[7] = data[4];
		data2[8] = data[5];
		data2[9] = data[6];
		data2[10] = data[7];
		data2[11] = data[8];
		data2[12] = data[9];
		data2[13] = data[10];
		data2[14] = data[11];
		data2[15] = '\0';
		status = oldapi (&function2, data2, &len2, &rc2);
		data[0] = data2[0];
		data[1] = data2[4];
		data[2] = data2[5];
		data[3] = data2[6];
		data[4] = data2[7];
		data[5] = data2[8];
		data[6] = data2[9];
		data[7] = data2[10];
		data[8] = data2[11];
		data[9] = data2[12];
		data[10] = data2[14];
		data[11] = data2[15];
		free (data2);
		*len = len2;
		break;
	case 41: // Start close intercept
		len2 = 12;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[8] = data[1];
		data2[9] = data[2];
		data2[4] = data[5];
		status = oldapi (&function2, data2, &len2, &rc2);
		data[0] = data2[0];
		data[1] = data2[8];
		data[2] = data2[9];

		free (data2);
		break;
	case 42: // Query close intercept
		len2 = 4;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		status = oldapi (&function2, data2, &len2, &rc2);

		free (data2);
		break;
	case 43: // Stop close intercept
		len2 = 4;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		status = oldapi (&function2, data2, &len2, &rc2);

		free (data2);
		break;
	case 50: // Start Keystroke Intercept
		len2 = *len;
		if (len2 < 32)
			len2 = 32;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		data2[8] = data[2];
		data2[9] = data[3];
		data2[12] = data[6];
		status = oldapi (&function2, data2, &len2, &rc2);
		data[0] = data2[0];
		data[2] = data2[8];
		data[3] = data2[9];

		free (data2);
		break;
	case 51: // Get Key
		len2 = 12;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		memcpy (&data2[4], &data[1], 7);
		status = oldapi (&function2, data2, &len2, &rc2);
		data[0] = data2[0];
		data[1] = data2[4];
		data[2] = data2[5];
		data[3] = data2[6];
		data[4] = data2[7];
		data[5] = data2[8];
		data[6] = data2[9];
		data[7] = data2[10];
		free (data2);
		break;
	case 52: // Post Intercept status
		len2 = 8 ;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		break;
	case 53: // Stop keystroke intercept
		len2 = 4;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		break;
	case 60: // Lock prsentation space API
		len2 = 8;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		data2[5] = data[2];
		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		break;
	case 61: // Lock windows service API
		len2 = 8;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		data2[5] = data[2];
		data2[10] = data[4];
		data2[11] = data[5];
		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		break;
	case 99: // Convert position
		len2 = 8;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		*len = len2;
		break;
	case 101: // Connect windows services
	case 102: // Disconnect windows services
		len2 = 4;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		break;
	case 103: // Query window coordinates
		len2 = 80;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		memcpy (&data2[1], &data[1], 16);
		status = oldapi (&function2, data2, &len2, &rc2);
		data[0] = data2[0];
		memcpy (&data[1], &data2[1], 16);
		free (data2);
		break;
	case 104: // Window status
		len2 = (*len == 16)? 24: 28;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		if (data2 [ 4 ] == '\1')
		{
			data2[6] = data[2];
			data2[7] = data[3];
			data2[8] = data[4];
			data2[9] = data[5];
			data2[12] = data[6];
			data2[13] = data[7];
			data2[16] = data[8];
			data2[17] = data[9];
			data2[20] = data[10];
			data2[21] = data[11];
			if (len2 == 28)
			{
				data2[24] = data[12];
				data2[25] = data[13];
				data2[26] = data[14];
				data2[27] = data[15];
			}
		}
		if (data2[4] == '\2')
		{
			data2[6] = data[2];
			data2[7] = data[3];
			data2[8] = data[4];
			data2[9] = data[5];
			data2[12] = data[6];
			data2[13] = data[7];
			data2[16] = data[8];
			data2[17] = data[9];
			data2[20] = data[10];
			data2[21] = data[11];
			if (len2 == 28)
			{
				data2[24] = data[12];
				data2[25] = data[13];
				data2[26] = data[14];
				data2[27] = data[15];
			}
		}
		if (data2[4] == '\3')
		{
			data2[6] = data[2];
			data2[7] = data[3];
			data2[8] = data[4];
			data2[9] = data[5];
			data2[10] = data[6];
			data2[11] = data[7];
			data2[12] = data[8];
			data2[13] = data[9];
			data2[14] = data[10];
			data2[15] = data[11];
			data2[16] = data[12];
			data2[17] = data[13];
			data2[18] = data[14];
			data2[19] = data[15];
			data2[20] = data[16];
			data2[21] = data[17];
			data2[22] = data[18];
			data2[23] = data[19];
		}
		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		break;
	case 105: // Change Switch list LT Name
	case 106: // Change PS Window Name
		len2 = 68;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		strcpy(&data2[5], &data[2]);
		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		break;
	case 120: // Connect for structured fields
		len2 = 16;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		data2[5] = data[2];
		data2[6] = data[3];
		data2[7] = data[4];
		data2[8] = data[5];
		data2[9] = data[6];
		data2[12] = data[7];
		data2[13] = data[8];
		data2[14] = data[9];
		data2[15] = data[10];

		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		break;
	case 121: // Disconnect for structured fields
		len2 = 8;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		data2[5] = data[2];

		status = oldapi (&function2, data2, &len2, &rc2);
		free (data2);
		break;
	case 122: // Query Communications Buffer Size
		len2 = 20;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		data2[5] = data[2];
		data2[8] = data[3];
		data2[9] = data[4];
		data2[12] = data[5];
		data2[13] = data[6];
		data2[16] = data[7];
		data2[17] = data[8];

		status = oldapi (&function2, data2, &len2, &rc2);

		data2[1] = data[4];
		data2[2] = data[5];
		data2[3] = data[8];
		data2[4] = data[9];
		data2[5] = data[12];
		data2[6] = data[13];
		data2[7] = data[16];
		data2[8] = data[17];

		free (data2);
		break;
	case 123: // Allocate Communications Buffer
		len2 = 8;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[1] = data[1];
		data2[4] = data[2];
		data2[5] = data[3];
		data2[6] = data[4];
		data2[7] = data[5];

		status = oldapi (&function2, data2, &len2, &rc2);

		free (data2);
		break;
	case 124: // Free Communications Buffer
		len2 = 8;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[1] = data[1];
		data2[4] = data[2];
		data2[5] = data[3];
		data2[6] = data[4];
		data2[7] = data[5];

		status = oldapi (&function2, data2, &len2, &rc2);

		free (data2);
		break;
	case 125: // Get Request Completion
		len2 = 24;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		data2[8] = data[2];
		data2[9] = data[3];

		status = oldapi (&function2, data2, &len2, &rc2);

		data[4] = data2[10];
		data[5] = data2[11];
		data[6] = data2[12];
		data[7] = data2[13];
		data[8] = data2[14];
		data[9] = data2[15];
		data[10] = data2[16];
		data[11] = data2[17];
		data[12] = data2[20];
		data[13] = data2[21];
		free (data2);
		break;
	case 126: // Read structured fields
		len2 = 20;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		data2[6] = data[2];
		data2[7] = data[3];
		data2[8] = data[4];
		data2[9] = data[5];
		data2[10] = data[6];
		data2[11] = data[7];
		data2[16] = data[10];
		data2[17] = data[11];
		data2[18] = data[12];
		data2[19] = data[13];
		status = oldapi (&function2, data2, &len2, &rc2);
		if ('A' == data[4])
		{
			data[8] = data2[12];
			data[9] = data2[13];
		}
		if ('M' == data[4])
		{
			data[8] = data2[12];
			data[9] = data2[13];
			data[10] = data2[16];
			data[11] = data2[17];
		}
		free (data2);
		break;
	case 127: // Write structured fields
		len2 = 20;
		data2 = (char*) malloc(len2);
		data2[0] = data[0];
		data2[4] = data[1];
		data2[6] = data[2];
		data2[7] = data[3];
		data2[8] = data[4];
		data2[9] = data[5];
		data2[10] = data[6];
		data2[11] = data[7];
		data2[16] = data[10];
		data2[17] = data[11];
		data2[18] = data[12];
		data2[19] = data[13];
		status = oldapi (&function2, data2, &len2, &rc2);
		if ('A' == data[4])
		{
			data[8] = data2[12];
			data[9] = data2[13];
		}
		if ('M' == data[4])
		{
			data[8] = data2[12];
			data[9] = data2[13];
			data[10] = data2[16];
			data[11] = data2[17];
		}
		free (data2);
		break;
	default:
		return 1;
	}


//	*len = len2;
	*rc = rc2;
	return status;
}

extern "C" __declspec(dllexport) BOOL __stdcall DllMain(  HINSTANCE hinstDLL,
  DWORD dwReason,
  LPVOID lpvReserved
  ) {

  if (dwReason == DLL_PROCESS_ATTACH)
  {

#ifdef DEBUG
	  f = fopen("c:\\sewbr.txt", "at");
	  if (f != NULL)
		  setbuf (f, NULL);
	  else
		  MessageBox (NULL, "Cannot open log file", "SEWBR", MB_OK);
#endif
  }
  if (dwReason == DLL_PROCESS_DETACH)
  {
#ifdef DEBUG
	  if (f != NULL)
		  fclose (f);
#endif
  }
  return TRUE;
}




