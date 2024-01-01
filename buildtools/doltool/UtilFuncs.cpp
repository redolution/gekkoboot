// 
// UtilFuncs.cpp
// 
// helper functions
// (put all platform specific code here)
// 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "UtilFuncs.h"
//#include "windows.h"	// needed for SetEndOfFile() call


// set new end of file
// (file must not be open when calling this function)
// 
// args:	filename of file to change end of file location
//			end of file offset
// returns:	true if successful
bool SetNewEndOfFile(const char* filename, u32 offset)
{
	/*HANDLE hFILE = CreateFile(filename, GENERIC_WRITE|GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(hFILE == INVALID_HANDLE_VALUE)
		return false;
	SetFilePointer(hFILE, offset, 0, FILE_BEGIN);
	SetEndOfFile(hFILE);
	CloseHandle(hFILE);*/
    char t[1000];
    sprintf(t, "dd if=%s of=tmp bs=1 count=%i", filename, offset);
    system(t);
    
    sprintf(t, "mv tmp %s", filename);
    system(t);
	return true;
}


// changes the file extension of a filename
// (old extension should be the same length as the new extension)
// 
// args:	filename
//			new extension to give filename eg "elf"
// returns:	pointer to changed filename
const char* ChangeFileExtension(char* filename, const char* newExtension)
{
	// get the length of the extension
	s32 extensionLength = 0;
	s32 extensionOffset = 0;
	if(newExtension[0] == '.')
		extensionOffset = 1;
	extensionLength = strlen(&newExtension[extensionOffset]);
	
	// check extension length of filename is same as new extension length
	if(strlen(filename) - extensionLength - 1 < 0)
		return filename;
	if(filename[strlen(filename) - extensionLength - 1] != '.')
		return filename;
	
	// replace extension
	strcpy(&filename[strlen(filename) - extensionLength], &newExtension[extensionOffset]);
	return filename;
}


