// 
// GenericFile.cpp
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

#include "GenericFile.h"
#include "UtilFuncs.h"
#include <string.h>


// specify buffer size when creating file object
// uses 100kb by default
GenericFile::GenericFile(u32 bufferSize)
{
	this->bufferSize = bufferSize;
	
	fd = 0;
	filename = 0;
	setFilename("");
}
GenericFile::~GenericFile()
{
	// close this file if its open
	if(isOpen())
		close();
	// free filename
	if(filename)
		delete[] filename;
}
	

// set filename
void GenericFile::setFilename(const char* name)
{
	if(filename)
		delete[] filename;
	filename = new char[strlen(name) + 1];
	strcpy(filename, name);
	filename[strlen(name)] = 0;
}


// write out data from this file to output file
// 
// args:	offset in this file to start getting data from
//			size of data from this file to write out
//			file descriptor of file to write to
// returns:	size of data written to output file
//			ERROR_RETURN if error
u32  GenericFile::writeOutData(u32 writeOffset, u32 writeSize, FILE* out_fd) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// goto write offset in this file
	if(fseek(fd, writeOffset, SEEK_SET))
		return ERROR_RETURN;
	
	// create buffer
	u8 *buffer = new u8[bufferSize];
	
	// write out data
	u32 blockSize;		// size of current block being written
	u32 processedSize;	// size of blocks already written
	for(processedSize=0; processedSize<writeSize; )
	{
		// work out copy size
		if(writeSize - processedSize >= bufferSize)
			blockSize = bufferSize;
		else
			blockSize = writeSize - processedSize;
		// write out block of data
		if(fread( buffer, 1, blockSize, fd)		!= blockSize)
		{
			delete[] buffer;
			return ERROR_RETURN;
		}
		if(fwrite(buffer, 1, blockSize, out_fd)	!= blockSize)
		{
			delete[] buffer;
			return ERROR_RETURN;
		}
		processedSize += blockSize;
	}
	
	delete[] buffer;
	return processedSize;
}

// read in data from input file to this file
// 
// args:	offset in this file to start putting data at
//			size of data to read into this file
//			file descriptor of file to read from
// returns:	size of data read in from input file
//			ERROR_RETURN if error
u32  GenericFile::readInData(u32 readOffset, u32 readSize, FILE* in_fd)
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// make sure this file is big enough to contain offset
	// if its not big enough, append zero's to file until it is big enough
	fseek(fd, 0, SEEK_END);
	u8 zero = 0;
	while(ftell(fd) < (s32)readOffset)
		fwrite(&zero, 1, 1, fd);
	
	// goto read offset in this file
	if(fseek(fd, readOffset, SEEK_SET))
		return ERROR_RETURN;
	
	// create buffer
	u8 *buffer = new u8[bufferSize];
	
	// read in data
	u32 blockSize;		// size of current block being written
	u32 processedSize;	// size of blocks already written
	for(processedSize=0; processedSize<readSize; )
	{
		// work out copy size
		if(readSize-processedSize >= bufferSize)
			blockSize = bufferSize;
		else
			blockSize = readSize-processedSize;
		// read in block of data
		if(fread( buffer, 1, blockSize, in_fd)	!= blockSize)
		{
			delete[] buffer;
			return ERROR_RETURN;
		}
		if(fwrite(buffer, 1, blockSize, fd)		!= blockSize)
		{
			delete[] buffer;
			return ERROR_RETURN;
		}
		processedSize += blockSize;
	}
	
	delete[] buffer;
	return processedSize;
}

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
bool GenericFile::moveData(u32 moveOffset, s32 moveAmount)
{
	// make sure file is open
	if(!isOpen())
		return false;
	// moveAmount == 0, so no data to move
	if(moveAmount == 0)
		return true;
	
	// moving data to a location before the start
	// of the file is not allowed! :)
	if((s32)(moveOffset+moveAmount) < 0)
		return false;
	// moving data from a location after the end
	// of the file is not allowed either
	fseek(fd, 0, SEEK_END);
	if(moveOffset > (u32)ftell(fd))
		return false;
	
	// get the size of the data to be moved
	u32 moveSize = ftell(fd) - moveOffset;
	
	// moving data to a location after the end of the file
	// requires that first the file be enlarged
	fseek(fd, 0, SEEK_END);
	u8 padding = 0;
	while(ftell(fd) < ((s32)moveOffset+moveAmount))
		fwrite(&padding, 1, 1, fd);
	
	// move data within file
	u8 *buffer = new u8[bufferSize];
	u32 blockSize;
	s32 moveSourceOffset, moveDestOffset;
	for(u32 processedSize=0; processedSize<moveSize; )
	{
		// work out size of data block to move
		if(moveSize-processedSize >= bufferSize)
			blockSize = bufferSize;
		else
			blockSize = moveSize-processedSize;
		
		// goto start of data block to move
		// make sure data doesnt get overwritten before it gets moved
		if(moveAmount > 0)
		{
			// move from the back of the data
			moveSourceOffset = moveOffset+moveSize - blockSize-processedSize;
		}
		else
		{
			// move from the start of the data
			moveSourceOffset = moveOffset + processedSize;
		}
		if(fseek(fd, moveSourceOffset, SEEK_SET)	!= 0)
		{
			delete[] buffer;
			return false;
		}
		// read in block of data to move
		if(fread(buffer, 1, blockSize, fd)			!= blockSize)
		{
			delete[] buffer;
			return false;
		}
		// goto data block destination
		// make sure data doesnt get overwritten before it gets moved
		if(moveAmount > 0)
		{
			moveDestOffset = moveSourceOffset+moveAmount;
		}
		else
		{
			moveDestOffset = moveSourceOffset+moveAmount;
		}
		if(fseek(fd, moveDestOffset, SEEK_SET)		!= 0)
		{
			delete[] buffer;
			return false;
		}
		// write out block of data
		if(fwrite(buffer, 1, blockSize, fd)			!= blockSize)
		{
			delete[] buffer;
			return false;
		}
		
		processedSize += blockSize;
	}
	delete[] buffer;
	
	return true;
}

// inserts data into file
// 
// args:	offset in this file to start inserting data at
//			size of data to insert into this file
//			file descriptor of file to insert data from
// returns:	size of data inserted from input file
//			ERROR_RETURN if error
u32  GenericFile::insertData(u32 insertOffset, u32 insertSize, FILE* in_fd)
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// move existing data to create a space for adding new data into
	if(!moveData(insertOffset, insertSize))
		return ERROR_RETURN;
	
	// insert data at insert offset
	if(readInData(insertOffset, insertSize, in_fd) != insertSize)
		return ERROR_RETURN;
	
	return insertSize;
}

// remove data from file
// (updates section headers to point to correct offsets)
// 
// args:	offset in this file to start removing data from
//			size of data to remove from this file
// returns:	size of data removed from input file
//			ERROR_RETURN if error
u32  GenericFile::removeData(u32 removeOffset, u32 removeSize)
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// move existing data to 'cover up' the old info
	if(!moveData(removeOffset, -(s32)removeSize))
		return ERROR_RETURN;
	
	// set new end of file marker
	fseek(fd, 0, SEEK_END);
	u32 eofOffset = ftell(fd) - removeSize;
	char filename[256];
	strcpy(filename, getFilename());
	close();
	if(!SetNewEndOfFile(filename, eofOffset))
		return ERROR_RETURN;
	if(!open(filename))
		return ERROR_RETURN;
	
	return removeSize;
}


// creates a new file
// overwrites existing file with the given filename
// 
// this may need to be overridden in extended classes in order
// to insert default data into files at time of creation
// 
// args:	name of file to create
// returns:	true if created file ok
bool GenericFile::create(const char* filename)
{
	// close file if already open
	if(isOpen())
		close();
	
	// create file
	fd = fopen(filename, "w+b");
	setFilename(filename);
	
	return isOpen();
}

// opens an existing file
// returns an error if the file doesnt exist
// 
// args:	name of file to open
// returns:	true if opened file ok
bool GenericFile::open(const char* filename)
{
	// close file if already open
	if(isOpen())
		close();
	
	// open file
	fd = fopen(filename, "r+b");
	setFilename(filename);
	
	return isOpen();
}

// closes an open file
// returns an error if the file isnt open
// 
// returns:	true if closed ok
//			false if no file open
bool GenericFile::close(void)
{
	if(isOpen())
	{
		fclose(fd);
		fd = 0;
		setFilename("");
		return true;
	}
	return false;
}


