// 
// ElfFile.h
// 
// loser  -  may 2003
// 
// class for handling Elf files and the data they contain
// this class operates directly on the files using standard file io routines
// 

#ifndef _ELF_FILE_H_
#define _ELF_FILE_H_

#include "GenericFile.h"

// Elf defines

// elf file type
#define ET_NONE		0x0000		// no file type
#define ET_REL		0x0001		// relocatable file
#define ET_EXEC		0x0002		// executable file
#define ET_DYN		0x0003		// shared object file
#define ET_CORE		0x0004		// core file
// machine type for elf file
#define EM_NONE		0x0000		// no machine specified
#define EM_M32		0x0001		// AT&T WE 32100
#define EM_SPARC	0x0002		// SPARC
#define EM_386		0x0003		// Intel 80386
#define EM_68K		0x0004		// Motorola 68000
#define EM_88K		0x0005		// Motorola 88000
#define EM_860		0x0007		// Intel 80860
#define EM_MIPS		0x0008		// MIPS R3000
#define EM_PPC		0x0014		// PowerPC
// object size
#define ELFCLASSNONE	0		// invalid class
#define ELFCLASS32		1		// 32 bit objects
#define ELFCLASS64		2		// 64 bit objects
// data encoding
#define ELFDATANONE		0		// invalid data encoding
#define ELFDATA2LSB		1		// little endian
#define ELFDATA2MSB		2		// big endian

// section header defines
#define SHN_UNDEF		0		// undefined section index
// section header types
#define SHT_NULL		0		// undefined/inactive section
#define SHT_PROGBITS	1		// section contains program defined data
#define SHT_SYMTAB		2		// symbol table
#define SHT_STRTAB		3		// string table
#define SHT_RELA		4		// relocation entries with specific addends
#define SHT_HASH		5		// symbol hash table
#define SHT_DYNAMIC		6		// info for dynamic linking
#define SHT_NOTE		7		// 'note' data
#define SHT_NOBITS		8		// same as progbits, but takes up no file space
#define SHT_REL			9		// relocation entries without specific addends
#define SHT_SHLIB		10		// reserved
#define SHT_DYNSYM		11		// symbol table (minimal)
// section header flags
#define SHF_WRITE		1		// section's data is 'writable' during execution
#define SHF_ALLOC		2		// section contains memory space during execution
#define SHF_EXECINSTR	4		// section contains executable machine instructions

// program header types
#define PT_NULL			0		// program is not used
#define PT_LOAD			1		// program is loaded to memory
#define PT_DYNAMIC		2		// program contains dynamic linking info
// program header flags
#define PF_EXEC			1		// program section is executable
#define PF_WRITE		2		// program section is writable
#define PF_READ			4		// program section is readable


class ElfFile : public GenericFile
{
private:
	// 
	// Elf Functions
	// 
	
	// set the size of the elf's objects
	// 
	// args:	object size (0 or 32 or 64)
	void setObjectSize(u8 size);
	// sets the endian format of the elf file
	// 
	// args:	ELFDATA2LSB or ELFDATA2MSB or ELFDATANONE
	void setEndianFormat(u8 endian);
	// set the size fo the elf header
	void setElfHeaderSize(u16 size);
	
	// set the offset/size/number of the program header table in elf
	void setProgramHeadersOffset(u32 offset);
	void setProgramHeadersSize(u16 size);
	void setNumProgramHeaders(u16 num);
	// set the offset/size/number of the section header table in elf
	void setSectionHeadersOffset(u32 offset);
	void setSectionHeadersSize(u16 size);
	void setNumSectionHeaders(u16 num);
	// set the index of the section header that contains the string table
	void setStringTableSectionIndex(u16 index);
	
	// get the offset/size/number of the program header table in elf
	u32  getProgramHeaderOffset(u16 index)	const;
	u32  getProgramHeadersOffset(void)	const;
	u32  getProgramHeadersSize(void)	const;
	u32  getNumProgramHeaders(void)		const;
	// get the offset/size/number of the section header table in elf
	u32  getSectionHeaderOffset(u16 index)	const;
	u32  getSectionHeadersOffset(void)	const;
	u32  getSectionHeadersSize(void)	const;
	u32  getNumSectionHeaders(void)		const;
	// get the index of the section header that contains the string table
	u32  getStringTableSectionIndex(void) const;	
	
	
	// 
	// Program/Section Functions
	// 
	
	// sets the offset within the elf file that the given section starts at
	// 
	// args:	index of section
	//			offset of section
	void setProgramOffset(u32 index, u32 offset);
	void setSectionOffset(u32 index, u32 offset);
	// sets the address that the section loads to
	// 
	// args:	index of section
	//			address of section
	void setProgramAddress(u32 index, u32 address);
	void setSectionAddress(u32 index, u32 address);
	// sets the size of the given section in bytes
	// 
	// args:	index of section
	//			size of section
	void setProgramSize(u32 index, u32 size);
	void setSectionSize(u32 index, u32 size);
	
	// add a new string to the string table
	// 
	// args:	string to add to table
	// returns:	string offset of new string in table
	//			ERROR_RETURN if error
	u32  addStringToStringTable(const char *string);
	// get a string from the string table
	// string buffer must not be allocated before passing to method
	// it will get allocated inside method, then must be deleted outside method
	// 
	// args:	offset of string in table
	//			pointer to create buffer for holding string
	// returns:	size of allocated buffer
	//			ERROR_RETURN if error
	u32  getStringFromStringTable(u32 stringOffset, char*& string) const;
	
	// get the offset in the string table of the section name
	// 
	// args:	index of section
	// returns:	offset of name string inside string table
	//			ERROR_RETURN if error
	u32  getSectionNameOffset(u32 index) const;

	
public:
	ElfFile();
	~ElfFile();
	
	// create a new elf file
	// overwrites any existing files
	// specifies little endian by default (ELFDATA2LSB)
	// 
	// args:	filename of elf file to create
	// returns:	true if opened file ok
	bool create(const char* filename);
	// create new elf file
	// overwrites any existing files
	// 
	// args:	filename of elf file to create
	//			endian format of elf file (ELFDATA2LSB or ELFDATA2MSB or ELFDATANONE)
	// returns:	true if opened file ok
	bool create(const char* filename, u32 endian);
	
	
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
	
	
	// sets the type of elf file
	// 
	// args:	ET_??? (eg ET_EXEC)
	void setElfType(u16 type);
	// sets the machine type
	// 
	// args:	EM_??? (eg EM_MIPS)
	void setMachineType(u16 type);
	// set the entry point of the elf
	void setEntryPoint(u32 address);
	// set the elf flags
	void setFlags(u32 flags);
	
	
	// get the size of the elf's objects
	// 
	// returns:	object size (32 or 64)
	//			0 if error
	u32  getObjectSize(void) const;
	// gets the endian format of the elf file
	// 
	// returns:	ELFDATA2LSB or ELFDATA2MSB or ELFDATANONE
	u32  getEndianFormat(void) const;
	// gets the type of elf file
	// 
	// returns:	ET_??? (eg ET_EXEC)
	u32  getElfType(void) const;
	// gets the machine type
	// 
	// returns:	EM_??? (eg EM_MIPS)
	u32  getMachineType(void) const;
	// get the entry point of the elf
	u32  getEntryPoint(void) const;
	// get the elf flags
	u32  getFlags(void) const;
	// get the size fo the elf header
	u32  getElfHeaderSize(void) const;
	
	// get the number of program/sections in elf file
	// (same as the number of headers)
	// 
	// returns:	number of programs/sections
	u32  getNumPrograms(void) const;
	u32  getNumSections(void) const;
	
	// checks the size of the objects in elf file
	bool is32Bit(void) const;
	bool is64Bit(void) const;
	
	// checks if big/litle endian
	bool isLittleEndian(void) const;
	bool isBigEndian(void) const;
	
	// checks the type of elf file
	bool isTypeNone(void) const;
	bool isTypeRelocatable(void) const;
	bool isTypeExecutable(void) const;
	bool isTypeShared(void) const;
	bool isTypeCore(void) const;
	
	// check machine type
	bool isMips(void) const;
	bool isPpc(void) const;
	bool isIntel386(void) const;
	bool is68000(void) const;
	
	
	// gets the lowest address that any of the sections
	// in the elf file get loaded to
	// 
	// returns:	address of lowest section
	//			ERROR_RETURN if error
	u32  getLowestAddress(void) const;
	// gets the highest address that the last byte in any
	// of the sections in the elf file get loaded to
	// 
	// returns:	address of lowest section
	//			ERROR_RETURN if error
	u32  getHighestAddress(void) const;
	
	// convert between file offsets and addresses
	// 
	// args:	offset/address
	// returns:	ERROR_RETURN if error
	//			otherwise offset/address
	u32  addressToOffset(u32 address) const;
	u32  offsetToAddress(u32 offset ) const;
	
	
	// add program/section header
	// (updates all header offsets)
	// 
	// args:	load address of the program/section data
	//			size of program/section data
	//			offset of program/section data in file
	//			type of program/section
	//			flags for program/section
	//			alignment of program/section
	//			name of section
	// returns:	true if successful
	bool addProgramHeader(u32 address, u32 size, u32 offset=0, u32 type=0, u32 flags=0, u32 alignment=1);
	bool addSectionHeader(u32 address, u32 size, const char* name, u32 offset=0, u32 type=0, u32 flags=0, u32 alignment=1);
	
	// remove an existing program/section header
	// (updates all header offsets)
	// 
	// args:	index of program/section header to remove
	// returns:	true if successfully removed
	bool removeProgramHeader(u32 index);
	bool removeSectionHeader(u32 index);
	
	
	// write program/section data out to file
	// 
	// args:	index of program/section
	//			file descriptor to write program/section data to
	// returns:	size of data written
	//			ERROR_RETURN if error
	u32  extractProgramData(u32 index, FILE* out_fd) const;
	u32  extractSectionData(u32 index, FILE* out_fd) const;
	
	// read program/section in from file to the next available program/section
	// 
	// args:	file descriptor to read program/section data from
	//			address to load the program/section data to
	//			size of program/section data to insert
	//			type of program/section
	//			flags for program/section
	//			alignment of program/section
	//			name of section
	// returns:	size of program/section data written
	//			ERROR_RETURN if error
	u32  addProgram(FILE* in_fd, u32 address, u32 size, u32 type=0, u32 flags=0, u32 alignment=4);
	u32  addSection(FILE* in_fd, u32 address, u32 size, const char* name, u32 type=0, u32 flags=0, u32 alignment=4);
	
	// remove a program/section from dol file
	// (will only remove program/section data if no other
	//  references to it still exist in elf file)
	// 
	// args:	index of program/section to remove
	// returns:	true if successfully removed
	bool removeProgram(u32 index);
	bool removeSection(u32 index);
	
	// remove all programs/sections from dol file
	// 
	// returns:	true if successfully removed
	bool removeAllPrograms(void);
	bool removeAllSections(void);
	bool removeAllProgramsAndSections(void);


	// gets the offset within the elf file that the given section starts at
	// 
	// args:	index of section
	// returns:	offset of section
	//			ERROR_RETURN if error
	u32  getProgramOffset(u32 index) const;
	u32  getSectionOffset(u32 index) const;
	// gets the address that the section loads to
	// 
	// args:	index of section
	// returns:	address of section
	//			ERROR_RETURN if error
	u32  getProgramAddress(u32 index) const;
	u32  getSectionAddress(u32 index) const;
	// gets the size of the given section in bytes
	// 
	// args:	index of section
	// returns:	size of section
	//			ERROR_RETURN if error
	u32  getProgramSize(u32 index) const;
	u32  getSectionSize(u32 index) const;
	
	// gets the type of program/section
	// 
	// args:	index of section
	// returns:	type of section
	//			ERROR_RETURN if error
	u32  getProgramType(u32 index) const;
	u32  getSectionType(u32 index) const;
	
	// gets the program/section flags
	// 
	// args:	index of section
	// returns:	flags for section
	//			ERROR_RETURN if error
	u32  getProgramFlags(u32 index) const;
	u32  getSectionFlags(u32 index) const;
	

	// get name of the section
	// string buffer must not be allocated before passing to method
	// it will get allocated inside method, then must be deleted outside method
	// 
	// args:	section index
	//			pointer to create buffer for holding name
	// returns:	size of allocated buffer
	//			ERROR_RETURN if error
	u32  getSectionName(u32 index, char*& name) const;
	
	
	
	
/*	
	// read program/section in from file to a specified program/section
	// 
	// args:	index of program/section
	//			file descriptor to read program/section data from
	//			address to load the program/section data to
	//			size of program/section data to insert
	// returns:	size of data written
	//			ERROR_RETURN if error
	u32  replaceProgram(u32 index, FILE* in_fd, u32 address, u32 size);
	u32  replaceSection(u32 index, FILE* in_fd, u32 address, u32 size);
*/	
};


#endif	// _ELF_FILE_H_



