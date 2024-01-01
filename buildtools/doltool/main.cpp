// 
// DolTool  
// ported to linux by TenOfTen and additions by others
// just compile with g++ -o doltool *cpp
// 
// prints info on dol files
// converts dol to binary file
// converts dol to elf file
// 
// 0.3.0 TenOfTen - linux port of loser's app
// 0.3.1 deufeufeu - microfix added for all the people who can't load dol with psoload and get random video garbage instead
// 0.3.2 Steve_- take loadaddress and entrypoint arg
//       TenOfTen - default to 80003100

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ElfFile.h"
#include "DolFile.h"
#include "Types.h"
#include "UtilFuncs.h"

const char* title = "DolTool v0.3.2 by loser / TenOfTen / deufeufeu / Steve_-";
const char* usage = "usage:    DolTool <option> <input filename> <loadaddress> <entrypoint>\n" \
                                        "options:  -i list info on dol file\n"                        \
                                        "          -b dump dol to binary file\n"                \
                                        "          -e dump dol to elf file\n"                        \
                                        "          -c convert binary file to dol file\n"\
                                        "          -d convert elf file to dol file\n\n" \
                                        "          loadaddress and entrypoint are for -c, both defaults to 80003100 if not specified\n";

// nop instruction opcode in ppc asm
#define NOP                0x60000000

// list info on dol file
// 
// args:        dol file object
// returns:        true if successful
bool listDolInfo(const DolFile& dolFile);

// dump dol file to a binary file
// 
// args:        dol file object
// returns:        true if successful
bool dumpDolToBinaryFile(const DolFile& dolFile);

// dump dol file to an elf file
// 
// args:        dol file object
// returns:        true if successful
bool dumpDolToElfFile(const DolFile& dolFile);

// convert a binary file to a dol file
// 
// args:        dol file object
// returns:        true if successful
bool convertBinaryFileToDol(DolFile& dolFile, const char *binFilename, unsigned int loadAddress, unsigned int entryPoint);

// convert an elf file to a dol file
// 
// args:        dol file object
// returns:        true if successful
bool convertElfFileToDol(DolFile& dolFile, const char *elfFilename);


int main(int argc, char *argv[])
{
        bool dumpBin        = false;
        bool dumpElf        = false;
        bool listInfo        = false;
        bool convertBin        = false;
        bool convertElf        = false;
        unsigned int loadAddress = 0, entryPoint = 0;
        
        // print title
        printf("%s\n", title);
        
        // process args
        if(argc == 2)
        {
                // if no options given, list info on dol file
                listInfo = true;
        }
        else if(argc >= 3)
        {
                // check case-insensitively for options
                if(     strcasecmp("-e", argv[1]) == 0)
                        dumpElf                = true;
                else if(strcasecmp("-b", argv[1]) == 0)
                        dumpBin                = true;
                else if(strcasecmp("-i", argv[1]) == 0)
                        listInfo        = true;
                else if(strcasecmp("-c", argv[1]) == 0)
                {
                        convertBin        = true;
                        if (argc == 5)
                        {
                            loadAddress       = strtoul(argv[3], (char **)NULL, 16);
                            entryPoint        = strtoul(argv[4], (char **)NULL, 16);
                        }
                        else if(argc == 3)
                        {
                            loadAddress = 0x80003100u;
                            entryPoint = 0x80003100u;
                        }
                        else
                        {
                            printf("%s\n", usage);
                            return 1;
                        }
                        

                }
                else if(strcasecmp("-d", argv[1]) == 0)
                        convertElf        = true;
                else
                {
                        // usage error
                        printf("%s\n", usage);
                        return 1;
                }
        }
        else
        {
                // usage error
                printf("%s\n", usage);
                return 1;
        }
        
        // open/create dol file
        char filename[256];
        strcpy(filename, argv[2]);
        DolFile dolFile;
        if(convertBin || convertElf)
        {
                char dolFilename[256];
                strcpy(dolFilename, filename);
                ChangeFileExtension(dolFilename, "dol");
                
                if(!dolFile.create(dolFilename))
                {
                        printf("Error creating %s!\n", dolFilename);
                        return 2;
                }
        }
        else
        {
                if(!dolFile.open(filename))
                {
                        printf("Error opening %s!\n", filename);
                        return 2;
                }
        }
        
        bool ret = true;
        // list info on dol file
        if(listInfo)
                ret = listDolInfo(dolFile);
        // dump to binary file
        else if(dumpBin)
                ret = dumpDolToBinaryFile(dolFile);
        // dump to elf file
        else if(dumpElf)
                ret = dumpDolToElfFile(dolFile);
        // convert binary file to dol
        else if(convertBin)
                ret = convertBinaryFileToDol(dolFile, filename, loadAddress, entryPoint);
        // convert elf file to dol
        else if(convertElf)
                ret = convertElfFileToDol(dolFile, filename);
        
        // close dol file
        dolFile.close();
        
        // return ok if finished ok
        if(ret)
                return 0;
        return 666;
}


// list info on dol file
// 
// args:        dol file object
// returns:        true if successful
bool listDolInfo(const DolFile& dolFile)
{
        unsigned int i;

        // print BSS info
        printf("\n");
        printf("BSS Address:      %08X\n",                dolFile.getBssAddress());
        printf("BSS Size:         %08X\n\n",        dolFile.getBssSize());
        // print entry point
        printf("Entry Point:      %08X\n",                dolFile.getEntryPoint());
        
        // print out TEXT section info
        printf("\n");
        for(i=0; i<dolFile.getMaxNumTextSections(); i++)
        {
                if(dolFile.isTextSectionValid(i))
                        printf("Text Section %2d:  Offset=%08X  Address=%08X  Size=%08X\n", i,
                                dolFile.getTextOffset(i), dolFile.getTextAddress(i), dolFile.getTextSize(i));
        }
        if(dolFile.getNumTextSections() > 0)
                printf("\n");
        
        // print out DATA section info
        for(i=0; i<dolFile.getMaxNumDataSections(); i++)
        {
                if(dolFile.isDataSectionValid(i))
                        printf("Data Section %2d:  Offset=%08X  Address=%08X  Size=%08X\n", i,
                                dolFile.getDataOffset(i), dolFile.getDataAddress(i), dolFile.getDataSize(i));
        }
        if(dolFile.getNumDataSections() > 0)
                printf("\n");
        
        return true;
}

// dump dol file to a binary file
// 
// args:        dol file object
// returns:        true if successful
bool dumpDolToBinaryFile(const DolFile& dolFile)
{
        unsigned int i;
        
        // get binary filename
        char binFilename[256];
        strcpy(binFilename, dolFile.getFilename());
        ChangeFileExtension(binFilename, "bin");
        
        // open binary output file
        FILE* bin_fd = 0;
        bin_fd = fopen(binFilename, "w+b");
        if(bin_fd == 0)
        {
                printf("Error creating %s!\n", binFilename);
                return false;
        }
        
        // get load address and entry point
        unsigned int loadAddress = dolFile.getLowestAddress();
        unsigned int entryPoint  = dolFile.getEntryPoint();
        // check if load address is same as entry point
        if(loadAddress != entryPoint)
        {
                // if load address is different to exec address,
                // it could cause problems in binary format
                
                // check if there is space before the load address
                // to put in a jump to the entry point
                if(loadAddress >= 0x80003100 + 4)
                {
                        // insert jump to entry point before code
                        unsigned int jump = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, 0x48000000 | (entryPoint - (loadAddress - 4)) );
                        fwrite(&jump, 1, 4, bin_fd);
                        // update load address and entry point
                        loadAddress -= 4;
                        entryPoint = loadAddress;
                }
        }
        
        // dump text bits first
        for(i=0; i<dolFile.getMaxNumTextSections(); i++)
        {
                if(dolFile.isTextSectionValid(i))
                {
                        unsigned int address= dolFile.getTextAddress(i);
                        //unsigned int offset = dolFile.getTextOffset(i);
                        //unsigned int size        = dolFile.getTextSize(i);
                        
                        // make sure binary file is big enough to contain correct section offset
                        unsigned char zero = 0;
                        fseek(bin_fd, 0, SEEK_END);
                        // if its not big enough, buffer file with 0 till its big enough
                        while((unsigned int)ftell(bin_fd) < (address-loadAddress))
                                fwrite(&zero, 1, 1, bin_fd);
                        
                        // goto section offset in binary file
                        if(fseek(bin_fd, address-loadAddress, SEEK_SET) != 0)
                                printf("Error seeking to TEXT section %d in binary file\n", i);
                        
                        // write section to binary file
                        if(dolFile.extractTextSection(i, bin_fd) == ERROR_RETURN)
                                printf("Error writing TEXT section %d in binary file\n", i);
                }
        }
        
        // then dump data bits
        for(i=0; i<dolFile.getMaxNumDataSections(); i++)
        {
                if(dolFile.isDataSectionValid(i))
                {
                        unsigned int address= dolFile.getDataAddress(i);
                        //unsigned int offset = dolFile.getDataOffset(i);
                        //unsigned int size        = dolFile.getDataSize(i);
                        
                        // make sure binary file is big enough to contain correct section offset
                        unsigned char zero = 0;
                        fseek(bin_fd, 0, SEEK_END);
                        // if its not big enough, buffer file with 0 till its big enough
                        while((unsigned int)ftell(bin_fd) < (address-loadAddress))
                                fwrite(&zero, 1, 1, bin_fd);
                        
                        // goto section offset in binary file
                        if(fseek(bin_fd, address-loadAddress, SEEK_SET) != 0)
                                printf("Error seeking to DATA section %d in binary file\n", i);
                        
                        // write section to binary file
                        if(dolFile.extractDataSection(i, bin_fd) == ERROR_RETURN)
                                printf("Error writing DATA section %d in binary file\n", i);
                }
        }
        
        // last attempt to make the entry point the same as the load address
        // this checks if there are nops at the load address that we can then overwrite
        // (ive decided to leave it out for now incase those nops are 'required' somehow...
/*        if(loadAddress != entryPoint)
        {
                int instr = 0;
                fseek(bin_fd, 0, SEEK_SET);
                fread(&instr, 4, 1, bin_fd);
                if(SET_ENDIAN_32BIT(instr) == NOP)
                {
                        // insert jump to entry point before code
                        unsigned int jump = TEST_CHANGE_ENDIAN_32BIT(MY_ENDIAN == LITTLE_ENDIAN, (x48000000 | (entryPoint - (loadAddress - 4)));
                        fseek(bin_fd, 0, SEEK_SET);
                        fwrite(&jump, 1, 4, bin_fd);
                        // update entry point
                        entryPoint = loadAddress;
                }
        }
*/        
        // close file and finish up
        fclose(bin_fd);
        printf(        "\nDumped %s to %s\n"
                        "Load Address: 0x%08X\n"
                        "Entry point:  0x%08X\n"
                        , dolFile.getFilename(), binFilename, loadAddress, entryPoint);
        
        return true;
}

// convert a binary file to a dol file
// 
// args:        dol file object
//                        binary filename
// returns:        true if successful
bool convertBinaryFileToDol(DolFile& dolFile, const char *binFilename, unsigned int loadAddress, unsigned int entryPoint)
{
        // open binary input file
        FILE* bin_fd = 0;
        bin_fd = fopen(binFilename, "rb");
        if(bin_fd == 0)
        {
                printf("Error opening %s!\n", binFilename);
                return false;
        }
        
        // get size of binary file
        fseek(bin_fd, 0, SEEK_END);
        unsigned int size = ftell(bin_fd);
        fseek(bin_fd, 0, SEEK_SET);
        
        // insert new text section for binary file
        // (add everything as text section)
        dolFile.insertTextSection(0, bin_fd, loadAddress, size);
        dolFile.setEntryPoint(entryPoint);
        
        // close file and finish up
        fclose(bin_fd);
        printf(        "\nConverted %s to %s\n", binFilename, dolFile.getFilename());
        return true;
}


// dump dol file to an elf file
// 
// args:        dol file object
// returns:        true if successful
bool dumpDolToElfFile(const DolFile& dolFile)
{
        // work out filename to use for elf file
        char elfFilename[256];
        strcpy(elfFilename, dolFile.getFilename());
        ChangeFileExtension(elfFilename, "elf");
        
        // create elf output file
        ElfFile elfFile;
        if(!elfFile.create(elfFilename, ELFDATA2MSB))
                return false;
        
        // set elf file as PPC
        elfFile.setMachineType(EM_PPC);
        
        // set entry point in elf file
        elfFile.setEntryPoint(dolFile.getEntryPoint());
        
        // create a temp file to store section data during copy
        FILE* temp_fd = fopen("~temp.tmp", "w+b");
        if(temp_fd == 0)
        {
                elfFile.close();
                return false;
        }
        char name[32];
        
        // add dol text sections to elf file
        for(u32 i=0; i<dolFile.getNumTextSections(); i++)
        {
                // get section data from dol file
                fseek(temp_fd, 0, SEEK_SET);
                dolFile.extractTextSection(i, temp_fd);
                sprintf(name, ".text%d", i);
                // add section data to elf file
                fseek(temp_fd, 0, SEEK_SET);
                elfFile.addSection(temp_fd, dolFile.getTextAddress(i), dolFile.getTextSize(i),
                        name, SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR, 32);
                
                // add a program header for the section just added
                // the section just added, was added as the last section
//                u32 offset = elfFile.getSectionOffset(elfFile.getNumSections() - 1);
//                elfFile.addProgramHeader(dolFile.getTextAddress(i), dolFile.getTextSize(i), offset, PT_LOAD, PF_EXEC|PF_READ, 32);
        }
        // add dol data sections to elf file
        for(u32    i=0; i<dolFile.getNumDataSections(); i++)
        {
                // get section data from dol file
                fseek(temp_fd, 0, SEEK_SET);
                dolFile.extractDataSection(i, temp_fd);
                sprintf(name, ".data%d", i);
                // add section data to elf file
                fseek(temp_fd, 0, SEEK_SET);
                elfFile.addSection(temp_fd, dolFile.getDataAddress(i), dolFile.getDataSize(i),
                        name, SHT_PROGBITS, SHF_ALLOC|SHF_WRITE, 32);
                
                // add a program header for the section just added
                // the section just added, was added as the last section
//                u32 offset = elfFile.getSectionOffset(elfFile.getNumSections() - 1);
//                elfFile.addProgramHeader(dolFile.getDataAddress(i), dolFile.getDataSize(i), offset, PT_LOAD, PF_WRITE|PF_READ, 32);
        }
        
        // add bss section to elf file
        elfFile.addSectionHeader(dolFile.getBssAddress(), dolFile.getBssSize(),
                ".bss", 0, SHT_NOBITS, SHF_WRITE|SHF_ALLOC, 1);
        // add a program header for the section just added
        // the section just added, was added as the last section
//        u32 index = elfFile.getNumSections() - 1;
//        elfFile.addProgramHeader(elfFile.getSectionAddress(index), elfFile.getSectionSize(index), elfFile.getSectionOffset(index), PT_LOAD, PF_WRITE|PF_READ, 32);
        
        // finish up
        fclose(temp_fd);
        remove("~temp.tmp");
        elfFile.close();
        printf(        "\nConverted %s to to %s\n", dolFile.getFilename(), elfFilename);
        return true;
}


// convert an elf file to a dol file
// 
// args:        dol file object
//                        elf filename
// returns:        true if successful
bool convertElfFileToDol(DolFile& dolFile, const char *elfFilename)
{
        // open elf input file
        ElfFile elfFile;
        if(!elfFile.open(elfFilename))
                return false;
        
        if(!elfFile.isPpc())
                printf("Elf file doesnt seem to be for PPC!\n");
        
        // create a temp file to store section data during copy
        FILE* temp_fd = fopen("~temp.tmp", "w+b");
        if(temp_fd == 0)
        {
                elfFile.close();
                return false;
        }
        
        // check which sections to add to dol file
        for(u32 i=0; i<elfFile.getNumSections(); i++)
        {
                // check if section type is 'PROGBITS'
                // these are the sections required for the dol file
                if(elfFile.getSectionType(i) == SHT_PROGBITS)
                {
                        // get section data from elf file
                        fseek(temp_fd, 0, SEEK_SET);
                        elfFile.extractSectionData(i, temp_fd);
                        fseek(temp_fd, 0, SEEK_SET);
                        
                        // sections with 'EXEC' flags are program code, and so
                        // should be added as a TEXT section in the dol file
                        if(elfFile.getSectionFlags(i) & SHF_EXECINSTR)
                        {
                                // add section data to dol file
                                dolFile.addTextSection(temp_fd, elfFile.getSectionAddress(i), elfFile.getSectionSize(i));
                        }
                        else
                        {
                                // add section data to dol file
                                if (elfFile.getSectionAddress(i) != 0 && elfFile.getSectionSize(i) != 0)
                                      dolFile.addDataSection(temp_fd, elfFile.getSectionAddress(i), elfFile.getSectionSize(i));
                        }
                }
                else
                {
                        // check for bss section by checking section name for '.bss'
                        char *name = 0;
                        if(elfFile.getSectionName(i, name) != ERROR_RETURN)
                        {
                                if(strcasecmp(name, ".bss") == 0)
                                {
                                        dolFile.setBssAddress(elfFile.getSectionAddress(i));
                                        dolFile.setBssSize(elfFile.getSectionSize(i));
                                }
                                delete[] name;
                        }
                }
        }
        
        // set dol files entry point
        dolFile.setEntryPoint(elfFile.getEntryPoint());
        
        // finish up
        fclose(temp_fd);
        remove("~temp.tmp");
        elfFile.close();
        printf(        "\nConverted %s to %s\n", elfFilename, dolFile.getFilename());
        return true;
}
