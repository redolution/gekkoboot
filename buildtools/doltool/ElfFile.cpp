// 
// ElfFile.cpp
// 
// loser  -  may 2003
// 
// class for handling Elf files and the data they contain
// this class operates directly on the files using standard file io routines
// 

#include "ElfFile.h"
#include <string.h>


// typedefs for elf values
typedef u32	Elf32_Addr;
typedef u16	Elf32_Half;
typedef u8	Elf32_Byte;
typedef u32	Elf32_Off;
typedef u32	Elf32_Sword;
typedef u32	Elf32_Word;

// size of identification section
#define EI_NIDENT	16

// Elf File Header
typedef struct
{
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half e_type;
	Elf32_Half e_machine;
	Elf32_Word e_version;
	Elf32_Addr e_entry;
	Elf32_Off  e_phoff;
	Elf32_Off  e_shoff;
	Elf32_Word e_flags;
	Elf32_Half e_ehsize;
	Elf32_Half e_phentsize;
	Elf32_Half e_phnum;
	Elf32_Half e_shentsize;
	Elf32_Half e_shnum;
	Elf32_Half e_shstrndx;
}
Elf32_Ehdr;


// Elf Program Header
typedef struct
{
	Elf32_Word	p_type;
	Elf32_Off	p_offset;
	Elf32_Addr	p_vaddr;
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;
	Elf32_Word	p_memsz;
	Elf32_Word	p_flags;
	Elf32_Word	p_align;
}
Elf32_Phdr;


// Elf Section Header
typedef struct
{
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
}
Elf32_Shdr;


ElfFile::ElfFile() : GenericFile()
{
}

ElfFile::~ElfFile()
{
}


// create a new elf file
// overwrites any existing files
// specifies little endian by default
// 
// args:	filename of elf file to create
// returns:	true if opened file ok
bool ElfFile::create(const char* filename)
{
	return create(filename, ELFDATA2LSB);
}

// create new elf file
// overwrites any existing files
// 
// args:	filename of elf file to create
//			endian format of elf file (ELFDATA2LSB or ELFDATA2MSB or ELFDATANONE)
// returns:	true if opened file ok
bool ElfFile::create(const char* filename, u32 endian)
{
	// create file
	if(!GenericFile::create(filename))
		return false;
	
	// setup new elf header
	Elf32_Ehdr header;
	memset(&header, 0, sizeof(Elf32_Ehdr));
	header.e_ident[0]	= 0x7F;
	header.e_ident[1]	= 'E';
	header.e_ident[2]	= 'L';
	header.e_ident[3]	= 'F';
	header.e_ident[4]	= ELFCLASS32;	// 32 bit machine
	header.e_ident[5]	= endian;		// endian
	header.e_ident[6]	= 1;			// version
	header.e_type		= TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != endian, ET_EXEC);
	header.e_machine	= TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != endian, EM_NONE);
	header.e_version	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, 1);
	header.e_entry		= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, 0);
	header.e_phoff		= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, 0);
	header.e_shoff		= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, sizeof(Elf32_Ehdr));
	header.e_flags		= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, 0);
	header.e_ehsize		= TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != endian, sizeof(Elf32_Ehdr));
	header.e_phentsize	= TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != endian, sizeof(Elf32_Phdr));
	header.e_phnum		= TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != endian, 0);
	header.e_shentsize	= TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != endian, sizeof(Elf32_Shdr));
	header.e_shnum		= TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != endian, 1);
	header.e_shstrndx	= TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != endian, 0);
	// write elf header to file
	fwrite(&header, 1, sizeof(Elf32_Ehdr), fd);
	
	// setup first section header (the 'null' section header)
	Elf32_Shdr sectionHeader;
	memset(&sectionHeader, 0, sizeof(Elf32_Shdr));
	// write section header to file
	fwrite(&sectionHeader, 1, sizeof(Elf32_Shdr), fd);
	
	// setup string table by inserting a string into the table
	addStringToStringTable("");
	
	return isOpen();
}


// set the size of the elf's objects
// 
// args:	object size (0 or 32 or 64)
void ElfFile::setObjectSize(u8 size)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set object size
	fseek(fd, 4, SEEK_SET);
	u8 elfSize = 0;
	if(size == 32)
		elfSize = ELFCLASS32;
	else if(size == 64)
		elfSize = ELFCLASS64;
	else
		elfSize = 0;
	fwrite(&elfSize, 1, 1, fd);
}

// sets the endian format of the elf file
// 
// args:	ELFDATA2LSB or ELFDATA2MSB or ELFDATANONE
void ElfFile::setEndianFormat(u8 endian)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set endian format
	fseek(fd, 5, SEEK_SET);
	fwrite(&endian, 1, 1, fd);
}

// sets the type of elf file
// 
// args:	ET_??? (eg ET_EXEC)
void ElfFile::setElfType(u16 type)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set elf type
	type = TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), type);
	fseek(fd, 16, SEEK_SET);
	fwrite(&type, 1, 2, fd);
}

// sets the machine type
// 
// args:	EM_??? (eg EM_MIPS)
void ElfFile::setMachineType(u16 type)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set machine type
	type = TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), type);
	fseek(fd, 18, SEEK_SET);
	fwrite(&type, 1, 2, fd);
}

// set the entry point of the elf
void ElfFile::setEntryPoint(u32 address)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set entry point
	address = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), address);
	fseek(fd, 24, SEEK_SET);
	fwrite(&address, 1, 4, fd);
}

// set the elf flags
void ElfFile::setFlags(u32 flags)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set flags
	flags = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), flags);
	fseek(fd, 36, SEEK_SET);
	fwrite(&flags, 1, 4, fd);
}

// set the size fo the elf header
void ElfFile::setElfHeaderSize(u16 size)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set elf header size
	size = TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), size);
	fseek(fd, 40, SEEK_SET);
	fwrite(&size, 1, 2, fd);
}


// get the size of the elf's objects
// 
// returns:	object size (32 or 64)
//			ERROR_RETURN if error
u32  ElfFile::getObjectSize(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get object size
	fseek(fd, 4, SEEK_SET);
	u8 size = 0;
	fread(&size, 1, 1, fd);
	if(		size == ELFCLASS32)
		return 32;
	else if(size == ELFCLASS64)
		return 64;
	return 0;
}

// gets the endian format of the elf file
// 
// returns:	ELFDATA2LSB or ELFDATA2MSB or ELFDATANONE
//			ERROR_RETURN if error
u32  ElfFile::getEndianFormat(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get endian format
	fseek(fd, 5, SEEK_SET);
	u8 endian = 0;
	fread(&endian, 1, 1, fd);
	return endian;
}

// gets the type of elf file
// 
// returns:	ET_??? (eg ET_EXEC)
u32  ElfFile::getElfType(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get elf type
	fseek(fd, 16, SEEK_SET);
	u16 type = 0;
	fread(&type, 1, 2, fd);
	return TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), type);
}

// gets the machine type
// 
// returns:	EM_??? (eg EM_MIPS)
u32  ElfFile::getMachineType(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get machine type
	fseek(fd, 18, SEEK_SET);
	u16 type = 0;
	fread(&type, 1, 2, fd);
	return TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), type);
}

// get the entry point of the elf
u32  ElfFile::getEntryPoint(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get entry point
	fseek(fd, 24, SEEK_SET);
	u32 address = 0;
	fread(&address, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), address);
}

// get the elf flags
u32  ElfFile::getFlags(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get flags
	fseek(fd, 36, SEEK_SET);
	u32 flags = 0;
	fread(&flags, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), flags);
}

// get the size fo the elf header
u32  ElfFile::getElfHeaderSize(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get elf header size
	fseek(fd, 40, SEEK_SET);
	u16 size = 0;
	fread(&size, 1, 2, fd);
	return TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), size);
}


// checks the size of the objects in elf file
bool ElfFile::is32Bit(void) const
{
	return (getObjectSize() == ELFCLASS32);
}
bool ElfFile::is64Bit(void) const
{
	return (getObjectSize() == ELFCLASS64);
}

// checks if big/litle endian
bool ElfFile::isLittleEndian(void) const
{
	return (getEndianFormat() == ELFDATA2LSB);
}
bool ElfFile::isBigEndian(void) const
{
	return (getEndianFormat() == ELFDATA2MSB);
}

// checks the type of elf file
bool ElfFile::isTypeNone(void) const
{
	return (getElfType() == ET_NONE);
}
bool ElfFile::isTypeRelocatable(void) const
{
	return (getElfType() == ET_REL);
}
bool ElfFile::isTypeExecutable(void) const
{
	return (getElfType() == ET_EXEC);
}
bool ElfFile::isTypeShared(void) const
{
	return (getElfType() == ET_DYN);
}
bool ElfFile::isTypeCore(void) const
{
	return (getElfType() == ET_CORE);
}

// check machine type
bool ElfFile::isMips(void) const
{
	return (getMachineType() == EM_MIPS);
}
bool ElfFile::isPpc(void) const
{
	return (getMachineType() == EM_PPC);
}
bool ElfFile::isIntel386(void) const
{
	return (getMachineType() == EM_386);
}
bool ElfFile::is68000(void) const
{
	return (getMachineType() == EM_68K);
}


// set the offset/size/number of the program header table in elf
void ElfFile::setProgramHeadersOffset(u32 offset)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set offset of program headers
	offset = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), offset);
	fseek(fd, 28, SEEK_SET);
	fwrite(&offset, 1, 4, fd);
}
void ElfFile::setProgramHeadersSize(u16 size)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set size of program headers
	size = TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), size);
	fseek(fd, 42, SEEK_SET);
	fwrite(&size, 1, 2, fd);
}
void ElfFile::setNumProgramHeaders(u16 num)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set number of program headers
	num = TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), num);
	fseek(fd, 44, SEEK_SET);
	fwrite(&num, 1, 2, fd);
}

// set the offset/size/number of the section header table in elf
void ElfFile::setSectionHeadersOffset(u32 offset)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set offset of section headers
	offset = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), offset);
	fseek(fd, 32, SEEK_SET);
	fwrite(&offset, 1, 4, fd);
}
void ElfFile::setSectionHeadersSize(u16 size)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set size of section headers
	size = TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), size);
	fseek(fd, 46, SEEK_SET);
	fwrite(&size, 1, 2, fd);
}
void ElfFile::setNumSectionHeaders(u16 num)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// set number of section headers
	num = TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), num);
	fseek(fd, 48, SEEK_SET);
	fwrite(&num, 1, 2, fd);
}
// set the index of the section header that contains the string table
void ElfFile::setStringTableSectionIndex(u16 index)
{
	// make sure file is open
	if(!isOpen())
		return;
	
	// get index of section that contains string table
	index = TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), index);
	fseek(fd, 50, SEEK_SET);
	fwrite(&index, 1, 2, fd);
}


// get the offset/size/number of the program header table in elf
u32  ElfFile::getProgramHeaderOffset(u16 index)	const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumProgramHeaders())
		return ERROR_RETURN;
	
	// get offset of program headers
	u32 offset = getProgramHeadersOffset();
	if(offset == ERROR_RETURN || offset == 0)
		return ERROR_RETURN;
	// get offset of specific program header
	u32 sizeOffset = getProgramHeadersSize();
	if(sizeOffset == 0 || sizeOffset == ERROR_RETURN)
		return ERROR_RETURN;
	
	// return specific header offset
	return offset + sizeOffset*index;
}
u32  ElfFile::getProgramHeadersOffset(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get offset of program headers
	fseek(fd, 28, SEEK_SET);
	u32 offset = 0;
	fread(&offset, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), offset);
}
u32  ElfFile::getProgramHeadersSize(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get size of program headers
	fseek(fd, 42, SEEK_SET);
	u16 size = 0;
	fread(&size, 1, 2, fd);
	return TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), size);
}
u32  ElfFile::getNumProgramHeaders(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get number of program headers
	fseek(fd, 44, SEEK_SET);
	u16 num = 0;
	fread(&num, 1, 2, fd);
	return TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), num);
}

// get the offset/size/number of the section header table in elf
u32  ElfFile::getSectionHeaderOffset(u16 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return ERROR_RETURN;
	
	// get offset of section headers
	u32 offset = getSectionHeadersOffset();
	if(offset == ERROR_RETURN || offset == 0)
		return ERROR_RETURN;
	// get offset of specific section header
	u32 sizeOffset = getSectionHeadersSize();
	if(sizeOffset == 0 || sizeOffset == ERROR_RETURN)
		return ERROR_RETURN;
	
	// return specific header offset
	return offset + sizeOffset*index;
}
u32  ElfFile::getSectionHeadersOffset(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get offset of section headers
	fseek(fd, 32, SEEK_SET);
	u32 offset = 0;
	fread(&offset, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), offset);
}
u32  ElfFile::getSectionHeadersSize(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get size of section headers
	fseek(fd, 46, SEEK_SET);
	u16 size = 0;
	fread(&size, 1, 2, fd);
	return TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), size);
}
u32  ElfFile::getNumSectionHeaders(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get number of section headers
	fseek(fd, 48, SEEK_SET);
	u16 num = 0;
	fread(&num, 1, 2, fd);
	return TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), num);
}
// get the index of the section header that contains the string table
u32  ElfFile::getStringTableSectionIndex(void) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get index of section that contains string table
	fseek(fd, 50, SEEK_SET);
	u16 index = 0;
	fread(&index, 1, 2, fd);
	return TEST_CHANGE_ENDIAN_16BIT(MY_ENDIAN != getEndianFormat(), index);
}


// sets the offset within the elf file that the given section starts at
// 
// args:	index of section
//			offset of section
void ElfFile::setProgramOffset(u32 index, u32 offset)
{
	// make sure file is open
	if(!isOpen())
		return;
	// make sure index is valid
	if(index >= getNumProgramHeaders())
		return;
	
	// get program header offset
	u32 headerOffset = getProgramHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return;
	// goto program offset value
	offset = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), offset);
	fseek(fd, headerOffset+4, SEEK_SET);
	fwrite(&offset, 1, 4, fd);
}
void ElfFile::setSectionOffset(u32 index, u32 offset)
{
	// make sure file is open
	if(!isOpen())
		return;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return;
	
	// get section header offset
	u32 headerOffset = getSectionHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return;
	// goto section offset value
	offset = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), offset);
	fseek(fd, headerOffset+16, SEEK_SET);
	fwrite(&offset, 1, 4, fd);
}
// sets the address that the section loads to
// 
// args:	index of section
//			address of section
void ElfFile::setProgramAddress(u32 index, u32 address)
{
	// make sure file is open
	if(!isOpen())
		return;
	// make sure index is valid
	if(index >= getNumProgramHeaders())
		return;
	
	// get program header offset
	u32 headerOffset = getProgramHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return;
	// goto program address value
	address = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), address);
	fseek(fd, headerOffset+8, SEEK_SET);
	// virtual address
	fwrite(&address, 1, 4, fd);
	// physical address
	fwrite(&address, 1, 4, fd);
}
void ElfFile::setSectionAddress(u32 index, u32 address)
{
	// make sure file is open
	if(!isOpen())
		return;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return;
	
	// get section header offset
	u32 headerOffset = getSectionHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return;
	// goto section address value
	address = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), address);
	fseek(fd, headerOffset+12, SEEK_SET);
	fwrite(&address, 1, 4, fd);
}
// sets the size of the given section in bytes
// 
// args:	index of section
//			size of section
void ElfFile::setProgramSize(u32 index, u32 size)
{
	// make sure file is open
	if(!isOpen())
		return;
	// make sure index is valid
	if(index >= getNumProgramHeaders())
		return;
	
	// get program header offset
	u32 headerOffset = getProgramHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return;
	// goto program address value
	size = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), size);
	fseek(fd, headerOffset+16, SEEK_SET);
	// file size
	fwrite(&size, 1, 4, fd);
	// memsize
	fwrite(&size, 1, 4, fd);
}
void ElfFile::setSectionSize(u32 index, u32 size)
{
	// make sure file is open
	if(!isOpen())
		return;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return;
	
	// get section header offset
	u32 headerOffset = getSectionHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return;
	// goto section address value
	size = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), size);
	fseek(fd, headerOffset+20, SEEK_SET);
	fwrite(&size, 1, 4, fd);
}
	
// gets the offset within the elf file that the given section starts at
// 
// args:	index of section
//			offset of section
u32  ElfFile::getProgramOffset(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumProgramHeaders())
		return ERROR_RETURN;
	
	// get program header offset
	u32 headerOffset = getProgramHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return ERROR_RETURN;
	// goto program offset value
	fseek(fd, headerOffset+4, SEEK_SET);
	u32 offset = 0;
	fread(&offset, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), offset);
}
u32  ElfFile::getSectionOffset(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return ERROR_RETURN;
	
	// get section header offset
	u32 headerOffset = getSectionHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return ERROR_RETURN;
	// goto section offset value
	u32 offset = 0;
	fseek(fd, headerOffset+16, SEEK_SET);
	fread(&offset, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), offset);
}
// gets the address that the section loads to
// 
// args:	index of section
//			address of section
u32  ElfFile::getProgramAddress(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumProgramHeaders())
		return ERROR_RETURN;
	
	// get program header offset
	u32 headerOffset = getProgramHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return ERROR_RETURN;
	// goto program offset value
	fseek(fd, headerOffset+8, SEEK_SET);
	u32 address = 0;
	fread(&address, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), address);
}
u32  ElfFile::getSectionAddress(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return ERROR_RETURN;
	
	// get section header offset
	u32 headerOffset = getSectionHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return ERROR_RETURN;
	// goto section offset value
	u32 address = 0;
	fseek(fd, headerOffset+12, SEEK_SET);
	fread(&address, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), address);
}
// gets the size of the given section in bytes
// 
// args:	index of section
//			size of section
u32  ElfFile::getProgramSize(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumProgramHeaders())
		return ERROR_RETURN;
	
	// get program header offset
	u32 headerOffset = getProgramHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return ERROR_RETURN;
	// goto program offset value
	fseek(fd, headerOffset+16, SEEK_SET);
	u32 size = 0;
	fread(&size, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), size);
}
u32  ElfFile::getSectionSize(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return ERROR_RETURN;
	
	// get section header offset
	u32 headerOffset = getSectionHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return ERROR_RETURN;
	// goto section offset value
	u32 size = 0;
	fseek(fd, headerOffset+20, SEEK_SET);
	fread(&size, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), size);
}

// get the number of program/sections in elf file
// (same as the number of headers)
// 
// returns:	number of programs/sections
u32  ElfFile::getNumPrograms(void) const
{
	return getNumProgramHeaders();
}
u32  ElfFile::getNumSections(void) const
{
	return getNumSectionHeaders();
}


// gets the lowest address that any of the sections
// in the elf file get loaded to
// 
// returns:	address of lowest section
//			ERROR_RETURN if error
u32  ElfFile::getLowestAddress(void) const
{
	u32 lowestAddress = ERROR_RETURN;
	
	// check all programs
	for(u32 i=0; i<getNumProgramHeaders(); i++)
	{
		u32 address = getProgramAddress(i);
		// check if address is the lowest one yet
		if(address < lowestAddress || lowestAddress == ERROR_RETURN)
			lowestAddress = address;
	}
	// check all sections
	for(u32 i=0; i<getNumSectionHeaders(); i++)
	{
		u32 address = getSectionAddress(i);
		// check if address is the lowest one yet
		if(address < lowestAddress || lowestAddress == ERROR_RETURN)
			lowestAddress = address;
	}
	
	return lowestAddress;
}

// gets the highest address that the last byte in any
// of the sections in the elf file get loaded to
// 
// returns:	address of lowest section
//			ERROR_RETURN if error
u32  ElfFile::getHighestAddress(void) const
{
	u32 highestAddress = ERROR_RETURN;
	
	// check all programs
	for(u32 i=0; i<getNumProgramHeaders(); i++)
	{
		u32 address = getProgramAddress(i);
		// check if address is the lowest one yet
		if(address > highestAddress || highestAddress == ERROR_RETURN)
			highestAddress = address;
	}
	// check all sections
	for(u32 i=0; i<getNumSectionHeaders(); i++)
	{
		u32 address = getSectionAddress(i);
		// check if address is the lowest one yet
		if(address > highestAddress || highestAddress == ERROR_RETURN)
			highestAddress = address;
	}
	
	return highestAddress;
}


// convert between file offsets and addresses
// 
// args:	offset/address
// returns:	ERROR_RETURN if error
//			otherwise offset/address
u32  ElfFile::addressToOffset(u32 address) const
{
	// check through all programs
	for(u32 i=0; i<getNumProgramHeaders(); i++)
	{
		if(address >= getProgramAddress(i) && address < (getProgramAddress(i) + getProgramSize(i)))
			return getProgramOffset(i) + (address - getProgramAddress(i));
	}
	// check through all sections
	for(u32 i=0; i<getNumSectionHeaders(); i++)
	{
		if(address >= getSectionAddress(i) && address < (getSectionAddress(i) + getSectionSize(i)))
			return getSectionOffset(i) + (address - getSectionAddress(i));
	}
	return ERROR_RETURN;
}
u32  ElfFile::offsetToAddress(u32 offset) const
{
	// check through all programs
	for(u32 i=0; i<getNumProgramHeaders(); i++)
	{
		if(offset >= getProgramOffset(i) && offset < (getProgramOffset(i) + getProgramSize(i)))
			return getProgramAddress(i) + (offset - getProgramOffset(i));
	}
	// check through all sections
	for(u32 i=0; i<getNumSectionHeaders(); i++)
	{
		if(offset >= getSectionOffset(i) && offset < (getSectionOffset(i) + getSectionSize(i)))
			return getSectionAddress(i) + (offset - getSectionOffset(i));
	}
	return ERROR_RETURN;
}


// add a new string to the string table
// 
// args:	string to add to table
// returns:	string offset of new string in table
//			ERROR_RETURN if error
u32  ElfFile::addStringToStringTable(const char *string)
{
	// check if file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// check if string table exists
	u32 stringTableIndex = getStringTableSectionIndex();
	if(stringTableIndex == ERROR_RETURN)
		return ERROR_RETURN;
	
	// get string table offset and string offset inside string table
	u32 stringTableOffset	= 0;
	u32 stringOffset		= 0;
	if(stringTableIndex != SHN_UNDEF)
	{
		// get offset of end string table (where new string is to be inserted)
		stringTableOffset	= getSectionOffset(stringTableIndex);
		stringOffset		= getSectionSize(stringTableIndex);
	}
	else
	{
		// if string table doesnt exist, create one at the end of the file
		fseek(fd, 0, SEEK_END);
		stringTableOffset = ftell(fd);
		
		// insert 'empty string' and 'string table name'
		char firstString[12];
		memcpy(firstString, "\0.shstrtab\0", 11);
		moveData(stringTableOffset, 11);
		fseek(fd, stringTableOffset, SEEK_SET);
		fwrite(firstString, 1, 11, fd);
		stringOffset = 11;
	}
	
	// get size of data to add to string table
	u32 stringSize			=  strlen(string) + 1;
	// add string to file
	moveData( stringTableOffset+stringOffset, stringSize);
	fseek(fd, stringTableOffset+stringOffset, SEEK_SET);
	fwrite(string, 1, stringSize, fd);
	
	// update string table header
	if(stringTableIndex != SHN_UNDEF)
	{
		// string table header already exists, so update size
		setSectionSize(stringTableIndex, getSectionSize(stringTableIndex) + stringSize);
	}
	else if(stringTableIndex == SHN_UNDEF)
	{
		// string table header doesnt exist, so create it
		
		// get offset to insert string table header at
		u32 headerOffset = getSectionHeadersOffset();
		if(headerOffset == ERROR_RETURN)
			return false;
		// insert header at end of file if its the first section header
		if(headerOffset == 0)
		{
			fseek(fd, 0, SEEK_END);
			headerOffset = ftell(fd);
			setSectionHeadersOffset(headerOffset);
		}
		// insert header after any other existing section headers
		headerOffset += getNumSectionHeaders()*getSectionHeadersSize();
		
		// if string table header is going to be inserted before the string table,
		// update the string table offset value!
		if(headerOffset >= stringTableOffset)
			stringTableOffset += sizeof(Elf32_Shdr);
		
		// setup string table header
		Elf32_Shdr sectionHeader;
		memset(&sectionHeader, 0, sizeof(Elf32_Shdr));
		u32 endian = getEndianFormat();
		sectionHeader.sh_offset	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, stringTableOffset);
		sectionHeader.sh_type	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, SHT_STRTAB);
		sectionHeader.sh_name	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, 1);	// set name to ".shstrtab"
		sectionHeader.sh_size	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, stringSize + 11);
		sectionHeader.sh_addralign	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, 1);
		sectionHeader.sh_entsize= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, 1);
		
		// insert string table section header into file
		setStringTableSectionIndex(getNumSectionHeaders());
		setNumSectionHeaders(getNumSectionHeaders() + 1);
		if(!moveData(headerOffset, sizeof(Elf32_Shdr)))
			return false;
		fseek(fd, headerOffset, SEEK_SET);
		if(fwrite(&sectionHeader, 1, sizeof(Elf32_Shdr), fd) != sizeof(Elf32_Shdr))
			return ERROR_RETURN;
	}
	
	// return new strings offset in string table
	return stringOffset;
}
// get a string from the string table
// string buffer must not be allocated before passing to method
// it will get allocated inside method, then must be deleted outside method
// 
// args:	offset of string in table
//			pointer to create buffer for holding string
// returns:	size of allocated buffer
//			ERROR_RETURN if error
u32  ElfFile::getStringFromStringTable(u32 stringOffset, char*& string) const
{
	// check if file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// check if string table exists
	u32 stringTableIndex = getStringTableSectionIndex();
	if(stringTableIndex == ERROR_RETURN)
		return ERROR_RETURN;
	
	// goto offset in string table
	u32 stringTableOffset = getSectionOffset(stringTableIndex);
	fseek(fd, stringTableOffset+stringOffset, SEEK_SET);
	
	// copy string to buffer
	u8* buffer = new u8[1024];
	fread(buffer, 1, 1024, fd);
	u32 stringLength = strlen((char*)buffer) + 1;
	string = new char[stringLength];
	strcpy(string, (char*)buffer);
	delete[] buffer;
	
	// return size of string buffer
	return stringLength;
}
	

// get the offset in the string table of the section name
// 
// args:	index of section
// returns:	offset of name string inside string table
//			ERROR_RETURN if error
u32  ElfFile::getSectionNameOffset(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return ERROR_RETURN;
	
	// get section header offset
	u32 headerOffset = getSectionHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return ERROR_RETURN;
	// goto section name offset value
	u32 offset = 0;
	fseek(fd, headerOffset+0, SEEK_SET);
	fread(&offset, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), offset);
}

// gets the type of program/section
// 
// args:	index of section
// returns:	type of section
//			ERROR_RETURN if error
u32  ElfFile::getProgramType(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumProgramHeaders())
		return ERROR_RETURN;
	
	// get program header offset
	u32 headerOffset = getProgramHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return ERROR_RETURN;
	// goto program type value
	u32 type = 0;
	fseek(fd, headerOffset+0, SEEK_SET);
	fread(&type, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), type);
}
u32  ElfFile::getSectionType(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return ERROR_RETURN;
	
	// get section header offset
	u32 headerOffset = getSectionHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return ERROR_RETURN;
	// goto section type value
	u32 type = 0;
	fseek(fd, headerOffset+4, SEEK_SET);
	fread(&type, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), type);
}

// gets the program/section flags
// 
// args:	index of section
// returns:	flags for section
//			ERROR_RETURN if error
u32  ElfFile::getProgramFlags(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumProgramHeaders())
		return ERROR_RETURN;
	
	// get program header offset
	u32 headerOffset = getProgramHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return ERROR_RETURN;
	// goto program flags value
	u32 flags = 0;
	fseek(fd, headerOffset+24, SEEK_SET);
	fread(&flags, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), flags);
}
u32  ElfFile::getSectionFlags(u32 index) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return ERROR_RETURN;
	
	// get section header offset
	u32 headerOffset = getSectionHeaderOffset(index);
	if(headerOffset == ERROR_RETURN)
		return ERROR_RETURN;
	// goto section flags value
	u32 flags = 0;
	fseek(fd, headerOffset+8, SEEK_SET);
	fread(&flags, 1, 4, fd);
	return TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != getEndianFormat(), flags);
}


// get name of the section
// string buffer must not be allocated before passing to method
// it will get allocated inside method, then must be deleted outside method
// 
// if an error is returned, the buffer does not need to be deleted
// 
// args:	section index
//			pointer to create buffer for holding name
// returns:	size of allocated buffer
//			ERROR_RETURN if error
u32  ElfFile::getSectionName(u32 index, char*& name) const
{
	u32 offset = getSectionNameOffset(index);
	if(offset == ERROR_RETURN)
		return ERROR_RETURN;
	return getStringFromStringTable(offset, name);
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
bool ElfFile::moveData(u32 moveOffset, s32 moveAmount)
{
	if(!GenericFile::moveData(moveOffset, moveAmount))
		return false;
	
	// update offsets of the 'header tables'
	u32 programHeaderTableOffset = getProgramHeadersOffset();
	if(programHeaderTableOffset != 0 && programHeaderTableOffset != ERROR_RETURN)
	{
		// update program header table offset if it was moved
		if(programHeaderTableOffset >= moveOffset)
			setProgramHeadersOffset(programHeaderTableOffset + moveAmount);
	}
	u32 sectionHeaderTableOffset = getSectionHeadersOffset();
	if(sectionHeaderTableOffset != 0 && sectionHeaderTableOffset != ERROR_RETURN)
	{
		// update section header table offset if it was moved
		if(sectionHeaderTableOffset >= moveOffset)
			setSectionHeadersOffset(sectionHeaderTableOffset + moveAmount);
	}
	
	// update offsets for any programs/sections that come after the inserted data
	for(u32 i=0; i<getNumProgramHeaders(); i++)
	{
		if(getProgramOffset(i) >= moveOffset)
			setProgramOffset(i, getProgramOffset(i) + moveAmount);
	}
	for(u32    i=0; i<getNumSectionHeaders(); i++)
	{
		if(getSectionOffset(i) >= moveOffset)
			setSectionOffset(i, getSectionOffset(i) + moveAmount);
	}
	
	return true;
}


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
bool ElfFile::addProgramHeader(u32 address, u32 size, u32 offset, u32 type, u32 flags, u32 alignment)
{
	// make sure file is open
	if(!isOpen())
		return false;
	// get offset to insert header to
	u32 headerOffset = getProgramHeadersOffset();
	if(headerOffset == ERROR_RETURN)
		return false;
	// insert header after elf header if there are currently no program headers
	if(headerOffset == 0)
	{
		headerOffset = sizeof(Elf32_Ehdr);
	}
	// insert header after any other existing program headers
	headerOffset += getNumProgramHeaders()*getProgramHeadersSize();
	// if inserting program header before program data,
	// update the offset of the program data to allow for the inserted header
	if(headerOffset <= offset)
		offset += sizeof(Elf32_Phdr);
	
	// setup program header
	Elf32_Phdr header;
	memset(&header, 0, sizeof(Elf32_Phdr));
	u32 endian = getEndianFormat();
	header.p_paddr	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, address);
	header.p_vaddr	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, address);
	header.p_memsz	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, size);
	header.p_filesz	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, size);
	header.p_flags	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, flags);
	header.p_offset	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, offset);
	header.p_type	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, type);
	header.p_align	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, alignment);
	
	// prepare to insert program header
	setNumProgramHeaders(getNumProgramHeaders() + 1);
	if(!moveData(headerOffset, sizeof(Elf32_Phdr)))
		return false;
	
	// set the program headers offset
	if(getProgramHeadersOffset() == 0)
		setProgramHeadersOffset(headerOffset);
	
	// insert program header
	fseek(fd, headerOffset, SEEK_SET);
	return (fwrite(&header, 1, sizeof(Elf32_Phdr), fd) == sizeof(Elf32_Phdr));
}
bool ElfFile::addSectionHeader(u32 address, u32 size, const char* name, u32 offset, u32 type, u32 flags, u32 alignment)
{
	// make sure file is open
	if(!isOpen())
		return false;
	// get offset to insert header to
	u32 headerOffset = getSectionHeadersOffset();
	if(headerOffset == ERROR_RETURN)
		return false;
	// insert header at end of file if its the first section header
	if(headerOffset == 0)
	{
		fseek(fd, 0, SEEK_END);
		headerOffset = ftell(fd);
		setSectionHeadersOffset(headerOffset);
	}
	// insert header after any other existing section headers
	headerOffset += getNumSectionHeaders()*getSectionHeadersSize();
	// if inserting section header before section data,
	// update the offset of the section data to allow for the inserted header
	if(headerOffset <= offset)
		offset += sizeof(Elf32_Shdr);
	
	// add section name to string table
	u32 nameOffset		= addStringToStringTable(name);
	// if inserting section header before section data,
	// update the offset of the section data to allow for the inserted header
	if(getSectionOffset(getStringTableSectionIndex())+nameOffset <= offset)
		offset += (strlen(name) + 1);
	
	// setup section header
	Elf32_Shdr header;
	memset(&header, 0, sizeof(Elf32_Shdr));
	u32 endian = getEndianFormat();
	header.sh_addr		= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, address);
	header.sh_addralign	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, alignment);
	header.sh_flags		= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, flags);
	header.sh_offset	= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, offset);
	header.sh_size		= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, size);
	header.sh_type		= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, type);
	header.sh_name		= TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN != endian, nameOffset);
	
	// insert section header
	setNumSectionHeaders(getNumSectionHeaders() + 1);
	if(!moveData(headerOffset, sizeof(Elf32_Shdr)))
		return false;
	fseek(fd, headerOffset, SEEK_SET);
	return (fwrite(&header, 1, sizeof(Elf32_Shdr), fd) == sizeof(Elf32_Shdr));
}

// remove a program/section header
// (updates all header offsets)
// 
// args:	index of program/section header to remove
// returns:	true if successfully removed
bool ElfFile::removeProgramHeader(u32 index)
{
	// make sure file is open
	if(!isOpen())
		return false;
	// make sure index is valid
	if(index >= getNumProgramHeaders())
		return false;
	
	// get offset and size of header to remove
	u32 removeSize	= getProgramHeadersSize();
	u32 removeOffset= getProgramHeaderOffset(index);
	if(removeSize == ERROR_RETURN || removeOffset == ERROR_RETURN)
		return false;
	// remove header
	if(removeData(removeOffset, removeSize) != removeSize)
		return false;
	setNumProgramHeaders(getNumProgramHeaders() - 1);
	
	// check if any headers left, if not clean up
	if(getNumProgramHeaders() == 0)
	{
		setProgramHeadersOffset(0);
	}
	return true;
}
bool ElfFile::removeSectionHeader(u32 index)
{
	// make sure file is open
	if(!isOpen())
		return false;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return false;
	
	// get offset and size of header to remove
	u32 removeSize	= getSectionHeadersSize();
	u32 removeOffset= getSectionHeaderOffset(index);
	if(removeSize == ERROR_RETURN || removeOffset == ERROR_RETURN)
		return false;
	// remove header
	if(removeData(removeOffset, removeSize) != removeSize)
		return false;
	// clear string index setting if this was the header for it
	if(getStringTableSectionIndex() == index)
		setStringTableSectionIndex(0);
	setNumSectionHeaders(getNumSectionHeaders() - 1);
	
	// check if any headers left, if not clean up
	if(getNumSectionHeaders() == 0)
	{
		setSectionHeadersOffset(0);
	}
	return true;
}


// write program/section data out to file
// 
// args:	index of program/section
//			file descriptor to write program/section data to
// returns:	size of data written
//			ERROR_RETURN if error
u32  ElfFile::extractProgramData(u32 index, FILE* out_fd) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumProgramHeaders())
		return ERROR_RETURN;
	
	// get program's offset and size
	u32 offset	= getProgramOffset(index);
	u32 size	= getProgramSize(index);
	if(offset == ERROR_RETURN || size == ERROR_RETURN)
		return ERROR_RETURN;
	
	// write out program data
	return writeOutData(offset, size, out_fd);
}
u32  ElfFile::extractSectionData(u32 index, FILE* out_fd) const
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return ERROR_RETURN;
	
	// get section's offset and size
	u32 offset	= getSectionOffset(index);
	u32 size	= getSectionSize(index);
	if(offset == ERROR_RETURN || size == ERROR_RETURN)
		return ERROR_RETURN;
	
	// write out section data
	return writeOutData(offset, size, out_fd);
}

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
u32  ElfFile::addProgram(FILE* in_fd, u32 address, u32 size, u32 type, u32 flags, u32 alignment)
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get the offset to insert the new program data at
	fseek(fd, 0, SEEK_END);
	u32 offset = ftell(fd);
	// add new program data
	u32 ret = readInData(offset, size, in_fd);
	
	// add new program header
	if(!addProgramHeader(address, size, offset, type, flags, alignment))
		return ERROR_RETURN;
	
	return ret;
}
u32  ElfFile::addSection(FILE* in_fd, u32 address, u32 size, const char* name, u32 type, u32 flags, u32 alignment)
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	
	// get the offset of the new section (enf of file)
	fseek(fd, 0, SEEK_END);
	u32 offset = ftell(fd);
	// add new section data
	u32 ret = readInData(offset, size, in_fd);
	
	// add new program header
	if(!addSectionHeader(address, size, name, offset, type, flags, alignment))
		return ERROR_RETURN;
	
	return ret;
}


// remove a program/section from dol file
// (will only remove program/section data if no other
//  references to it still exist in elf file)
// 
// args:	index of program/section to remove
// returns:	true if successfully removed
bool ElfFile::removeProgram(u32 index)
{
	// make sure file is open
	if(!isOpen())
		return false;
	// check if program index is valid
	if(index >= getNumProgramHeaders())
		return false;
	
	// get offset and size of program data to remove
	u32 offset	= getProgramOffset(index);
	u32 size	= getProgramSize(index);
	if(offset == ERROR_RETURN || size == ERROR_RETURN)
		return false;
	
	// remove program header
	if(!removeProgramHeader(index))
		return false;
	
	// remove program data from file if no more references to it
	// (data is still referenced if a program/section has its offset for their data)
	for(u32 i=0; i<getNumProgramHeaders(); i++)
	{
		// if data is referenced, return without removing it
		if(getProgramOffset(i) == offset)
			return true;
	}
	for(u32     i=0; i<getNumSectionHeaders(); i++)
	{
		// if data is referenced, return without removing it
		if(getSectionOffset(i) == offset)
			return true;
	}
	
	// no other sections referenced it, so remove it
	return (removeData(offset, size) != ERROR_RETURN);
}
bool ElfFile::removeSection(u32 index)
{
	// make sure file is open
	if(!isOpen())
		return false;
	// check if section index is valid
	if(index >= getNumSectionHeaders())
		return false;
	
	// get offset and size of section data to remove
	u32 offset	= getSectionOffset(index);
	u32 size	= getSectionSize(index);
	if(offset == ERROR_RETURN || size == ERROR_RETURN)
		return false;
	
	// remove section header
	if(!removeSectionHeader(index))
		return false;
	
	// remove sectionm data from file if no more references to it
	// (data is still referenced if a program/section has its offset for their data)
	for(u32 i=0; i<getNumProgramHeaders(); i++)
	{
		// if data is referenced, return without removing it
		if(getProgramOffset(i) == offset)
			return true;
	}
	for(u32     i=0; i<getNumSectionHeaders(); i++)
	{
		// if data is referenced, return without removing it
		if(getSectionOffset(i) == offset)
			return true;
	}
	
	// no other sections referenced it, so remove it
	return (removeData(offset, size) != ERROR_RETURN);
}

// remove all programs/sections from dol file
// 
// returns:	true if successfully removed
bool ElfFile::removeAllPrograms(void)
{
	bool ret = true;
	while(getNumProgramHeaders())
		if(!removeProgram(getNumProgramHeaders() - 1))
			ret = false;
	return ret;
}
bool ElfFile::removeAllSections(void)
{
	bool ret = true;
	while(getNumSectionHeaders())
		if(!removeSection(getNumSectionHeaders() - 1))
			ret = false;
	return ret;
}
bool ElfFile::removeAllProgramsAndSections(void)
{
	return (removeAllPrograms() && removeAllSections());
}







/*
// read program/section in from file to a specified program/section
// replace the given program/section with the newly added program/section
// 
// args:	index of program/section
//			file descriptor to read program/section data from
//			address to load the program/section data to
//			size of program/section data to insert
// returns:	size of data written
//			ERROR_RETURN if error
u32  ElfFile::replaceProgram(u32 index, FILE* in_fd, u32 address, u32 size)
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumProgramHeaders())
		return ERROR_RETURN;
	
	// delete existing program
	removeProgram(index);
	
	// get the offset of the section
	fseek(fd, 0, SEEK_END);
	u32 offset = ftell(fd);
	
	// add new program info to header
	setProgramAddress(	index, address);
	setProgramOffset(	index, offset);
	setProgramSize(		index, size);
	
	// insert program at end of file
	u32 ret = readInData(offset, size, in_fd);
	return ret;
}
u32  ElfFile::replaceSection(u32 index, FILE* in_fd, u32 address, u32 size)
{
	// make sure file is open
	if(!isOpen())
		return ERROR_RETURN;
	// make sure index is valid
	if(index >= getNumSectionHeaders())
		return ERROR_RETURN;
	
	// delete existing program
	removeSection(index);
	
	// get the offset of the section
	fseek(fd, 0, SEEK_END);
	u32 offset = ftell(fd);
	
	// add new section info to header
	setSectionAddress(	index, address);
	setSectionOffset(	index, offset);
	setSectionSize(		index, size);
	
	// insert section at end of file
	u32 ret = readInData(offset, size, in_fd);
	return ret;
}
*/
