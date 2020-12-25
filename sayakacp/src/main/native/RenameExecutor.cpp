/*
 * RenameExecutor.cpp
 *
 *  Created on: 16/09/2011
 *      Author: u07286
 */

#include "RenameExecutor.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

RenameExecutor::RenameExecutor(): log ("RenameExecutor")
{

}

RenameExecutor::~RenameExecutor() {
}

#define ENTRY_NAME "SayakaPendingFileRenameOperations"

void RenameExecutor::execute()
{
	bool done = true;
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\Session Manager", 0, KEY_ALL_ACCESS, &hKey)
			== ERROR_SUCCESS) {
		static char achPath[2];
		DWORD dwType;
		DWORD size;
		size = 0;
		if ( ERROR_FILE_NOT_FOUND != RegQueryValueEx(hKey, ENTRY_NAME, NULL, &dwType,(LPBYTE) achPath, &size) )
		{
			char *buffer = (char*) malloc(size+1);
			if ( ERROR_SUCCESS == RegQueryValueEx(hKey, ENTRY_NAME, NULL, &dwType,(LPBYTE) buffer, &size) )
			{
				for (int i = 0; i < size && buffer[i]; )
				{
					char *f1 = &buffer[i];
					int len1 = strlen(f1);
					i += len1 + 1;
					char *f2 = &buffer[i];
					int len2 = strlen(f2);
					i+= len2 + 1;
					FILE *f = fopen (f2, "r");
					if (f != NULL)
					{
						fclose (f);
						log.info("Renaming %s to %s", f2, f1);

						std::string f3 = f1;
						f3 += ".old";

						if (! MoveFileEx(f1, f3.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) ) {
							log.warn (">> Error moving away file %s", f2);
							done = false;
						}
						if (! MoveFileEx(f2, f1, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) ) {
							log.warn (">> Error moving file %s", f2);
							done = false;
						}
					}
				}
				if (done)
					RegDeleteValueA(hKey, ENTRY_NAME);
			}
		} else {
			log.info("No files to rename");
		}
		RegCloseKey(hKey);
	} else {
		log.warn ("Cannot open key HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager");
	}
}



