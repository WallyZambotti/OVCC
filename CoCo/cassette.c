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

#include "resource.h"
#include "defines.h"
#include "coco3.h"
#include "config.h"
#include "cassette.h"
#include "stdio.h"
#include "logger.h"

static unsigned char MotorState=0,TapeMode=STOP,WriteProtect=0,Quiet=30;
static AG_DataSource *TapeHandle=NULL;
static unsigned long TapeOffset=0,TotalSize=0;
static char TapeFileName[MAX_PATH]="";
static unsigned char TempBuffer[8192];
static unsigned char *CasBuffer=NULL;
static char FileType=0;
unsigned long BytesMoved=0;
static unsigned int TempIndex=0;

unsigned char One[21]={0x80,0xA8,0xC8,0xE8,0xE8,0xF8,0xF8,0xE8,0xC8,0xA8,0x78,0x50,0x50,0x30,0x10,0x00,0x00,0x10,0x30,0x30,0x50};
unsigned char Zero[40]={0x80,0x90,0xA8,0xB8,0xC8,0xD8,0xE8,0xE8,0xF0,0xF8,0xF8,0xF8,0xF0,0xE8,0xD8,0xC8,0xB8,0xA8,0x90,0x78,0x78,0x68,0x50,0x40,0x30,0x20,0x10,0x08,0x00,0x00,0x00,0x08,0x10,0x10,0x20,0x30,0x40,0x50,0x68,0x68};

//Write Stuff
static int LastTrans=0;
static unsigned char Mask=0,Byte=0,LastSample=0;
//****************************

void WavtoCas(unsigned char *,unsigned int);
int MountTape( char *);
void CloseTapeFile(void);
void SyncFileBuffer (void);
void CastoWav(unsigned char *,unsigned int,unsigned long *);

void Motor(unsigned char State)
{
	MotorState=State;
	switch (MotorState)
	{
		case 0:
			SetSndOutMode(0);
			switch (TapeMode)
			{
				case STOP:

				break;

				case PLAY:
					Quiet=30;
					TempIndex=0;
				break;

				case REC:
					SyncFileBuffer();
				break;

				case EJECT:

				break;
			}
		break;	//MOTOROFF

		case 1:
			switch (TapeMode)
			{
				case STOP:
					SetSndOutMode(0);
				break;

				case PLAY:
					SetSndOutMode(2);
				break;

				case REC:
					SetSndOutMode(1);
				break;

				case EJECT:
					SetSndOutMode(0);
			}
		break;	//MOTORON	
	}
	return;
}

unsigned int GetTapeCounter(void)
{
	return(TapeOffset);
}

void SetTapeCounter(unsigned int Count)
{
	TapeOffset=Count;
	if (TapeOffset>TotalSize)
		TotalSize=TapeOffset;
	UpdateTapeCounter(TapeOffset,TapeMode);
	return;
}

void SetTapeMode(unsigned char Mode)	//Handles button pressed from Dialog
{
	TapeMode=Mode;
	switch (TapeMode)
	{
		case STOP:

		break;

		case PLAY:
			if (TapeHandle==NULL)
				//if (!LoadTape())
					TapeMode=STOP;
				else
					TapeMode=Mode;

			if (MotorState)
				Motor(1);
		break;

		case REC:
			if (TapeHandle==NULL)
				//if (!LoadTape())
					TapeMode=STOP;
				else
					TapeMode=Mode;
		break;

		case EJECT:
			CloseTapeFile();
			strcpy(TapeFileName,"EMPTY");
		break;
	}
	UpdateTapeCounter(TapeOffset,TapeMode);
	return;
}

void FlushCassetteBuffer(unsigned char *Buffer,unsigned int Lenth)
{
	if (TapeMode!=REC) 
		return;

	switch(FileType)
	{
	case WAV:
		AG_Seek(TapeHandle, TapeOffset+44, AG_SEEK_SET);
		AG_WriteP(TapeHandle, Buffer, (size_t)Lenth, &BytesMoved);
		if (Lenth!=BytesMoved)
			return;
		TapeOffset+=Lenth;
		if (TapeOffset>TotalSize)
			TotalSize=TapeOffset;
	break;

	case CAS:
		WavtoCas(Buffer, Lenth);
		break;
	}
	UpdateTapeCounter(TapeOffset,TapeMode);
	return;
}

void LoadCassetteBuffer(unsigned char *CassBuffer)
{
	unsigned long BytesMoved=0;
	if (TapeMode!=PLAY)
		return;
	switch (FileType)
	{
	case WAV:
		AG_Seek(TapeHandle, TapeOffset+44, AG_SEEK_SET);
		AG_ReadP(TapeHandle, CassBuffer, (size_t)TAPEAUDIORATE/60, &BytesMoved);
		TapeOffset+=BytesMoved;
		if (TapeOffset>TotalSize)
			TapeOffset=TotalSize;
	break;

	case CAS:
		CastoWav(CassBuffer,TAPEAUDIORATE/60,&BytesMoved);
	break;
	}

	UpdateTapeCounter(TapeOffset,TapeMode);
	return;
}

int MountTape( char *FileName)	//Return 1 on sucess 0 on fail
{
	char Extension[4]="";
	unsigned char Index=0;
	if (TapeHandle!=NULL)
	{
		TapeMode=STOP;
		CloseTapeFile();
	}

	WriteProtect=0;
	FileType=0;	//0=wav 1=cas
	TapeHandle = AG_OpenFile(FileName, "r+");
	if (TapeHandle == NULL)//Can't open read/write. try read only
	{
		TapeHandle = AG_OpenFile(FileName, "r");
		WriteProtect=1;
	}
	if (TapeHandle == NULL)
	{
		_MessageBox("Can't Mount Tape File");
		return(0);	//Give up
	}
	AG_Seek(TapeHandle, 0, AG_SEEK_END);
	TotalSize = AG_Tell(TapeHandle);
	TapeOffset=0;
	strcpy(Extension,&FileName[strlen(FileName)-3]);
	for (Index=0;Index<strlen(Extension);Index++)
		Extension[Index]=toupper(Extension[Index]);

	if (strcmp(Extension,"WAV"))
	{
		FileType=CAS;
		LastTrans=0;
		Mask=0;
		Byte=0;	
		LastSample=0;
		TempIndex=0;
		if (CasBuffer!=NULL)
			free(CasBuffer);

		CasBuffer=(unsigned char *)malloc(WRITEBUFFERSIZE);
		AG_Seek(TapeHandle, 0, AG_SEEK_SET);
		AG_ReadP(TapeHandle, CasBuffer, TotalSize, &BytesMoved);
		if (BytesMoved!=TotalSize)
			return(0);
	}
	return(1);
}

void CloseTapeFile(void)
{
	if (TapeHandle==NULL)
		return;
	SyncFileBuffer();
	AG_CloseDataSource(TapeHandle);
	TapeHandle=NULL;
	TotalSize=0;
}

void GetTapeName(char *Name)
{
	strcpy(Name,TapeFileName);
	return;
}

void SyncFileBuffer (void)
{
	char Buffer[64]="";
	unsigned long BytesMoved=0;
	unsigned int FileSize=TotalSize+40-8;
	unsigned short WaveType=1;		//WAVE type format
	unsigned int FormatSize=16;		//size of WAVE section chunck
	unsigned short Channels=1;		//mono/stereo
	unsigned int BitRate=TAPEAUDIORATE;		//sample rate
	unsigned short BitsperSample=8;	//Bits/sample
	unsigned int BytesperSec=BitRate*Channels*(BitsperSample/8);		//bytes/sec
	unsigned short BlockAlign=(BitsperSample * Channels)/8;		//Block alignment
	unsigned int ChunkSize=FileSize;

	AG_Seek(TapeHandle, 0, AG_SEEK_SET);
	switch (FileType)
	{
	case CAS:
		CasBuffer[TapeOffset]=Byte;	//capture the last byte
		LastTrans=0;	//reset all static inter-call variables
		Mask=0;
		Byte=0;	
		LastSample=0;
		TempIndex=0;
		AG_WriteP(TapeHandle, CasBuffer, (size_t)TapeOffset, &BytesMoved);
	break;

	case WAV:
		sprintf(Buffer,"RIFF");
		AG_WriteP(TapeHandle, Buffer, 4, &BytesMoved);
		AG_WriteP(TapeHandle, &FileSize, 4, &BytesMoved);
		sprintf(Buffer,"WAVE");
		AG_WriteP(TapeHandle, Buffer, 4, &BytesMoved);
		sprintf(Buffer,"fmt ");
		AG_WriteP(TapeHandle, Buffer, 4, &BytesMoved);
		AG_WriteP(TapeHandle, &FormatSize, 4, &BytesMoved);
		AG_WriteP(TapeHandle, &WaveType, 2, &BytesMoved);
		AG_WriteP(TapeHandle, &Channels, 2, &BytesMoved);
		AG_WriteP(TapeHandle, &BitRate, 4, &BytesMoved);
		AG_WriteP(TapeHandle, &BytesperSec, 4, &BytesMoved);
		AG_WriteP(TapeHandle, &BlockAlign, 2, &BytesMoved);
		AG_WriteP(TapeHandle, &BitsperSample, 2, &BytesMoved);
		sprintf(Buffer,"data");
		AG_WriteP(TapeHandle, Buffer, 4, &BytesMoved);
		AG_WriteP(TapeHandle, &ChunkSize, 4, &BytesMoved);
	break;
	}
	
	//FlushFileBuffers(TapeHandle);
	return;
}


void CastoWav(unsigned char *Buffer,unsigned int BytestoConvert,unsigned long *BytesConverted)
{	
	unsigned char Byte=0;
	char Mask=0;

	if (Quiet>0)
	{
		Quiet--;
		memset(Buffer,0,BytestoConvert);
		return;
	}

	if ((TapeOffset>TotalSize) | (TotalSize==0))	//End of tape return nothing
	{
		memset(Buffer,0,BytestoConvert);
		TapeMode=STOP;	//Stop at end of tape
		return;
	}

	while ((TempIndex<BytestoConvert) & (TapeOffset<=TotalSize))
	{
		Byte=CasBuffer[(TapeOffset++)%TotalSize];
		for (Mask=0;Mask<=7;Mask++)
		{
			if ((Byte & (1<<Mask))==0)
			{
				memcpy(&TempBuffer[TempIndex],Zero,40);
				TempIndex+=40;
			}
			else
			{
				memcpy(&TempBuffer[TempIndex],One,21);
				TempIndex+=21;
			}
		}
	}
	
	if (TempIndex>=BytestoConvert)
	{
		memcpy(Buffer,TempBuffer,BytestoConvert);									//Fill the return Buffer
		memcpy (TempBuffer, &TempBuffer[BytestoConvert],TempIndex-BytestoConvert);	//Slide the overage to the front
		TempIndex-=BytestoConvert;													//Point to the Next free byte in the tempbuffer
	}
	else	//We ran out of source bytes
	{
		memcpy(Buffer,TempBuffer,TempIndex);						//Partial Fill of return buffer;
		memset(&Buffer[TempIndex],0,BytestoConvert-TempIndex);		//and silence for the rest
		TempIndex=0;
	}
	return;
}


void WavtoCas(unsigned char *WaveBuffer,unsigned int Lenth)
{
	unsigned char Bit=0,Sample=0;
	unsigned int Index=0,Width=0;

	for (Index=0;Index<Lenth;Index++)
	{
		Sample=WaveBuffer[Index];
		if ( (LastSample <= 0x80) & (Sample>0x80))	//Low to High transition
		{
			Width=Index-LastTrans;
			if ((Width <10) | (Width >50))	//Invalid Sample Skip it
			{
				LastSample=0;
				LastTrans=Index;
				Mask=0;
				Byte=0;
			}
			else
			{
				Bit=1;
				if (Width >30)
					Bit=0;
				Byte=Byte | (Bit<<Mask);
				Mask++;
				Mask&=7;
				if (Mask==0)
				{
					CasBuffer[TapeOffset++]=Byte;
					Byte=0;
					if (TapeOffset>= WRITEBUFFERSIZE)	//Don't blow past the end of the buffer
						TapeMode=STOP;
				}

			}
			LastTrans=Index;
		} //Fi LastSample
		LastSample=Sample;
	}	//Next Index
	LastTrans-=Lenth;
	if (TapeOffset>TotalSize)
		TotalSize=TapeOffset;
	return;
}
