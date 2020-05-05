/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

Additional material copyright 2019 by Walter Zambotti
This file is part of OVCC (Open Virtual Color Computer).

    OVCC and VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
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
#include "config.h"
#include "tcc1014graphicsSDL.h"
#include "pakinterface.h"
#include "logger.h"
#include "hd6309.h"
#include "fileops.h"
#include <sys/mman.h>
#include <fcntl.h>

#define handle_error(msg) do { perror(msg) ; exit(EXIT_FAILURE);} while (0)

static PUINT8 MemPages[1024];
static PUINT8 memory=NULL;				//Emulated RAM FULL 128-8096K
static PUINT8 taskmemory=NULL;			//Emulated RAM 64k
static PUINT8 vectormemory=NULL;		//Emulated RAM 64k
static PUINT8 InternalRomBuffer=NULL;   //Emulated Internal ROM 32k
static UINT8  MmuTask=0;				// $FF91 bit 0
static UINT8  MmuEnabled=0;				// $FF90 bit 6
static UINT8  RamVectors=0;				// $FF90 bit 3
static UINT8  MmuState=0;				// Composite variable handles MmuTask and MmuEnabled
static UINT8  RomMap=0;					// $FF90 bit 1-0
static UINT8  MapType=0;				// $FFDE/FFDF toggle Map type 0 = ram/rom
static UINT16 MmuRegisters[4][8];		// $FFA0 - FFAF
static UINT32 MemConfig[4]={0x20000,0x80000,0x200000,0x800000};
static UINT16 RamMask[4]={15,63,255,1023};
static UINT8  StateSwitch[4]={8,56,56,56};
static UINT8  VectorMask[4]={15,63,63,63};
static UINT8  VectorMaska[4]={12,60,60,60};
static UINT32 VidMask[4]={0x1FFFF,0x7FFFF,0x1FFFFF,0x7FFFFF};
static UINT8  CurrentRamConfig=1;
static UINT16 MmuPrefix=0;

#define MMUBankMax 2
#define MMUSlotMax 8
#define CoCoPageSize (8*1024)
#define Mem64kSize (64*1024)
#define CoCoROMSize (4*CoCoPageSize) 		// 32k for internal ROM + ...
#define CoCoPAKExtROMSize (4*CoCoPageSize)	//... 32k for external/PAK ROM
#define CoCoFullMemOffset 0
#define Task0MemOffset (StateSwitch[CurrentRamConfig]*CoCoPageSize)
#define Task1MemOffset (StateSwitch[CurrentRamConfig]*CoCoPageSize)
#define VideoMemOffset (StateSwitch[CurrentRamConfig]*CoCoPageSize)
#define RomMemOffset (MemConfig[CurrentRamConfig])
#define PAKExtMemOffset (RomMemOffset + CoCoROMSize)
#define TotalMMmemSize (MemConfig[CurrentRamConfig]+CoCoROMSize)

static int CoCoMemFD;

static PUINT8 ptrCoCoFullMem = NULL, ptrCoCoTask0Mem = NULL, ptrCoCoTask1Mem = NULL, ptrCoCoRomMem = NULL, ptrCoCoPakExtMem = NULL,
	ptrTasks[MMUBankMax],
	MMUbank[MMUBankMax][MMUSlotMax] = 
	{ 
		{ NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL }, 
		{ NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL }
	};

static UINT16 MMUpages[MMUBankMax][MMUSlotMax] =
{
	{ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }, 
	{ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }
};

static PUINT8 Create32KROMMemory();
static PUINT8 Create64KVirtualMemory(UINT32);
static void SetMMUslot(UINT8, UINT8, UINT16);

/*****************************************************************************************
* MmuInit Initilize and allocate memory for RAM Internal and External ROM Images.        *
* Copy Rom Images to buffer space and reset GIME MMU registers to 0                      *
* Returns NULL if any of the above fail.                                                 *
*****************************************************************************************/

static PUINT8 MmuInit_hw(UINT8  RamConfig)
{
	UINT32 RamSize=0;
	UINT32 Index1=0;
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

	vectormemory = ptrCoCoFullMem + (VectorMask[CurrentRamConfig] - 7) * CoCoPageSize;

	CopyRom();
	MmuReset();
	return(memory);
}

static void MmuReset_hw(void)
{
	UINT32 Index1=0,Index2=0;
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
		//MemPageOffsets[Index1]=1;
	}

	SetRomMap(0);
	SetMapType(0);
}

static void freePhysicalMemPages()
{
	for(int pageCnt = 0 ; pageCnt < MMUSlotMax ; pageCnt++)
	{
		if (MMUbank[0][pageCnt] != NULL) munmap(MMUbank[0][pageCnt], CoCoPageSize);
		if (MMUbank[1][pageCnt] != NULL) munmap(MMUbank[1][pageCnt], CoCoPageSize);
		MMUbank[0][pageCnt] = NULL;
		MMUbank[1][pageCnt] = NULL;
	}

	if(ptrCoCoRomMem != NULL) munmap(ptrCoCoRomMem, CoCoROMSize);
	if(ptrCoCoPakExtMem != NULL) munmap(ptrCoCoPakExtMem, CoCoPAKExtROMSize);
	if(ptrCoCoTask0Mem != NULL) munmap(ptrCoCoTask0Mem, Mem64kSize);
	if(ptrCoCoTask1Mem != NULL) munmap(ptrCoCoTask1Mem, Mem64kSize);
	if(ptrCoCoFullMem != NULL) munmap(ptrCoCoFullMem, TotalMMmemSize);

	ptrCoCoRomMem = NULL;
	ptrCoCoPakExtMem = NULL;
	ptrCoCoTask0Mem = NULL;
	ptrCoCoTask1Mem = NULL;
	ptrCoCoFullMem = NULL;
}

static void SetMMUslot(UINT8 task, UINT8 slotnum, UINT16 mempage)
{
	int memOffset;
	PUINT8 memPtr;

	if (MMUpages[task][slotnum] != mempage) // only bother the MMU if the page is different
	{
		memOffset = mempage * CoCoPageSize;
		memPtr = MMUbank[task][slotnum];
		/* unmap previous memory */
		// if (memPtr) munmap(memPtr, CoCoPageSize); /* attempted fix for memory leak, probably not needed */
		//fprintf(stderr, "task %d slot %d page %d mem %x", task, slotnum, mempage, memPtr);
		/* map new meory */
		MMUbank[task][slotnum] = (PUINT8)mmap(memPtr, CoCoPageSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, CoCoMemFD, memOffset);

		if ((void*)memPtr != (void*)MMUbank[task][slotnum])
		{
			char errmsg[128]; 
			sprintf(errmsg, "Failed to map page %d to mmu[%d][%d]\n",	mempage, task, slotnum);
			handle_error(errmsg);
			return;
		}

		MMUpages[task][slotnum] = mempage;
	}
}

static void UpdateMMap(void)
{
	UINT8 slotNo;
	UINT16 page;

	/*
		It may be worth trying to map ROM pages into RAM when attempting to map pages 3C-3F into the lower slots
		when the RomMap is set as in 32K internal mode. * note 1
	*/

	for (int slotNo = 0; slotNo < 8; slotNo++)
	{
		if (MapType || slotNo < 4) // memory pages are always memory below bank 4 or if MapType is RAM
		{
			if (MmuEnabled)
			{
				page = MmuRegisters[MmuTask][slotNo];
				//if (!MapType && page >= 0x3C && page <= 0x3F) page = RamMask[CurrentRamConfig] + slotNo - 3; // * note 1
				SetMMUslot(MmuTask, slotNo, page);
			}
			else  // map the default pages 
			{
				page = StateSwitch[CurrentRamConfig] + slotNo;
				SetMMUslot(MmuTask, slotNo, page);
			}
		}
		else // The bank is 4 and above and we are dealing with ROM
		{
			switch (RomMap)
			{
			case 0: //16K Internal 16K External
			case 1:
				if (slotNo < 6)
					page = RamMask[CurrentRamConfig] + slotNo - 3;
				else
					page = RamMask[CurrentRamConfig] + slotNo - 1;
				break;
			case 2: // 32K Internal
				page = RamMask[CurrentRamConfig] + slotNo - 3;
				break;
			case 3: // 32 External
				page = RamMask[CurrentRamConfig] + slotNo + 1;
				break;
			}
			SetMMUslot(MmuTask, slotNo, page);
		}
	}

	// static char prevprtbuf[80];
	// char prtbuf[80];
	// sprintf(prtbuf, "T%01x E%01x M%01x- %02x %02x %02x %02x %02x %02x %02x %02x", MmuState, MmuEnabled, MapType,
	// 	MMUpages[MmuTask][0], MMUpages[MmuTask][1], MMUpages[MmuTask][2], MMUpages[MmuTask][3], 
	// 	MMUpages[MmuTask][4], MMUpages[MmuTask][5], MMUpages[MmuTask][6], MMUpages[MmuTask][7]);
	// if (strcmp(prtbuf, prevprtbuf))
	// {
	// 	fprintf(stderr, "%s\n", prtbuf);
	// 	strcpy(prevprtbuf, prtbuf);
	// }
}

// void dumpMem(UINT16 addr, UINT16 len)
// {
// 	UINT16 i, t, ti;
// 	fprintf(stderr, "Dump mem %x task %d maptype %d rommap %d\n", taskmemory, MmuTask, MapType, RomMap);
// 	for(t = 0 ; t < 2 ; t++)
// 	{
// 		for(i = 0 ; i < len ; i++)
// 		{
// 			ti = (addr+i) % 0xffff;
// 			if ((i % 16) == 0) fprintf(stderr, "%08x  ", (int)ti);
// 			fprintf(stderr, "%02x ", (int)ptrTasks[t][ti]);
// 		}
// 		fprintf(stderr, "\n");
// 	}
// }

static unsigned char *Create32KROMMemory()
{
	// The ROM mem offset points to beyond the CoCo RAM memory size.
	// The ROM mem is appended to the physical mem in Create64KVirtualMemory()

    ptrCoCoRomMem = (PUINT8)mmap(NULL, CoCoROMSize, PROT_READ | PROT_WRITE, MAP_SHARED, CoCoMemFD, RomMemOffset);
    
    if (ptrCoCoRomMem == MAP_FAILED)
    {
        printf("Cannot mmap ROM mem\n");
        return NULL;
    }

	  ptrCoCoPakExtMem = (PUINT8)mmap(NULL, CoCoPAKExtROMSize, PROT_READ | PROT_WRITE, MAP_SHARED, CoCoMemFD, PAKExtMemOffset);
    
    if (ptrCoCoPakExtMem == MAP_FAILED)
    {
        printf("Cannot mmap PAK/external ROM mem\n");
        return NULL;
    }

	return ptrCoCoRomMem;
}

static PUINT8 Create64KVirtualMemory(UINT32 ramsize)
{
	CoCoMemFD = shm_open("/CoCoMem", O_RDWR | O_CREAT | O_EXCL, 0600);

	if (CoCoMemFD == -1)
	{
		fprintf(stderr, "shm_open failed\n");
		return NULL;
	}

	off_t totalramromsize = ramsize + CoCoROMSize + CoCoPAKExtROMSize;

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

	ptrCoCoTask0Mem = (PUINT8)mmap(NULL, Mem64kSize, PROT_READ | PROT_WRITE, MAP_SHARED, CoCoMemFD, Task0MemOffset);
	
	if (ptrCoCoTask0Mem == MAP_FAILED)
	{
		printf("Cannot mmap task 0 mem\n");
		return NULL;
	}

	ptrTasks[0] = ptrCoCoTask0Mem;

	ptrCoCoTask1Mem = (PUINT8)mmap(NULL, Mem64kSize, PROT_READ | PROT_WRITE, MAP_SHARED, CoCoMemFD, Task1MemOffset);
	
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

void SetVectors_hw(UINT8  data)
{
	RamVectors=!!data; //Bit 3 of $FF90 MC3
}

void SetMmuRegister_hw(UINT8  Register,UINT8  data)
{	
	UINT8  BankRegister,Task;
	UINT16 bank;
	BankRegister = Register & 7;
	Task=!!(Register & 8);
	bank = MmuPrefix | (data & RamMask[CurrentRamConfig]);
	MmuRegisters[Task][BankRegister]=bank ; //gime.c returns what was written so I can get away with this

	UpdateMMap();
}

void SetRomMap_hw(UINT8  data)
{	
	RomMap=(data & 3);
	UpdateMMap();
}

void SetMapType_hw(UINT8  type)
{
	MapType=type;
	UpdateMMap();
}

void Set_MmuTask_hw(UINT8  task)
{
	MmuTask=task;
	MmuState= (!MmuEnabled)<<1 | MmuTask;
	if (MmuEnabled) 
	{
		UpdateMMap();
	}
	taskmemory = ptrTasks[MmuTask];
}

void Set_MmuEnabled_hw(UINT8  usingmmu)
{
	MmuEnabled=usingmmu;
	MmuState= (!MmuEnabled)<<1 | MmuTask;
	UpdateMMap();
}
 
PUINT8  Getint_rom_pointer_hw(void)
{
	return(InternalRomBuffer);
}

PUINT8 GetPakExtMem()
{
	return ptrCoCoPakExtMem;
}

void CopyRom_hw(void)
{
	char ExecPath[MAX_PATH];
	UINT16 temp=0;
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
}

static int load_int_rom(char filename[MAX_PATH])
{
	UINT16 index=0;
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

UINT8  MmuRead8_hw(UINT8  bank, UINT16 address)
{
	return MemPages[bank][address & 0x1FFF];
}

void MmuWrite8_hw(UINT8  data, UINT8  bank, UINT16 address)
{
	MemPages[bank][address & 0x1FFF] = data;
}

// Coco3 MMU Code

UINT8  MemRead8_hw(UINT16 address)
{
	if (address<0xFE00)
	{
		return(taskmemory[address]);
	}
	if (address>0xFEFF)
		return (port_read(address));
	if (RamVectors)	//Address must be $FE00 - $FEFF
		return(vectormemory[address]);
	return(taskmemory[address]);
}	

void MemWrite8_hw(UINT8  data, UINT16 address)
{
	if (address < 0xFE00)
	{
		taskmemory[address] = data;
		return;
	}
	if (address > 0xFEFF)
	{
		port_write(data, address);
		return;
	}
	if (RamVectors)	//Address must be $FE00 - $FEFF
		vectormemory[address] = data;
	else
		taskmemory[address] = data;
}

void SetDistoRamBank_hw(UINT8  data)
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

static void SetMmuPrefix(UINT8  data)
{
	MmuPrefix=(data & 3)<<8;
	return;
}

void SetHWMmu()
{
	MmuInit=MmuInit_hw;
	MmuReset=MmuReset_hw;
	SetVectors=SetVectors_hw;
	SetMmuRegister=SetMmuRegister_hw;
	SetRomMap=SetRomMap_hw;
	SetMapType=SetMapType_hw;
	Set_MmuTask=Set_MmuTask_hw;
	Set_MmuEnabled=Set_MmuEnabled_hw;
	Getint_rom_pointer=Getint_rom_pointer_hw;
	CopyRom=CopyRom_hw;
	MmuRead8=MmuRead8_hw;
	MmuWrite8=MmuWrite8_hw;
	MemRead8=MemRead8_hw;
	MemWrite8=MemWrite8_hw;
	SetDistoRamBank=SetDistoRamBank_hw;
	SetMMUStat(1);
}
