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

#include <agar/core.h>
#include <stdio.h>
#include <stdbool.h>
#include "defines.h"
#include "fileops.h"

static char ExecFolder[MAX_PATH];

void ValidatePath(char *Path)
{
	char TempPath[MAX_PATH]="";
	int tpl;

	if (ExecFolder[0] == 0) getcwd(ExecFolder, sizeof(ExecFolder));

	strcpy(TempPath,Path);			
	PathRemoveFileSpec(TempPath);		//Get path to Incomming file
	tpl = strlen(ExecFolder);

	if (!strncmp(TempPath, ExecFolder, tpl))	// If they match remove the Path
	{
		strcpy(Path, &(Path[++tpl]));
		//PathStripPath(Path);
	}
	return;
}

int CheckPath( char *Path)	//Return 1 on Error
{
	char TempPath[MAX_PATH]="";

	if (ExecFolder[0] == 0) getcwd(ExecFolder, sizeof(ExecFolder));

	if ((strlen(Path)==0) | (strlen(Path) > MAX_PATH))
		return(1);
	
	if (!AG_FileExists(Path)) //File Doesn't exist
	{
		strcpy(TempPath, ExecFolder);

		if ( (strlen(TempPath)) + (strlen(Path)) > MAX_PATH)	//Resulting path is to large Bail.
			return(1);

		strcat(TempPath, Path);
		
		if (!AG_FileExists(TempPath))
			return(1);

		strcpy(Path,TempPath);
	}

	return(0);
}

char GetPathDelim()
{
	static char dirdelim = 0;

	if (dirdelim == 0)
	{
		char *platform = SDL_GetPlatform();

		if (!strcmp(platform, "Windows"))
		{
			dirdelim = '\\';
		}
		else 
		{
			dirdelim = '/';
		}
	}

	return dirdelim;
}

char *GetPathDelimStr()
{
	static char PathDelimString[2] = " ";

	PathDelimString[0] = GetPathDelim();

	return PathDelimString;
}

// These are here to remove dependance on shlwapi.dll. ASCII only
void PathStripPath (char *TextBuffer)
{
	char dirdelim = GetPathDelim();

	char TempBuffer[MAX_PATH] = "";
	short Index = (short)strlen(TextBuffer);

	if ((Index > MAX_PATH) | (Index==0))	//Test for overflow
		return;

	for (; Index >= 0; Index--)
		if (TextBuffer[Index] == dirdelim)
			break;
	
	if (Index < 0)	//delimiter not found
		return;
	strcpy(TempBuffer, &TextBuffer[Index + 1]);
	strcpy(TextBuffer, TempBuffer);
}

BOOL PathRemoveFileSpec(char *Path)
{
	char dirdelim = GetPathDelim();

	int Index=strlen(Path),Lenth=Index;
	if ( (Index==0) | (Index > MAX_PATH))
		return(false);
	
	while ( (Index>0) & (Path[Index] != dirdelim))
		Index--;
	while ( (Index>0) & (Path[Index] == dirdelim))
		Index--;
	if (Index<0)
		return(false);
	Path[Index+1]=0;
	return( !(strlen(Path) == Lenth));
}		

BOOL PathRemoveExtension(char *Path)
{
	size_t Index=strlen(Path),Lenth=Index;
	if ( (Index==0) | (Index > MAX_PATH))
		return(false);
	
	while ( (Index>0) & (Path[Index--] != '.') );
	Path[Index+1]=0;
	return( !(strlen(Path) == Lenth));
}

char* PathFindExtension(char *Path)
{
	size_t Index=strlen(Path),Lenth=Index;
	if ( (Index==0) | (Index > MAX_PATH))
		return(&Path[strlen(Path)+1]);
	while ( (Index>0) & (Path[Index--] != '.') );
	return(&Path[Index+1]);
}