// 
// DolFile.h
// 
// loser  -  may 2003
// 
// class for handling Dol files and the data they contain
// this class operates directly on the files using standard file io routines
// 

#ifndef _DOL_FILE_H_
#define _DOL_FILE_H_

#include "GenericFile.h"


class DolFile : public GenericFile
{
private:
	
	// sets the offset within the dol file that the given section starts at
	// 
	// args:	index of section
	//			offset of section
	void setTextOffset(u32 index, u32 offset);
	void setDataOffset(u32 index, u32 offset);
	
	// sets the address that the section loads to
	// 
	// args:	index of section
	//			address of section
	void setTextAddress(u32 index, u32 address);
	void setDataAddress(u32 index, u32 address);
	
	// sets the size of the given section in bytes
	// 
	// args:	index of section
	//			size of section
	void setTextSize(u32 index, u32 size);
	void setDataSize(u32 index, u32 size);
	
public:
	DolFile();
	~DolFile();
	
	// creates a new dol file
	// overwrites any existing file with the same filename
	// 
	// args:	filename of dol file to create
	// returns:	true if created file ok
	bool create(const char* filename);
	
	
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
	bool moveData(u32 moveOffset, s32 moveAmount);


	// checks if the given text section is valid
	// (a section is 'valid' if it has section info set for it)
	// 
	// args:	index of text section to check
	// returns:	true if valid
	//			false if invalid or incorrect index
	bool isTextSectionValid(u32 index) const;
	
	// checks if the given data section is valid
	// (a section is 'valid' if it has section info set for it)
	// 
	// args:	index of data section to check
	// returns:	true if valid
	//			false if invalid or incorrect index
	bool isDataSectionValid(u32 index) const;
	
	// gets the number of 'valid' text sections
	// 
	// returns:	0 or more = number of valid text sections
	//			ERROR_RETURN if error
	u32  getNumTextSections(void) const;
	
	// gets the number of 'valid' data sections
	// 
	// returns:	0 or more = number of valid data sections
	//			ERROR_RETURN if error
	u32  getNumDataSections(void) const;
	
	// get the maximum number of text/data sections possible in a dol file
	static u32 getMaxNumTextSections(void);
	static u32 getMaxNumDataSections(void);
	
	
	// gets the offset within the dol file that the given section starts at
	// 
	// args:	index of section
	// returns:	offset from start of file
	//			ERROR_RETURN if error
	u32  getTextOffset(u32 index) const;
	u32  getDataOffset(u32 index) const;
	
	// gets the address that the section loads to
	// 
	// args:	index of section
	// returns:	load address of section
	//			ERROR_RETURN if error
	u32  getTextAddress(u32 index) const;
	u32  getDataAddress(u32 index) const;
	
	// gets the size of the given section in bytes
	// 
	// args:	index of section
	// returns:	size in bytes of section
	//			ERROR_RETURN if error
	u32  getTextSize(u32 index) const;
	u32  getDataSize(u32 index) const;
	
	
	// gets the address of the entry point of the dol file
	// 
	// returns:	entry point address
	//			ERROR_RETURN if error
	u32  getEntryPoint(void) const;
	
	// gets the address and size of the bss area for the dol file
	// 
	// returns:	bss address/size
	//			ERROR_RETURN if error
	u32  getBssAddress(void) const;
	u32  getBssSize(void) const;
	
	// gets the lowest address that any of the sections
	// in the dol file get loaded to
	// (doesn't include BSS)
	// 
	// returns:	address of lowest section
	//			ERROR_RETURN if error
	u32  getLowestAddress(void) const;
	
	// gets the highest address that the last byte in any
	// of the sections in the dol file get loaded to
	// (doesn't include BSS)
	// 
	// returns:	address of lowest section
	//			ERROR_RETURN if error
	u32  getHighestAddress(void) const;
	
	
	// sets the address and size of the bss area for the dol file
	// 
	// args:	size/address of bss
	void setBssAddress(u32 address);
	void setBssSize(u32 size);
	
	// sets the address of the entry point of the dol file
	// 
	// args:	address of entry point
	void setEntryPoint(u32 address);
	
	
	// convert between file offsets and addresses
	// 
	// args:	offset/address
	// returns:	ERROR_RETURN if error
	//			otherwise offset/address
	u32  addressToOffset(u32 address) const;
	u32  offsetToAddress(u32 offset) const;
	
	
	// write section data out to file
	// 
	// args:	index of section
	//			file descriptor to write section data to
	// returns:	size of data written
	//			ERROR_RETURN if error
	u32  extractTextSection(u32 index, FILE* out_fd) const;
	u32  extractDataSection(u32 index, FILE* out_fd) const;
	
	// read section data in from file to a specified section
	// rounds up size of section to be a multiple of 32 bytes
	// 
	// if section with given index exists, it will be overwritten
	// 
	// args:	index of section
	//			file descriptor to read section data from
	//			address to load the section data to
	//			size of section data to insert
	// returns:	size of data written (rounded up to a multiple of 32 bytes)
	//			ERROR_RETURN if error
	u32  insertTextSection(u32 index, FILE* in_fd, u32 address, u32 size);
	u32  insertDataSection(u32 index, FILE* in_fd, u32 address, u32 size);
	
	// read section in from file to the next available section
	// rounds up size of section to be a multiple of 32 bytes
	// 
	// args:	file descriptor to read section data from
	//			address to load the section data to
	//			size of section data to insert
	// returns:	size of data written (rounded up to a multiple of 32 bytes)
	//			ERROR_RETURN if error
	u32  addTextSection(FILE* in_fd, u32 address, u32 size);
	u32  addDataSection(FILE* in_fd, u32 address, u32 size);
	
	// remove a text/data section from dol file
	// 
	// args:	index of section to remove
	// returns:	true if successfully removed
	bool removeTextSection(u32 index);
	bool removeDataSection(u32 index);
	
	// remove all text/data sections from dol file
	// 
	// returns:	true if successfully removed
	bool removeAllTextSections(void);
	bool removeAllDataSections(void);
	bool removeAllSections(void);
};


#endif	// _DOL_FILE_H_

