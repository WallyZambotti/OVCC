/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

#include "defines.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "tcc1014mmu.h"
#include "iobus.h"
//#include "cc3rom.h"
#include "config.h"
#include "tcc1014graphicsSDL.h"
#include "pakinterface.h"
#include "logger.h"
#include "hd6309.h"
#include "fileops.h"
#include <sys/mman.h>
#include <fcntl.h>

#if defined(_WIN64)
#define MSABI 
#else
#define MSABI __attribute__((ms_abi))
#endif

typedef unsigned char   UINT8, *PUINT8;
typedef unsigned short  UINT16, *PUINT16;
typedef unsigned int  UINT32, *PUINT32;
typedef unsigned long  UINT64, *PUINT64;

static unsigned char *MemPages[1024];
static unsigned short MemPageOffsets[1024];
static unsigned char *memory=NULL;	//Emulated RAM FULL 128-8096K
static unsigned char *taskmemory=NULL;	//Emulated RAM 64k
static unsigned char *InternalRomBuffer=NULL;
static unsigned char MmuTask=0;		// $FF91 bit 0
static unsigned char MmuEnabled=0;	// $FF90 bit 6
static unsigned char RamVectors=0;	// $FF90 bit 3
static unsigned char MmuState=0;	// Composite variable handles MmuTask and MmuEnabled
static unsigned char RomMap=0;		// $FF90 bit 1-0
static unsigned char MapType=0;		// $FFDE/FFDF toggle Map type 0 = ram/rom
static unsigned short MmuRegisters[4][8];	// $FFA0 - FFAF
static unsigned int MemConfig[4]={0x20000,0x80000,0x200000,0x800000};
static unsigned short RamMask[4]={15,63,255,1023};
static unsigned char StateSwitch[4]={8,56,56,56};
static unsigned char VectorMask[4]={15,63,63,63};
static unsigned char VectorMaska[4]={12,60,60,60};
static unsigned int VidMask[4]={0x1FFFF,0x7FFFF,0x1FFFFF,0x7FFFFF};
static unsigned char CurrentRamConfig=1;
static unsigned short MmuPrefix=0;

#define CoCoPageSize (8*1024)
#define Mem64kSize (64*1024)
#define CoCoROMSize (4*CoCoPageSize) // either 4 or 8 * 8k pages are reserved even though we only use 4 of them
#define MMUBankMax 2
#define MMUSlotMax 8
#define handle_error(msg) do { perror(msg) ; exit(EXIT_FAILURE);} while (0)
#define CoCoFullMemOffset 0
#define Task0MemOffset (StateSwitch[CurrentRamConfig]*CoCoPageSize)
#define Task1MemOffset (StateSwitch[CurrentRamConfig]*CoCoPageSize)
#define VideoMemOffset (StateSwitch[CurrentRamConfig]*CoCoPageSize)
#define RomMemOffset (MemConfig[CurrentRamConfig])
#define TotalMMmemSize (MemConfig[CurrentRamConfig]+CoCoROMSize)

int CoCoMemFD;

PUINT8 ptrCoCoFullMem = NULL, ptrCoCoTask0Mem = NULL, ptrCoCoTask1Mem = NULL, ptrCoCoRomMem = NULL, ptrTasks[MMUBankMax],
	MMUbank[MMUBankMax][MMUSlotMax] = 
	{ 
		{ NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL }, 
		{ NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL }
	};

UINT16 MMUpages[MMUBankMax][MMUSlotMax] =
{
	{ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }, 
	{ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }
};

void UpdateMmuArray(void);
unsigned char *Create32KROMMemory();
unsigned char *Create64KVirtualMemory(unsigned int);
int SetMMUslot(UINT8, UINT8, UINT16);

/*****************************************************************************************
* MmuInit Initilize and allocate memory for RAM Internal and External ROM Images.        *
* Copy Rom Images to buffer space and reset GIME MMU registers to 0                      *
* Returns NULL if any of the above fail.                                                 *
*****************************************************************************************/
unsigned char *MmuInit(unsigned char RamConfig)
{
	unsigned int RamSize=0;
	unsigned int Index1=0;
	RamSize=MemConfig[RamConfig];
	CurrentRamConfig=RamConfig;

	if (memory != NULL)
		freePhysicalMemPages(); // free(memory);

	memory = Create64KVirtualMemory(RamSize); // malloc(RamSize);
	if (memory==NULL)
		return(NULL);

	taskmemory = ptrCoCoTask0Mem; // default to first task 0
	SetVidMaskSDL(VidMask[CurrentRamConfig]);

	InternalRomBuffer = Create32KROMMemory();
	if (InternalRomBuffer == NULL)
		return(NULL);

	CopyRom();
	MmuReset();
	return(memory);
}

void MmuReset(void)
{
	unsigned int Index1=0,Index2=0;
	MmuTask=0;
	MmuEnabled=0;
	RamVectors=0;
	MmuState=0;
	RomMap=0;
	MapType=0;
	MmuPrefix=0;
	for (Index1=0;Index1<8;Index1++)
		for (Index2=0;Index2<4;Index2++)
		{
			UINT16 page = StateSwitch[CurrentRamConfig] + Index1;
			MmuRegisters[Index2][Index1]=page;
			if (Index2 < 2) 
			{
				MMUbank[Index2][Index1] = ptrTasks[Index2] + Index1 * CoCoPageSize;
				SetMMUslot(Index2, Index1, page);
			}
		}

	for (Index1=0;Index1<1024;Index1++)
	{
		MemPages[Index1]=memory+( (Index1 & RamMask[CurrentRamConfig]) *0x2000);
		MemPageOffsets[Index1]=1;
	}
	SetRomMap(0);
	SetMapType(0);
	return;
}

void freePhysicalMemPages()
{
	for(int pageCnt = 0 ; pageCnt < MMUSlotMax ; pageCnt++)
	{
		if (MMUbank[0][pageCnt] != NULL) munmap(MMUbank[0][pageCnt], CoCoPageSize);
		if (MMUbank[1][pageCnt] != NULL) munmap(MMUbank[1][pageCnt], CoCoPageSize);
		MMUbank[0][pageCnt] = NULL;
		MMUbank[1][pageCnt] = NULL;
	}

	if(ptrCoCoRomMem != NULL) munmap(ptrCoCoRomMem, Mem64kSize);
	if(ptrCoCoTask0Mem != NULL) munmap(ptrCoCoTask0Mem, Mem64kSize);
	if(ptrCoCoTask1Mem != NULL) munmap(ptrCoCoTask1Mem, Mem64kSize);
	if(ptrCoCoFullMem != NULL) munmap(ptrCoCoFullMem, TotalMMmemSize);

	ptrCoCoRomMem = NULL;
	ptrCoCoTask0Mem = NULL;
	ptrCoCoTask1Mem = NULL;
	ptrCoCoFullMem = NULL;
}

UINT8 *MemPagePointer(PUINT8 ptrmem, UINT16 page)
{
	return(ptrmem + page * CoCoPageSize);
}

int SetMMUslot(UINT8 task, UINT8 slotnum, UINT16 mempage)
{
  int memOffset;
  PUINT8 memPtr;

	if (MMUpages[task][slotnum] != mempage)
	{
		memOffset = mempage * CoCoPageSize;
		memPtr = MMUbank[task][slotnum];

		MMUbank[task][slotnum] = (PUINT8)mmap(memPtr, CoCoPageSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, CoCoMemFD, memOffset);

		if ((void*)memPtr != (void*)MMUbank[task][slotnum])
		{
			char errmsg[128]; 
			sprintf(errmsg, "Failed to map page %d to mmu[%d][%d]\n",	mempage, task, slotnum);
			handle_error(errmsg);
			return 0;
		}

		MMUpages[task][slotnum] = mempage;
	}

  return 1;
}

void UpdateMMap(void)
{
	UINT8 slotNo;
	UINT16 page;

	for (int slotNo = 0; slotNo < 8; slotNo++)
	{
		if (MapType || slotNo < 4) // memory pages are always memory below bank 4 or if MapType is RAM
		{
			if (MmuEnabled)
			{
				page = MmuRegisters[MmuTask][slotNo];
				SetMMUslot(MmuTask, slotNo, page);
			}
			else  // map the default pages 
			{
				page = StateSwitch[CurrentRamConfig] + slotNo;
				SetMMUslot(0, slotNo, page);
				SetMMUslot(1, slotNo, page);
			}
		}
		else // The bank is 4 and above and we are dealing with ROM
		{
			switch (RomMap)
			{
			case 0: //16K Internal 16K External
			case 1:
				if (slotNo < 6)
				{
					page = slotNo - 4 + StateSwitch[CurrentRamConfig] + 8;
					SetMMUslot(MmuTask, slotNo, page);
				}
				else
				{
					// Unmapping the memory page may cause seg faults.
					// The seg faults could be trapped but would probably
					// hamper perforance significantly.
					// So instead could use:
					// SetMMUslot(RomMemOffset, MmuTask, slotNo, page+4);
					// where page+4 would return a page + 32k into the ROM.
					// The ROM would need to be setup as 64k instead of 32k 
					// and the top 32k would be wasted (used) for this purpose.
					// For now leave it unmapped with no trapping and see how
					// often unmapped ROM pages are accessed.
					munmap(MMUbank[MmuTask][slotNo], CoCoPageSize);
					//MMUbank[MmuTask][slotNo] = NULL;
					MMUpages[MmuTask][slotNo] = 0xFFFF;
					// A better mechanism would be to have the pak device 
					// share its ROM with the mmu at initialisation time.
				}
				break;
			case 2: // 32K Internal
				page = slotNo - 4 + StateSwitch[CurrentRamConfig] + 8;
				SetMMUslot(MmuTask, slotNo, page);
				break;
			case 3: // 32 External
				munmap(MMUbank[MmuTask][slotNo], CoCoPageSize);
				//MMUbank[MmuTask][slotNo] = NULL;
				MMUpages[MmuTask][slotNo] = 0xFFFF;
				break;
			}
		}
	}

	// static char prevprtbuf[80];
	// char prtbuf[80];
	// sprintf(prtbuf, "%02x - %02x %02x %02x %02x %02x %02x %02x %02x", MmuTask,
	// 	MMUpages[0][0], MMUpages[0][1], MMUpages[0][2], MMUpages[0][3], 
	// 	MMUpages[0][4], MMUpages[0][5], MMUpages[0][6], MMUpages[0][7]);
	// if (strcmp(prtbuf, prevprtbuf))
	// {
	// 	fprintf(stderr, "%s\n", prtbuf);
	// 	strcpy(prevprtbuf, prtbuf);
	// }
}

unsigned char *Create32KROMMemory()
{
	size_t romsize = CoCoROMSize;

	// The ROM mem offset points to beyond the CoCo RAM memory size.
	// The ROM mem is appended to the physical mem in Create64KVirtualMemory()

    ptrCoCoRomMem = (PUINT8)mmap(NULL, romsize, PROT_READ | PROT_WRITE, MAP_SHARED, CoCoMemFD, RomMemOffset);
    
    if (ptrCoCoRomMem == MAP_FAILED)
    {
        printf("Cannot mmap ROM mem\n");
        return NULL;
    }

	return ptrCoCoRomMem;
}

unsigned char *Create64KVirtualMemory(unsigned int ramsize)
{
	CoCoMemFD = shm_open("/CoCoMem", O_RDWR | O_CREAT | O_EXCL, 0600);

	if (CoCoMemFD == -1)
	{
			fprintf(stderr, "shm_open failed\n");
			return NULL;
	}

	off_t totalramromsize = ramsize + CoCoROMSize;

	shm_unlink("/CoCoMem");
	ftruncate(CoCoMemFD, totalramromsize);

	ptrCoCoFullMem = (PUINT8)mmap(NULL, totalramromsize, PROT_READ | PROT_WRITE, MAP_SHARED, CoCoMemFD, 0);

	if (ptrCoCoFullMem == MAP_FAILED)
	{
		fprintf(stderr, "Cannot mmap main memory\n");
		return NULL;
	}

	// Task0 and Task1 Mem Offset will default to that defined in StateSwitch
	// 128k CoCo = Page 8
	// All other memsizes = Page 56

	__off_t memoffset = Task0MemOffset;
	size_t memsize = Mem64kSize;
	ptrCoCoTask0Mem = (PUINT8)mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, CoCoMemFD, memoffset);
	
	if (ptrCoCoTask0Mem == MAP_FAILED)
	{
			printf("Cannot mmap task 0 mem\n");
			return NULL;
	}

	ptrTasks[0] = ptrCoCoTask0Mem;

	memoffset = Task1MemOffset;
	memsize = Mem64kSize;
	ptrCoCoTask1Mem = (PUINT8)mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, CoCoMemFD, memoffset);
	
	if (ptrCoCoTask1Mem == MAP_FAILED)
	{
			printf("Cannot mmap task 1 mem\n");
			return NULL;
	}

	ptrTasks[1] = ptrCoCoTask1Mem;

	// Stamp the memory with 0x00FF

	for (int Index1 = 0; Index1<ramsize; Index1++)
	{
		*(ptrCoCoFullMem + Index1) = (Index1 & 1)-1;
	}

	return ptrCoCoFullMem;
}

void SetVectors(unsigned char data)
{
	RamVectors=!!data; //Bit 3 of $FF90 MC3
	return;
}

void SetMmuRegister(unsigned char Register,unsigned char data)
{	
	unsigned char BankRegister,Task;
	unsigned short bank;
	BankRegister = Register & 7;
	Task=!!(Register & 8);
	bank = MmuPrefix | (data & RamMask[CurrentRamConfig]);
	MmuRegisters[Task][BankRegister]=bank ; //gime.c returns what was written so I can get away with this

	UpdateMMap();

	return;
}

void SetRomMap(unsigned char data)
{	
	RomMap=(data & 3);
	UpdateMmuArray();
	UpdateMMap();
	return;
}

void SetMapType(unsigned char type)
{
	MapType=type;
	UpdateMmuArray();
	UpdateMMap();
	return;
}

void Set_MmuTask(unsigned char task)
{
	MmuTask=task;
	MmuState= (!MmuEnabled)<<1 | MmuTask;
	if (MmuEnabled) 
	{
		UpdateMMap();
		taskmemory = ptrTasks[MmuTask];
	}
	return;
}

void Set_MmuEnabled (unsigned char usingmmu)
{
	MmuEnabled=usingmmu;
	MmuState= (!MmuEnabled)<<1 | MmuTask;
	UpdateMMap();
	return;
}
 
unsigned char * Getint_rom_pointer(void)
{
	return(InternalRomBuffer);
}

void CopyRom(void)
{
	char ExecPath[MAX_PATH];
	unsigned short temp=0;
	temp=load_int_rom(BasicRomName());		//Try to load the image
	if (temp == 0)
	{	// If we can't find it use default copy
		AG_Strlcpy(ExecPath, GlobalExecFolder, sizeof(ExecPath));
		strcat(ExecPath, GetPathDelimStr());
		strcat(ExecPath, "coco3.rom");
		temp = load_int_rom(ExecPath);
	}
	if (temp == 0)
	{
		fprintf(stderr, "Missing file coco3.rom\n");
		exit(0);
	}
//		for (temp=0;temp<=32767;temp++)
//			InternalRomBuffer[temp]=CC3Rom[temp];
	return;
}

int load_int_rom(char filename[MAX_PATH])
{
	unsigned short index=0;
	FILE *rom_handle;
	rom_handle=fopen(filename,"rb");
	if (rom_handle==NULL)
		return(0);
	while ((feof(rom_handle)==0) & (index<0x8000))
		InternalRomBuffer[index++]=fgetc(rom_handle);

	fclose(rom_handle);
	return(index);
}

// Coco3 MMU Code
unsigned char MmuRead8(unsigned char bank, unsigned short address)
{
	return MemPages[bank][address & 0x1FFF];
}

void MmuWrite8(unsigned char data, unsigned char bank, unsigned short address)
{
	MemPages[bank][address & 0x1FFF] = data;
}

// Coco3 MMU Code
unsigned char MemRead8(unsigned short address)
{
	if (address<0xFE00)
	{
		if (MemPageOffsets[MmuRegisters[MmuState][address >> 13]] == 1)
			return(taskmemory[address]);
		return(PackMem8Read(MemPageOffsets[MmuRegisters[MmuState][address >> 13]] + (address & 0x1FFF)));
	}
	if (address>0xFEFF)
		return (port_read(address));
	if (RamVectors)	//Address must be $FE00 - $FEFF
		return(memory[(0x2000 * VectorMask[CurrentRamConfig]) | (address & 0x1FFF)]);
	if (MemPageOffsets[MmuRegisters[MmuState][address >> 13]] == 1)
		return(taskmemory[address]);
	return(PackMem8Read(MemPageOffsets[MmuRegisters[MmuState][address >> 13]] + (address & 0x1FFF)));
}

unsigned char MSABI MemRead8_s(unsigned short address)
{
	if (address<0xFE00)
	{
		if (MemPageOffsets[MmuRegisters[MmuState][address >> 13]] == 1)
			return(taskmemory[address]);
		return(PackMem8Read(MemPageOffsets[MmuRegisters[MmuState][address >> 13]] + (address & 0x1FFF)));
	}
	if (address>0xFEFF)
		return (port_read(address));
	if (RamVectors)	//Address must be $FE00 - $FEFF
		return(memory[(0x2000 * VectorMask[CurrentRamConfig]) | (address & 0x1FFF)]);
	if (MemPageOffsets[MmuRegisters[MmuState][address >> 13]] == 1)
		return(taskmemory[address]);
	return(PackMem8Read(MemPageOffsets[MmuRegisters[MmuState][address >> 13]] + (address & 0x1FFF)));
}

void MemWrite8(unsigned char data, unsigned short address)
{
	//	char Message[256]="";
	//	if ((address>=0xC000) & (address<=0xE000))
	//	{
	//		sprintf(Message,"Writing %i to ROM Address %x\n",data,address);
	//		WriteLog(Message,TOCONS);
	//	}
	if (address < 0xFE00)
	{
		if (MapType || (MmuRegisters[MmuState][address >> 13] < VectorMaska[CurrentRamConfig]) || (MmuRegisters[MmuState][address >> 13] > VectorMask[CurrentRamConfig]))
			taskmemory[address] = data;
		return;
	}
	if (address > 0xFEFF)
	{
		port_write(data, address);
		return;
	}
	if (RamVectors)	//Address must be $FE00 - $FEFF
		memory[(0x2000 * VectorMask[CurrentRamConfig]) | (address & 0x1FFF)] = data;
	else
		if (MapType || (MmuRegisters[MmuState][address >> 13] < VectorMaska[CurrentRamConfig]) || (MmuRegisters[MmuState][address >> 13] > VectorMask[CurrentRamConfig]))
			taskmemory[address] = data;
	return;
}

void MSABI MemWrite8_s(unsigned char data, unsigned short address)
{
	//	char Message[256]="";
	//	if ((address>=0xC000) & (address<=0xE000))
	//	{
	//		sprintf(Message,"Writing %i to ROM Address %x\n",data,address);
	//		WriteLog(Message,TOCONS);
	//	}
	if (address < 0xFE00)
	{
		if (MapType || (MmuRegisters[MmuState][address >> 13] < VectorMaska[CurrentRamConfig]) || (MmuRegisters[MmuState][address >> 13] > VectorMask[CurrentRamConfig]))
			taskmemory[address] = data;
		return;
	}
	if (address > 0xFEFF)
	{
		port_write(data, address);
		return;
	}
	if (RamVectors)	//Address must be $FE00 - $FEFF
		memory[(0x2000 * VectorMask[CurrentRamConfig]) | (address & 0x1FFF)] = data;
	else
		if (MapType || (MmuRegisters[MmuState][address >> 13] < VectorMaska[CurrentRamConfig]) || (MmuRegisters[MmuState][address >> 13] > VectorMask[CurrentRamConfig]))
			taskmemory[address] = data;
	return;
}

// unsigned char fMemRead8( unsigned short address)
// {
// 	if (address<0xFE00)
// 	{
// 		if (MemPageOffsets[MmuRegisters[MmuState][address>>13]]==1)
// 			return(MemPages[MmuRegisters[MmuState][address>>13]][address & 0x1FFF]);
// 		return( PackMem8Read( MemPageOffsets[MmuRegisters[MmuState][address>>13]] + (address & 0x1FFF) ));
// 	}
// 	if (address>0xFEFF)
// 		return (port_read(address));
// 	if (RamVectors)	//Address must be $FE00 - $FEFF
// 		return(memory[(0x2000*VectorMask[CurrentRamConfig])|(address & 0x1FFF)]); 
// 	if (MemPageOffsets[MmuRegisters[MmuState][address>>13]]==1)
// 		return(MemPages[MmuRegisters[MmuState][address>>13]][address & 0x1FFF]);
// 	return( PackMem8Read( MemPageOffsets[MmuRegisters[MmuState][address>>13]] + (address & 0x1FFF) ));
// }

// void fMemWrite8(unsigned char data,unsigned short address)
// {
// 	if (address<0xFE00)
// 	{
// 		if (MapType || (MmuRegisters[MmuState][address>>13] <VectorMaska[CurrentRamConfig]) || (MmuRegisters[MmuState][address>>13] > VectorMask[CurrentRamConfig]))
// 			MemPages[MmuRegisters[MmuState][address>>13]][address & 0x1FFF]=data;
// 		return;
// 	}
// 	if (address>0xFEFF)
// 	{
// 		port_write(data,address);
// 		return;
// 	}
// 	if (RamVectors)	//Address must be $FE00 - $FEFF
// 		memory[(0x2000*VectorMask[CurrentRamConfig])|(address & 0x1FFF)]=data;
// 	else
// 	if (MapType || (MmuRegisters[MmuState][address>>13] <VectorMaska[CurrentRamConfig]) || (MmuRegisters[MmuState][address>>13] > VectorMask[CurrentRamConfig]))
// 		MemPages[MmuRegisters[MmuState][address>>13]][address & 0x1FFF]=data;
// 	return;
// }
/*****************************************************************
* 16 bit memory handling routines                                *
*****************************************************************/

unsigned short MemRead16(unsigned short addr)
{
	return (MemRead8(addr)<<8 | MemRead8(addr+1));
}

unsigned short MSABI MemRead16_s(unsigned short addr)
{
	return (MemRead8_s(addr)<<8 | MemRead8_s(addr+1));
}

void MemWrite16(unsigned short data,unsigned short addr)
{
	MemWrite8( data >>8,addr);
	MemWrite8( data & 0xFF,addr+1);
	return;
}

void MSABI MemWrite16_s(unsigned short data,unsigned short addr)
{
	MemWrite8_s( data >>8,addr);
	MemWrite8_s( data & 0xFF,addr+1);
	return;
}

unsigned int MSABI MemRead32_s(unsigned short Address)
{
	return ( (MemRead16(Address)<<16) | MemRead16(Address+2) );
}

void MSABI MemWrite32_s(unsigned int data,unsigned short Address)
{
	MemWrite16( data>>16,Address);
	MemWrite16( data & 0xFFFF,Address+2);
	return;
}

unsigned int MemRead32(unsigned short Address)
{
	return ( (MemRead16(Address)<<16) | MemRead16(Address+2) );
}

void MemWrite32(unsigned int data,unsigned short Address)
{
	MemWrite16( data>>16,Address);
	MemWrite16( data & 0xFFFF,Address+2);
	return;
}

void SetDistoRamBank(unsigned char data)
{

	switch (CurrentRamConfig)
	{
	case 0:	// 128K
		return;
		break;
	case 1:	//512K
		return;
		break;
	case 2:	//2048K
		SetVideoBankSDL(data & 3);
		SetMmuPrefix(0);
		return;
		break;
	case 3:	//8192K	//No Can 3 
		SetVideoBankSDL(data & 0x0F);
		SetMmuPrefix( (data & 0x30)>>4);
		return;
		break;
	}
	return;
}

void SetMmuPrefix(unsigned char data)
{
	MmuPrefix=(data & 3)<<8;
	return;
}

void UpdateMmuArray(void)
{
	if (MapType)
	{
		MemPages[VectorMask[CurrentRamConfig]-3]=memory+(0x2000*(VectorMask[CurrentRamConfig]-3));
		MemPages[VectorMask[CurrentRamConfig]-2]=memory+(0x2000*(VectorMask[CurrentRamConfig]-2));
		MemPages[VectorMask[CurrentRamConfig]-1]=memory+(0x2000*(VectorMask[CurrentRamConfig]-1));
		MemPages[VectorMask[CurrentRamConfig]]=memory+(0x2000*VectorMask[CurrentRamConfig]);

		MemPageOffsets[VectorMask[CurrentRamConfig]-3]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]-2]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]-1]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]]=1;
		return;
	}
	switch (RomMap)
	{
	case 0:
	case 1:	//16K Internal 16K External
		MemPages[VectorMask[CurrentRamConfig]-3]=InternalRomBuffer;
		MemPages[VectorMask[CurrentRamConfig]-2]=InternalRomBuffer+0x2000;
		MemPages[VectorMask[CurrentRamConfig]-1]=NULL;
		MemPages[VectorMask[CurrentRamConfig]]=NULL;

		MemPageOffsets[VectorMask[CurrentRamConfig]-3]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]-2]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]-1]=0;
		MemPageOffsets[VectorMask[CurrentRamConfig]]=0x2000;
		return;
	break;

	case 2:	// 32K Internal
		MemPages[VectorMask[CurrentRamConfig]-3]=InternalRomBuffer;
		MemPages[VectorMask[CurrentRamConfig]-2]=InternalRomBuffer+0x2000;
		MemPages[VectorMask[CurrentRamConfig]-1]=InternalRomBuffer+0x4000;
		MemPages[VectorMask[CurrentRamConfig]]=InternalRomBuffer+0x6000;

		MemPageOffsets[VectorMask[CurrentRamConfig]-3]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]-2]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]-1]=1;
		MemPageOffsets[VectorMask[CurrentRamConfig]]=1;
		return;
	break;

	case 3:	//32K External
		MemPages[VectorMask[CurrentRamConfig]-1]=NULL;
		MemPages[VectorMask[CurrentRamConfig]]=NULL;
		MemPages[VectorMask[CurrentRamConfig]-3]=NULL;
		MemPages[VectorMask[CurrentRamConfig]-2]=NULL;

		MemPageOffsets[VectorMask[CurrentRamConfig]-1]=0;
		MemPageOffsets[VectorMask[CurrentRamConfig]]=0x2000;
		MemPageOffsets[VectorMask[CurrentRamConfig]-3]=0x4000;
		MemPageOffsets[VectorMask[CurrentRamConfig]-2]=0x6000;
		return;
	break;
	}
	return;
}

