// 
// UtilFuncs.h
// 
// helper functions
// (put all platform specific code here)
// 

#ifndef _UTIL_FUNCS_H_
#define _UTIL_FUNCS_H_

#include "Types.h"


// set new end of file
// (file must not be open when calling this function)
// 
// args:	filename of file to change end of file location
//			end of file offset
// returns:	true if successful
bool SetNewEndOfFile(const char* filename, u32 offset);


// changes the file extension of a filename
// (old extension should be the same length as the new extension)
// 
// args:	filename
//			new extension to give filename eg "elf"
// returns:	pointer to changed filename
const char* ChangeFileExtension(char* filename, const char* newExtension);


#endif

