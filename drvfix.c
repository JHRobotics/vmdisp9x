/*****************************************************************************

Copyright (c) 2024 Jaroslav Hensl <emulator@emulace.cz>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*****************************************************************************/

/* Fix NE executable version to 0x400 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#pragma pack(push)
#pragma pack(1)

/* http://www.delorie.com/djgpp/doc/exe/ */
typedef struct EXE_header {
  uint16_t signature; /* == 0x5a4D */
  uint16_t bytes_in_last_block;
  uint16_t blocks_in_file;
  uint16_t num_relocs;
  uint16_t header_paragraphs;
  uint16_t min_extra_paragraphs;
  uint16_t max_extra_paragraphs;
  uint16_t ss;
  uint16_t sp;
  uint16_t checksum;
  uint16_t ip;
  uint16_t cs;
  uint16_t reloc_table_offset;
  uint16_t overlay_number;
} EXE_header_t;

#define EXE_SIGN 0x5a4D

/* from: https://wiki.osdev.org/NE */
typedef struct NE_header
{
	uint16_t signature;          //"NE", 0x4543
	uint8_t MajLinkerVersion;    //The major linker version
	uint8_t MinLinkerVersion;    //The minor linker version
	uint16_t EntryTableOffset;   //Offset of entry table, see below
	uint16_t EntryTableLength;   //Length of entry table in bytes
	uint32_t FileLoadCRC;        //32-bit CRC of entire contents of file
	uint8_t ProgFlags;           //Program flags, bitmapped
	uint8_t ApplFlags;           //Application flags, bitmapped
	uint16_t AutoDataSegIndex;   //The automatic data segment index
	uint16_t InitHeapSize;       //The initial local heap size
	uint16_t InitStackSize;      //The initial stack size
	uint32_t EntryPoint;         //CS:IP entry point, CS is index into segment table
	uint32_t InitStack;          //SS:SP initial stack pointer, SS is index into segment table
	uint16_t SegCount;           //Number of segments in segment table
	uint16_t ModRefs;            //Number of module references (DLLs)
	uint16_t NoResNamesTabSiz;   //Size of non-resident names table, in bytes (Please clarify non-resident names table)
	uint16_t SegTableOffset;     //Offset of Segment table
	uint16_t ResTableOffset;     //Offset of resources table
	uint16_t ResidNamTable;      //Offset of resident names table
	uint16_t ModRefTable;        //Offset of module reference table
	uint16_t ImportNameTable;    //Offset of imported names table (array of counted strings, terminated with string of length 00h)
	uint32_t OffStartNonResTab;  //Offset from start of file to non-resident names table
	uint16_t MovEntryCount;      //Count of moveable entry point listed in entry table
	uint16_t FileAlnSzShftCnt;   //File alignment size shift count (0=9(default 512 byte pages))
	uint16_t nResTabEntries;     //Number of resource table entries
	uint8_t targOS;              //Target OS
	uint8_t OS2EXEFlags;         //Other OS/2 flags
	uint16_t retThunkOffset;     //Offset to return thunks or start of gangload area - what is gangload?
	uint16_t segrefthunksoff;    //Offset to segment reference thunks or size of gangload area
	uint16_t mincodeswap;        //Minimum code swap area size
	uint16_t expctwinver;        //Expected windows version eg. 0x030A, 0x0400
} NE_header_t;

#define NE_SIGN 0x454E

#pragma pack(pop)

#define ERROR_OPEN  -1
#define ERROR_FREAD -2
#define ERROR_WRITE -3
#define ERROR_SEEK  -4
#define ERROR_SIGN_MZ  -5
#define ERROR_SIGN_NE  -6

long fix_exe(const char *file, uint16_t new_expctwinver)
{
	EXE_header_t exe;
	NE_header_t   ne;
	FILE *f;
	int rc = 0;
	long offset;
	
	f = fopen(file, "r+b");
	if(f != NULL)
	{
		if(fread(&exe, sizeof(EXE_header_t), 1, f) == 1)
		{
			if(exe.signature == EXE_SIGN)
			{
				offset = exe.blocks_in_file * 512;
				if(exe.bytes_in_last_block)
				{
					offset -= (512 - exe.bytes_in_last_block);
				}
				
				if(fseek(f, offset, SEEK_SET) == 0)
				{
					if(fread(&ne, sizeof(NE_header_t), 1, f) == 1)
					{
						if(ne.signature == NE_SIGN)
						{
							rc = ne.expctwinver;
							if(ne.expctwinver != new_expctwinver)
							{
								ne.expctwinver = new_expctwinver;
								if(fseek(f, offset, SEEK_SET) == 0)
								{
									if(fwrite(&ne, sizeof(NE_header_t), 1, f) != 1)
										rc = ERROR_WRITE;
								} else rc = ERROR_SEEK;
							}
						} else rc = ERROR_SIGN_NE;
					} else rc = ERROR_FREAD;
				} else rc = ERROR_SEEK;
			} else rc = ERROR_SIGN_MZ;
		} else rc = ERROR_FREAD;
		
		fclose(f);
	} else rc = ERROR_OPEN;
	
	return rc;
}

int main(int argc, char *argv[])
{
	uint16_t expctwinver = 0x400;
	long rc;
		
	if(argc < 2)
	{
		printf("Usage: %s <file_to_patch.drv> [new expected windows version in hex]\n"
		"expected version is 0400 by default.\n\n"
		"Example: %s mydriver.drv 0400\n", argv[0], argv[0]);
		return EXIT_FAILURE;
	}
	
	if(argc >= 4)
	{
		expctwinver = strtol(argv[2], NULL, 16);
	}
	
	rc = fix_exe(argv[1], expctwinver);
	
	if(rc < 0)
	{
		switch(rc)
		{
			case ERROR_OPEN:
				printf("Failed to open %s\n", argv[1]);
				break;
			case ERROR_FREAD:
				printf("Read error (file corrupted?)\n");
				break;
			case ERROR_WRITE:
				printf("Write error (file readonly, in usage)\n");
				break;
			case ERROR_SEEK:
				printf("Seek error (file corrupted?)\n");
				break;
			case ERROR_SIGN_MZ:
				printf("File is not EXE!\n");
				break;
			case ERROR_SIGN_NE:
				printf("File is not NE!\n");
				break;
		}
		
		return EXIT_FAILURE;
	}
	
	if(rc == expctwinver)
	{
		printf("%s: NE.expctwinver is already %lX\n", argv[1], rc);
	}
	else
	{
		printf("%s: NE.expctwinver %lX => %lX\n", argv[1], rc, ((unsigned long)expctwinver));
	}
	
	return EXIT_SUCCESS;
}

