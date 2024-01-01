// 
// GenericFile.h
// 
// loser  -  may 2003
// 
// class for handling files and the data they contain
// this class operates directly on the files using standard file io routines
// 
// when extending this class the 'create' and 'moveData' functions
// may need to be extended too!
// 
// the size of buffers used when copying/moving data gets set
// by giving the buffer-size as an arg in the constructor
// 

#ifndef _GENERIC_FILE_H_
#define _GENERIC_FILE_H_

#include <stdio.h>
#include "Types.h"


class GenericFile
{
private:
protected:
	FILE *fd;			// file descriptor for this file
	char *filename;		// filename of this file
	u32 bufferSize;		// size of buffers used when copying data etc
	
	
	// set filename
	// 
	// args:	name to set as the filename of this file
	virtual void setFilename(const char* name);
	
	
	// write out data from file to output file
	// 
	// args:	offset in file to start getting data from
	//			size of data from file to write out
	//			file descriptor of file to write to
	// returns:	size of data written to output file
	//			ERROR_RETURN if error
	virtual u32  writeOutData(u32 writeOffset, u32 writeSize, FILE* out_fd) const;
	
	// read in data from input file to this file
	// 
	// args:	offset in file to start putting data at
	//			size of data to read into file
	//			file descriptor of file to read from
	// returns:	size of data read in from input file
	//			ERROR_RETURN if error
	virtual u32  readInData(u32 readOffset, u32 readSize, FILE* in_fd);
	
	// moves all the file data after the 'moveOffset'
	// to a new location within the file
	// a positive 'moveAmount' means move all data 'forwards'
	// a negative 'moveAmount' means to move all data 'backwards'
	// 
	// (moving data 'forward' allows for easy inserting of data)
	// (moving data 'backward' allows for easy removal of data)
	// attempting to move data to a location before the start
	// of the file will result in an error
	// 
	// this may need to be extended in extended classes if there
	// are header offsets etc that need to be kpet 'valid' after
	// moving data around in a file
	// 
	// args:	offset within file to move data from
	//			number of bytes to move data 'forward' by
	//			(a negative value means to move data 'backwards')
	// returns:	true if moved data ok
	virtual bool moveData(u32 moveOffset, s32 moveAmount);
	
	// inserts data into file
	// 
	// args:	offset in file to start inserting data at
	//			size of data to insert into file
	//			file descriptor of file to insert data from
	// returns:	size of data inserted from input file
	//			ERROR_RETURN if error
	virtual u32  insertData(u32 insertOffset, u32 insertSize, FILE* in_fd);
	
	// remove data from file
	// 
	// args:	offset in file to start removing data from
	//			size of data to remove from file
	// returns:	size of data removed from input file
	//			ERROR_RETURN if error
	virtual u32  removeData(u32 removeOffset, u32 removeSize);
	
	
public:
	// specify buffer size when creating file object
	// uses 100kb by default
	GenericFile(u32 bufferSize = 100*1024);
	virtual ~GenericFile();
	
	// creates a new file
	// overwrites any existing files with the same name
	// 
	// this may need to be overridden in extended classes in order
	// to insert default data into files at time of creation
	// 
	// args:	name of file to create
	// returns:	true if created file ok
	virtual bool create(const char* filename);
	
	// open existing file
	// 
	// args:	name of file to open
	// returns:	true if opened file ok
	virtual bool open(const char* filename);
	
	// closes file if open
	// returns error if no file currently open
	// 
	// returns:	true if closed ok
	virtual bool close(void);
	
	// checks if there is a file currently open
	// 
	// returns:	true if file is currently open
	virtual bool isOpen(void) const				{ return (fd != 0); }
	
	
	// get filename of open file
	// 
	// returns:	filename of open file
	//			"" if no file open
	virtual const char* getFilename(void) const	{ return filename;	}
};


#endif	// _GENERIC_FILE_H_

