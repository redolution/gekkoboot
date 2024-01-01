// 
// DolFile.cpp
// 
// loser  -  may 2003
// 
// class for handling Dol files and the data they contain
// this class operates directly on the files using standard file io routines
// 

#include "DolFile.h"
#include <string.h>
#include "UtilFuncs.h"

// maximum number of text sections in dol file
#define DOL_NUM_TEXT	7

// maximum number of data sections in dol file
#define DOL_NUM_DATA	11

// dol file header
// note: sections should be on 32 byte boundaries
typedef struct
{
	u32 textOffset[DOL_NUM_TEXT];	// offset of text sections
	u32 dataOffset[DOL_NUM_DATA];	// offset of data sections
	
	u32 textAddress[DOL_NUM_TEXT];	// address of text sections
	u32 dataAddress[DOL_NUM_DATA];	// address of data sections
	
	u32 textSize[DOL_NUM_TEXT];		// size of text sections
	u32 dataSize[DOL_NUM_DATA];		// size of data sections
	
	u32 bssAddress;					// address of start of bss are in memory
	u32 bssSize;					// size of bss area
	u32 entryPoint;					// entry point of dol file (address)
	u32 zero[7];					// seems to be zeroed data
}
DolHeader;


DolFile::DolFile() : GenericFile()
{
}

DolFile::~DolFile()
{
}


// creates a new dol file
// overwrites any existing file with the same filename
// 
// args:	filename of dol file to create
// returns:	true if created file ok
bool DolFile::create(const char* filename)
{
	// create a generic file
	if(!GenericFile::create(filename))
		return false;
	
	// write an empty dol header to file
	DolHeader header;
	memset(&header, 0, sizeof(DolHeader));
	if(fwrite(&header, 1, sizeof(DolHeader), fd) != sizeof(DolHeader))
		return false;
	
	return isOpen();
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
// args:	offset within file to move data from
//			number of bytes to move data 'forward' by
//			(a negative value means to move data 'backwards')
// returns:	true if moved data ok
bool DolFile::moveData(u32 moveOffset, s32 moveAmount)
{
	// do generic move of data within file
	if(!GenericFile::moveData(moveOffset, moveAmount))
		return false;
	
	// update headers' offsets for any sections that come after the removed data
	for(s32 i=0; i<DOL_NUM_TEXT; i++)
	{
		if(isTextSectionValid(i))
		{
			if(getTextOffset(i) >= moveOffset)
				setTextOffset(i, getTextOffset(i) + moveAmount);
		}
	}
	for(s32 i=0; i<DOL_NUM_DATA; i++)
	{
		if(isDataSectionValid(i))
		{
			if(getDataOffset(i) >= moveOffset)
				setDataOffset(i, getDataOffset(i) + moveAmount);
		}
	}
	
	return true;
}


// checks if the given text section is valid
// (a section is 'valid' if it has section info set for it)
// 
// args:	index of text section to check
// returns:	true if valid
//			false if invalid or incorrect index
bool DolFile::isTextSectionValid(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return false;
	// make sure index is valid
	if(index >= DOL_NUM_TEXT)
		return false;
	
	// check sections 'offset' within file
	// to see if it is valid
	u32 offset = getTextOffset(index);
	if(offset == ERROR_RETURN || offset == 0)
		return false;
	return true;
}

// checks if the given data section is valid
// (a section is 'valid' if it has section info set for it)
// 
// args:	index of data section to check
// returns:	true if valid
//			false if invalid or incorrect index
bool DolFile::isDataSectionValid(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return false;
	// make sure index is valid
	if(index >= DOL_NUM_DATA)
		return false;
	
	// check sections 'offset' within file
	// to see if it is valid
	u32 offset = getDataOffset(index);
	if(offset == ERROR_RETURN || offset == 0)
		return false;
	return true;
}


// gets the number of 'valid' text sections
// 
// returns:	number of valid text sections
//			ERROR_RETURN if not open
u32  DolFile::getNumTextSections(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	u32 numTextSections = 0;
	for(s32 i=0; i<DOL_NUM_TEXT; i++)
	{
		if(isTextSectionValid(i))
			numTextSections++;
	}
	
	return numTextSections;
}

// gets the number of 'valid' data sections
// 
// returns:	number of valid data sections
//			ERROR_RETURN if not open
u32  DolFile::getNumDataSections(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	u32 numDataSections = 0;
	for(s32 i=0; i<DOL_NUM_DATA; i++)
	{
		if(isDataSectionValid(i))
			numDataSections++;
	}
	
	return numDataSections;
}


// get the maximum number of text/data sections possible in a dol file
u32  DolFile::getMaxNumTextSections(void)
{
	return DOL_NUM_TEXT;
}
u32  DolFile::getMaxNumDataSections(void)
{
	return DOL_NUM_DATA;
}


// gets the offset within the dol file that the given section starts at
// 
// args:	index of section
// returns:	offset from start of file
//			ERROR_RETURN if error
u32  DolFile::getTextOffset(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= DOL_NUM_TEXT)
		return ERROR_RETURN;
	
	// get offset of text section
	fseek(fd, index*4, SEEK_SET);
	u32 offset = ERROR_RETURN;
	fread(&offset, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, offset);
}
u32  DolFile::getDataOffset(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= DOL_NUM_DATA)
		return ERROR_RETURN;
	
	// get offset of data section
	fseek(fd, DOL_NUM_TEXT*4 + index*4, SEEK_SET);
	u32 offset = ERROR_RETURN;
	fread(&offset, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, offset);
}

// gets the address that the section loads to
// 
// args:	index of section
// returns:	load address of section
//			ERROR_RETURN if error
u32  DolFile::getTextAddress(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= DOL_NUM_TEXT)
		return ERROR_RETURN;
	
	// get address of text section
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA+index)*4, SEEK_SET);
	u32 address = ERROR_RETURN;
	fread(&address, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, address);
}
u32  DolFile::getDataAddress(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= DOL_NUM_DATA)
		return ERROR_RETURN;
	
	// get address of data section
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA+DOL_NUM_TEXT+index)*4, SEEK_SET);
	u32 address = ERROR_RETURN;
	fread(&address, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, address);
}

// gets the size of the given section in bytes
// 
// args:	index of section
// returns:	size in bytes of section
//			ERROR_RETURN if error
u32  DolFile::getTextSize(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= DOL_NUM_TEXT)
		return ERROR_RETURN;
	
	// get size of text section
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA)*4*2 + index*4, SEEK_SET);
	u32 size = ERROR_RETURN;
	fread(&size, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, size);
}
u32  DolFile::getDataSize(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= DOL_NUM_DATA)
		return ERROR_RETURN;
	
	// get size of text section
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA)*4*2 + (DOL_NUM_TEXT+index)*4, SEEK_SET);
	u32 size = ERROR_RETURN;
	fread(&size, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, size);
}

// gets the address of the entry point of the dol file
// 
// returns:	entry point address
//			ERROR_RETURN if error
u32  DolFile::getEntryPoint(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get entry point
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA)*4*3 + 8, SEEK_SET);
	u32 address = ERROR_RETURN;
	fread(&address, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, address);
}

// gets the address and size of the bss area for the dol file
// 
// returns:	bss address/size
//			ERROR_RETURN if error
u32  DolFile::getBssAddress(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get bss address
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA)*4*3, SEEK_SET);
	u32 address = ERROR_RETURN;
	fread(&address, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, address);
}
u32  DolFile::getBssSize(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get bss size
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA)*4*3 + 4, SEEK_SET);
	u32 size = ERROR_RETURN;
	fread(&size, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, size);
}


// gets the lowest address that any of the sections
// in the dol file get loaded to
// (doesn't include BSS)
// 
// returns:	address of lowest section
//			ERROR_RETURN if error
u32  DolFile::getLowestAddress(void) const
{
	u32 lowestAddress = ERROR_RETURN;
	
	// check all text sections
	for(u32 i=0; i<DOL_NUM_TEXT; i++)
	{
		if(isTextSectionValid(i))
		{
			u32 address = getTextAddress(i);
			// check if address is the lowest one yet
			if(address < lowestAddress || lowestAddress == ERROR_RETURN)
				lowestAddress = address;
		}
	}
	// check all data sections
	for(u32 i=0; i<DOL_NUM_DATA; i++)
	{
		if(isDataSectionValid(i))
		{
			u32 address = getDataAddress(i);
			// check if address is the lowest one yet
			if(address < lowestAddress || lowestAddress == ERROR_RETURN)
				lowestAddress = address;
		}
	}
	
	return lowestAddress;
}
// gets the highest address that the last byte in any
// of the sections in the dol file get loaded to
// (doesn't include BSS)
// 
// returns:	address of lowest section
//			ERROR_RETURN if error
u32  DolFile::getHighestAddress(void) const
{
	u32 highestAddress = ERROR_RETURN;
	
	// check all text sections
	for(u32 i=0; i<DOL_NUM_TEXT; i++)
	{
		if(isTextSectionValid(i))
		{
			u32 address = getTextAddress(i);
			// check if address is the lowest one yet
			if(address > highestAddress || highestAddress == ERROR_RETURN)
				highestAddress = address;
		}
	}
	// check all data sections
	for(u32 i=0; i<DOL_NUM_DATA; i++)
	{
		if(isDataSectionValid(i))
		{
			u32 address = getDataAddress(i);
			// check if address is the lowest one yet
			if(address > highestAddress || highestAddress == ERROR_RETURN)
				highestAddress = address;
		}
	}
	
	return highestAddress;
}


// sets the offset within the dol file that the given section starts at
// 
// args:	index of section
//			offset of section
void DolFile::setTextOffset(u32 index, u32 offset)
{
	// make sure file is open
	if(!isOpen())
		return;
	// make sure index is valid
	if(index >= DOL_NUM_TEXT)
		return;
	
	// set text offset
	offset = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, offset);
	fseek(fd, index*4, SEEK_SET);
	fwrite(&offset, 1, 4, fd);
}
void DolFile::setDataOffset(u32 index, u32 offset)
{
	// make sure file is open
	if(!isOpen())
		return;
	// make sure index is valid
	if(index >= DOL_NUM_DATA)
		return;
	
	// set data offset
	offset = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, offset);
	fseek(fd, DOL_NUM_TEXT*4 + index*4, SEEK_SET);
	fwrite(&offset, 1, 4, fd);
}

// sets the address that the section loads to
// 
// args:	index of section
//			address of section
void DolFile::setTextAddress(u32 index, u32 address)
{
	// make sure file is open
	if(!isOpen())
		return;
	// make sure index is valid
	if(index >= DOL_NUM_TEXT)
		return;
	
	// set text address
	address = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, address);
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA)*4 + index*4, SEEK_SET);
	fwrite(&address, 1, 4, fd);
}
void DolFile::setDataAddress(u32 index, u32 address)
{
	// make sure file is open
	if(!isOpen())
		return;
	// make sure index is valid
	if(index >= DOL_NUM_DATA)
		return;
	
	// set data address
	address = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, address);
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA)*4 + DOL_NUM_TEXT*4 + index*4, SEEK_SET);
	fwrite(&address, 1, 4, fd);
}

// sets the size of the given section in bytes
// 
// args:	index of section
//			size of section
void DolFile::setTextSize(u32 index, u32 size)
{
	// make sure file is open
	if(!isOpen())
		return;
	// make sure index is valid
	if(index >= DOL_NUM_TEXT)
		return;
	
	// set text size
	size = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, size);
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA)*4*2 + index*4, SEEK_SET);
	fwrite(&size, 1, 4, fd);
}
void DolFile::setDataSize(u32 index, u32 size)
{
	// make sure file is open
	if(!isOpen())
		return;
	// make sure index is valid
	if(index >= DOL_NUM_DATA)
		return;
	
	// set data size
	size = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, size);
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA)*4*2 + DOL_NUM_TEXT*4 + index*4, SEEK_SET);
	fwrite(&size, 1, 4, fd);
}

// sets the address of the entry point of the dol file
// 
// args:	address of entry point
void DolFile::setEntryPoint(u32 address)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// insert text section
	address = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, address);
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA)*4*3 + 8, SEEK_SET);
	fwrite(&address, 1, 4, fd);
}


// convert between file offsets and addresses
// 
// args:	offset/address
// returns:	ERROR_RETURN if error
//			otherwise offset/address
u32  DolFile::addressToOffset(u32 address) const
{
	// check through all text sections
	for(u32 i=0; i<DOL_NUM_TEXT; i++)
	{
		if(address >= getTextAddress(i) && address < (getTextAddress(i) + getTextSize(i)))
			return getTextOffset(i) + (address - getTextAddress(i));
	}
	// check through all data sections
	for(u32 i=0; i<DOL_NUM_DATA; i++)
	{
		if(address >= getDataAddress(i) && address < (getDataAddress(i) + getDataSize(i)))
			return getDataOffset(i) + (address - getDataAddress(i));
	}
	return ERROR_RETURN;
}
u32  DolFile::offsetToAddress(u32 offset) const
{
	// check through all text sections
	for(u32 i=0; i<DOL_NUM_TEXT; i++)
	{
		if(offset >= getTextOffset(i) && offset < (getTextOffset(i) + getTextSize(i)))
			return getTextAddress(i) + (offset - getTextOffset(i));
	}
	// check through all data sections
	for(u32 i=0; i<DOL_NUM_DATA; i++)
	{
		if(offset >= getDataOffset(i) && offset < (getDataOffset(i) + getDataSize(i)))
			return getDataAddress(i) + (offset - getDataOffset(i));
	}
	return ERROR_RETURN;
}


// sets the address and size of the bss area for the dol file
// 
// args:	size/address of bss
void DolFile::setBssAddress(u32 address)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set bss address
	address = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, address);
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA)*4*3, SEEK_SET);
	fwrite(&address, 1, 4, fd);
}
void DolFile::setBssSize(u32 size)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set bss size
	size = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, size);
	fseek(fd, (DOL_NUM_TEXT+DOL_NUM_DATA)*4*3 + 4, SEEK_SET);
	fwrite(&size, 1, 4, fd);
}



// write section data out to file
// 
// args:	index of section
//			file descriptor to write section data to
// returns:	size of data written
//			ERROR_RETURN if error
u32  DolFile::extractTextSection(u32 index, FILE* out_fd) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= DOL_NUM_TEXT)
		return ERROR_RETURN;
	
	// get section's offset and size
	u32 offset	= getTextOffset(index);
	u32 size	= getTextSize(index);
	
	// write out text section
	return writeOutData(offset, size, out_fd);
}
u32  DolFile::extractDataSection(u32 index, FILE* out_fd) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= DOL_NUM_DATA)
		return ERROR_RETURN;
	
	// get section's offset and size
	u32 offset	= getDataOffset(index);
	u32 size	= getDataSize(index);
	
	// write out data section
	return writeOutData(offset, size, out_fd);
}

// read section in from file to a specified section
// rounds up size of section to be a multiple of 32 bytes
// 
// args:	index of section
//			file descriptor to read section data from
//			address to load the section data to
//			size of section data to insert
// returns:	size of data written (rounded up to a multiple of 32 bytes)
//			ERROR_RETURN if error
u32  DolFile::insertTextSection(u32 index, FILE* in_fd, u32 address, u32 size)
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= DOL_NUM_TEXT)
		return ERROR_RETURN;
	
	// delete existing text section
	if(isTextSectionValid(index))
		removeTextSection(index);
	
	// get size of section as a multiple of 32 bytes
	u32 properSize = ((size+31)/32)*32;
	// get the offset of the section
	fseek(fd, 0, SEEK_END);
	u32 offset = ftell(fd);
	
	// add new section header
	setTextAddress(	index, address);
	setTextOffset(	index, offset);
	setTextSize(	index, properSize);
	
	// insert text section at end of file
	u32 ret = readInData(offset, size, in_fd);
	
	// insert padding bytes to make section size a multiple of 32 bytes
	if(properSize > size)
	{
		u32 padSize = properSize - size;
		u8* buffer = new u8[padSize];
		memset(buffer, 0, padSize);
		fwrite(buffer, 1, padSize, fd);
		delete[] buffer;
		if(ret != ERROR_RETURN)
			ret += (padSize);
	}
	return ret;
}
u32  DolFile::insertDataSection(u32 index, FILE* in_fd, u32 address, u32 size)
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= DOL_NUM_DATA)
		return ERROR_RETURN;
	
	// delete existing data section
	if(isDataSectionValid(index))
		removeDataSection(index);
	
	// get size of section as a multiple of 32 bytes
	u32 properSize = ((size+31)/32)*32;
	// get the offset of the section
	fseek(fd, 0, SEEK_END);
	u32 offset = ftell(fd);
	
	// add new section info to header
	setDataAddress(	index, address);
	setDataOffset(	index, offset);
	setDataSize(	index, properSize);
	
	// insert data section at end of file
	u32 ret = readInData(offset, size, in_fd);
	
	// insert padding bytes to make section size a multiple of 32 bytes
	if(properSize > size)
	{
		s32 padSize = properSize - size;
		u8* buffer = new u8[padSize];
		memset(buffer, 0, padSize);
		fwrite(buffer, 1, padSize, fd);
		delete[] buffer;
		if(ret != ERROR_RETURN)
			ret += (padSize);
	}
	return ret;
}

// read section in from file to the next available section
// rounds up size of section to be a multiple of 32 bytes
// 
// args:	file descriptor to read section data from
//			address to load the section data to
//			size of section data to insert
// returns:	size of data written (rounded up to a multiple of 32 bytes)
//			ERROR_RETURN if error
u32  DolFile::addTextSection(FILE* in_fd, u32 address, u32 size)
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get index of next free section
	if(getNumTextSections() == getMaxNumTextSections())
		return false;
	u32 index;
	for(index=0; index<DOL_NUM_TEXT; index++)
	{
		if(!isTextSectionValid(index))
			break;
	}
	
	// insert section into dol file
	return insertTextSection(index, in_fd, address, size);
}
u32  DolFile::addDataSection(FILE* in_fd, u32 address, u32 size)
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get index of next free section
	if(getNumDataSections() == getMaxNumDataSections())
		return false;
	u32 index;
	for(index=0; index<DOL_NUM_DATA; index++)
	{
		if(!isDataSectionValid(index))
			break;
	}
	
	// insert section into dol file
	return insertDataSection(index, in_fd, address, size);
}


// remove a text/data section from dol file
// 
// args:	index of section to remove
// returns:	true if successfully removed
bool DolFile::removeTextSection(u32 index)
{
	// make sure file is open
	if(!isOpen())
		return false;
	// check if text section is set/index is valid
	if(!isTextSectionValid(index))
		return false;
	
	// get section info to remove section data after deleting header
	u32 offset	= getTextOffset(index);
	u32 size	= getTextSize(index);
	
	// remove section header
	setTextAddress(index, 0);
	setTextOffset(index,  0);
	setTextSize(index,    0);
	
	// remove section data from file
	return (removeData(offset, size) != size);
}
bool DolFile::removeDataSection(u32 index)
{
	// make sure file is open
	if(!isOpen())
		return false;
	// check if data section is set/index is valid
	if(!isDataSectionValid(index))
		return false;
	
	// get section info to remove section data after deleting header
	u32 offset	= getDataOffset(index);
	u32 size	= getDataSize(index);
	
	// remove section header
	setDataAddress(index, 0);
	setDataOffset(index,  0);
	setDataSize(index,    0);
	
	// remove section data from file
	return (removeData(offset, size) != size);
}

// remove all text/data sections from dol file
// 
// returns:	true if successfully removed
bool DolFile::removeAllTextSections(void)
{
	bool ret = true;
	for(u32 i=0; i<DOL_NUM_TEXT; i++)
	{
		// only try to remove text section if it is set
		if(isTextSectionValid(i))
		{
			if(!removeTextSection(i))
				ret = false;
		}
	}
	return ret;
}
bool DolFile::removeAllDataSections(void)
{
	bool ret = true;
	for(u32 i=0; i<DOL_NUM_DATA; i++)
	{
		// only try to remove data section if it is set
		if(isDataSectionValid(i))
		{
			if(!removeDataSection(i))
				ret = false;
		}
	}
	return ret;
}
bool DolFile::removeAllSections(void)
{
	return (removeAllTextSections() && removeAllDataSections());
}


