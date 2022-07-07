/*
Copyright 2019 Walter ZAMBOTTI
This file is part of iniman

    iniman is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    iniman is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with iniman.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <stdbool.h>
#include "iniman.h"

#define MAX_LINE_LEN 512

static INIman _iniman = { NULL, 0, -1 };
static INIman *iniman = NULL;

static int readline(FILE *fp, char *bp)
{   
    int c = '\0';
    int i = 0;

    while(((c = getc(fp)) != '\n') && (c != EOF))
    {   
        bp[i++] = c;
    }

    bp[i] = '\0';
    return(c != EOF);
}

#define SECTIONBUCKETSIZE 128
#define ENTRYBUCKETSIZE 32
#define FILEBUCKETSIZE 8

static INIfile *recordFile(char *filename)
{
    if (iniman->fileCnt % FILEBUCKETSIZE == 0) 
    {
        iniman->files = realloc(iniman->files, FILEBUCKETSIZE * sizeof(INIman) * (int)(iniman->fileCnt/FILEBUCKETSIZE+1));
    }

    iniman->files[iniman->fileCnt].name = strdup(filename);
    iniman->files[iniman->fileCnt].sections = NULL;
    iniman->files[iniman->fileCnt].sectionCnt = 0;
    iniman->files[iniman->fileCnt].file = NULL;
    iniman->files[iniman->fileCnt].backup = false;
    iniman->lastfile = iniman->fileCnt;

    return &iniman->files[iniman->fileCnt++];
}

static INIsection *recordSection(INIfile *inifile, char *section)
{
    if (inifile->sectionCnt % SECTIONBUCKETSIZE == 0) 
    {
        inifile->sections = realloc(inifile->sections, SECTIONBUCKETSIZE * sizeof(INIsection) * (int)(inifile->sectionCnt/SECTIONBUCKETSIZE+1));
    }
        
    inifile->sections[inifile->sectionCnt].name = strdup(section);
    inifile->sections[inifile->sectionCnt].entries = NULL;
    inifile->sections[inifile->sectionCnt].entryCnt = 0;

    return &inifile->sections[inifile->sectionCnt++];
}

static void *recordEntry(INIsection *section, char *name, char *value)
{
    if (section->entryCnt % ENTRYBUCKETSIZE == 0)
    {
        section->entries = realloc(section->entries, ENTRYBUCKETSIZE * sizeof(INIentry) * (int)(section->entryCnt/ENTRYBUCKETSIZE+1));
    }

    section->entries[section->entryCnt].name = strdup(name);
    section->entries[section->entryCnt].value = strdup(value);

    return &section->entries[section->entryCnt++];
}

static void BackupINIfile(INIfile *inifile)
{
    if (!inifile->backup) return;

#define BUF_SIZE 1024

    FILE *inputFd, *outputFd;
    size_t numRead;
    char buf[BUF_SIZE];
    char backupininame[256];

    /* Open input and output files */

    inputFd = fopen(inifile->name, "rb");
    if (inputFd == NULL)
    {
        printf("iniman : backup error opening ini file %s", inifile->name);
    }

    strcpy(backupininame, inifile->name);
    strcat(backupininame, "_bck");

    outputFd = fopen(backupininame, "wb");
    if (outputFd == NULL)
    {
        printf("iniman : backup error opening backup file %s", backupininame);
    }

    /* Transfer data until we encounter end of input or an error */

    while ((numRead = fread(buf, 1, BUF_SIZE, inputFd)) > 0)
    {
        if (fwrite(buf, 1, numRead, outputFd) != numRead)
        {
            printf("iniman : error while writing backup\n");
        }
    }

    if (ferror(outputFd))
    {
        printf("iniman : error while writing backup\n");
    }

    if (fclose(inputFd))
    {
        printf("iniman : error closing ini\n");
    }

    if (fclose(outputFd) == -1)
    {
        printf("iniman : error closing backup\n");
    }

    inifile->backup = false; // Only backup once!
}

static bool freeINIfile(INIfile *inifile)
{
    int isection, ientry;

    if (inifile == NULL) return false;

    for(isection = 0 ; isection < inifile->sectionCnt ; isection++)
    {
        for(ientry = 0 ; ientry < inifile->sections[isection].entryCnt ; ientry++)
        {
            if (inifile->sections[isection].entries[ientry].name != NULL)
            {
                free(inifile->sections[isection].entries[ientry].name);
                inifile->sections[isection].entries[ientry].name = NULL;
            }

            if (inifile->sections[isection].entries[ientry].value != NULL)
            {
                free(inifile->sections[isection].entries[ientry].value);        
                inifile->sections[isection].entries[ientry].value = NULL;        
            }
        }

        if (inifile->sections[isection].name)
        {
            free(inifile->sections[isection].name);
            inifile->sections[isection].name = NULL;
        }
        if (inifile->sections[isection].entries != 0)
        {
            free(inifile->sections[isection].entries);
            inifile->sections[isection].entries = NULL;
        }
        inifile->sections[isection].entryCnt = 0;
    }

    if (inifile->name != NULL)
    {
        free(inifile->name);
        inifile->name = NULL;
    }

    if (inifile->sections != NULL)
    {
        free(inifile->sections);
        inifile->sections = NULL;
    }
    
    inifile->sectionCnt = 0;
    return true;
}

static bool saveINIfile(INIfile *inifile, bool freemem)
{
    int isection, ientry;

    if (inifile == NULL) return false;

    if (freemem) BackupINIfile(inifile);

    if ((inifile->file = fopen(inifile->name, "w")) == NULL)
    {
        return false;
    }

    for(isection = 0 ; isection < inifile->sectionCnt ; isection++)
    {
        if (inifile->sections[isection].name != NULL)
        {
            fprintf(inifile->file, "%s\n", inifile->sections[isection].name); 
        }

        for(ientry = 0 ; ientry < inifile->sections[isection].entryCnt ; ientry++)
        {
            if (inifile->sections[isection].entries[ientry].name != NULL)
            {
                fprintf(inifile->file, "%s=%s\n", 
                    inifile->sections[isection].entries[ientry].name,
                    inifile->sections[isection].entries[ientry].value);

            }
        }
    }

    fclose(inifile->file);

    if (freemem)
    {
        freeINIfile(inifile);
    }
    
    return true;
}

static INIfile *searchFile(char *filename)
{
    int ifile;

    for (ifile = 0 ; ifile < iniman->fileCnt ; ifile++)
    {
        if (iniman->files[ifile].name != NULL && !strcmp(iniman->files[ifile].name, filename))
        {
            iniman->lastfile = ifile;
            return (&iniman->files[ifile]);
        }
    }

    return NULL;
}

static bool INImanInitialised()
{
    if (iniman == NULL)
    {
        printf("iniman : Not Initialised!\n");
        return false;
    }

    return true;
}

static INIfile *loadINIfile(char *name, bool DoNotAutoCreate)
{
    char iniline[MAX_LINE_LEN];
    char dummy[1] = "";
    regex_t sectexp;
    regex_t entryexp;
    regmatch_t entparts[4];
    INIsection *section;

    if (!INImanInitialised()) return NULL;

    INIfile *inifile = searchFile(name);

    if (inifile != NULL || DoNotAutoCreate) return inifile;

    inifile = recordFile(name);

    if (inifile == NULL) return NULL;

    inifile->file = fopen(inifile->name, "r");

    if (inifile->file == NULL)
    {
        inifile->file = fopen(inifile->name, "w");

        if (inifile->file == NULL)
        {
            return NULL;
        }

        inifile->sectionCnt = 0;
    }

    regcomp(&sectexp, "^\\[[^[]*\\]", REG_EXTENDED | REG_NOSUB);
    regcomp(&entryexp, "(^[^=]*)(=)(.*)", REG_EXTENDED);

    while (readline(inifile->file, iniline)) 
    {
        if (!regexec(&sectexp, iniline, 0, NULL, 0))
        {
            section = recordSection(inifile, iniline);
        }
        else if (!regexec(&entryexp, iniline, 4, entparts, 0))
        {
            iniline[entparts[1].rm_eo] = '\0';
            char *entname = &iniline[entparts[1].rm_so];
            char *entval;

            if (entparts[3].rm_so != -1)
            {
                iniline[entparts[3].rm_eo] = '\0';
                entval = &iniline[entparts[3].rm_so];
            }
            else
            {
                entval = strdup(dummy);
            }

            recordEntry(section, entname, entval);
        }
    }

    fclose(inifile->file);

    return inifile;
}

static INIsection *searchSection(INIfile *inifile, char *section)
{
    int isection;

    for (isection = 0 ; isection < inifile->sectionCnt ; isection++)
    {
        if (! strcmp (inifile->sections[isection].name, section))
        {
            return (&inifile->sections[isection]);
        }
    }

    return NULL;
}

static INIentry *searchEntry(INIsection *section, char *entry)
{
    int ientry;

    for (ientry = 0 ; ientry < section->entryCnt ; ientry++)
    {
        if (! strcmp (section->entries[ientry].name, entry))
        {
            return(&section->entries[ientry]);
        }
    }

    return NULL;
}

int GetPrivateProfileString(char *section, char *entry, char *defaultval, char *buffer, int bufferlen, char *filename)
{
    INIfile *inifile = loadINIfile(filename, false);

    if (inifile == NULL) 
    {
        fprintf(stderr, "iniman : cannot load inifile %d : %s\n", errno, filename);
        return -1;
    }

    char bracedsection[256];

    sprintf(bracedsection, "[%s]", section);
    INIsection *sectionp = searchSection(inifile, bracedsection);

    if (sectionp == NULL)
    {
        strncpy(buffer, defaultval, bufferlen);
        return strlen(buffer);
    }

    INIentry *entryp = searchEntry(sectionp, entry);

    if (entryp == NULL)
    {
        strncpy(buffer, defaultval, bufferlen);
        return strlen(buffer);
    }

    strncpy(buffer, entryp->value, bufferlen);
    return strlen(buffer);
}

int WritePrivateProfileString(char *section, char *entry, char *value, char *filename)
{
    INIfile *inifile = loadINIfile(filename, false);

    if (inifile == NULL) 
    {
        fprintf(stderr, "iniman : cannot load inifile %d : %s\n", errno, filename);
        return -1;
    }

    char bracedsection[256];

    sprintf(bracedsection, "[%s]", section);
    INIsection *sectionp = searchSection(inifile, bracedsection);

    if (sectionp == NULL) // New Section and New Entry
    {
        sectionp = recordSection(inifile, bracedsection);
        recordEntry(sectionp, entry, value);
        return 1;
    }
    
    INIentry *entryp = searchEntry(sectionp, entry);

    if (entryp == NULL) // New entry
    {
        recordEntry(sectionp, entry, value);
        return 1;
    }

    // Update old entry

    free(entryp->value);
    entryp->value = strdup(value);
        
    return 1;
}

int GetPrivateProfileInt(char *section, char *entry, int defaultval, char *filename)
{
    char defval[64];
    char buffer[128];

    sprintf(defval, "%d", defaultval);
    GetPrivateProfileString(section, entry, defval, buffer, sizeof(buffer), filename);

    return(atoi(buffer));
}

int WritePrivateProfileInt(char *section, char *entry, int defaultval, char *filename)
{
	char buffer[64]="";
	sprintf(buffer, "%i", defaultval);
	return(WritePrivateProfileString(section, entry, buffer, filename));
}

bool DeletePrivateProfileEntry(char *section, char *entry, char *filename)
{
    INIfile *inifile = loadINIfile(filename, true);

    if (inifile == NULL) 
    {
        fprintf(stderr, "iniman : cannot load inifile %d : %s\n", errno, filename);
        return false;
    }

    char bracedsection[256];

    sprintf(bracedsection, "[%s]", section);
    INIsection *sectionp = searchSection(inifile, bracedsection);

    if (sectionp == NULL)
    {
        return false;
    }

    INIentry *entryp = searchEntry(sectionp, entry);

    if (entryp == NULL) // New entry
    {
        return false;
    }

    // Remove old entry

    free(entryp->name);
    entryp->name = NULL;

    return true;
}

bool DeletePrivateProfileSection(char *section, char *filename)
{
    INIfile *inifile = loadINIfile(filename, true);

    if (inifile == NULL) 
    {
        fprintf(stderr, "iniman : cannot load inifile %d : %s\n", errno, filename);
        return -1;
    }

    char bracedsection[256];

    sprintf(bracedsection, "[%s]", section);
    INIsection *sectionp = searchSection(inifile, bracedsection);

    if (sectionp == NULL)
    {
        return false;
    }

    for(int ientry = 0 ; ientry < sectionp->entryCnt ; ientry++)
    {
        if (sectionp->entries[ientry].name != NULL)
        {
            free(sectionp->entries[ientry].name);
        }

        sectionp->entries[ientry].name = NULL;
    }

    free(sectionp->name);
    sectionp->name = NULL;

    return true;
}

bool DuplicatePrivateProfile(char *filename, char *newfilename)
{
    INIfile *inifile = loadINIfile(filename, true);

    if (inifile == NULL) 
    {
        fprintf(stderr, "iniman : cannot load inifile %d : %s\n", errno, filename);
        return false;
    }

    INIfile *newinifile = searchFile(newfilename);

    if (newinifile != NULL)
    {
        if (newinifile == inifile)
        {
            fprintf(stderr, "iniman : cannot duplicate inifile to same inifile : %s\n", filename);
            return false;
        }
        freeINIfile(newinifile);
    }
    else
    {
        newinifile = recordFile(newfilename);
    }
    
    int isection, ientry;
    size_t size;

    newinifile->sectionCnt = inifile->sectionCnt;

    if (inifile->sectionCnt == 0) return true;
    size = ((inifile->sectionCnt%SECTIONBUCKETSIZE)+1)*SECTIONBUCKETSIZE;
    newinifile->sections = malloc(size);
    memset(newinifile->sections, 0, size);

    for (isection = 0 ; isection < inifile->sectionCnt ; isection++)
    {
        if (inifile->sections[isection].name != NULL)
        {
            newinifile->sections[isection].name = strdup(inifile->sections[isection].name);
        }
        
        newinifile->sections[isection].entryCnt = inifile->sections[isection].entryCnt;

        if (!inifile->sections[isection].entryCnt) continue;

        size = ((inifile->sections[isection].entryCnt%ENTRYBUCKETSIZE)+1)*ENTRYBUCKETSIZE;
        newinifile->sections[isection].entries = malloc(size);
        memset(newinifile->sections[isection].entries, 0, size);

        for (ientry = 0 ; ientry < inifile->sections[isection].entryCnt ; ientry++)
        {
            if (inifile->sections[isection].entries[ientry].name != NULL)
            {
                newinifile->sections[isection].entries[ientry].name = strdup(inifile->sections[isection].entries[ientry].name);
            }

            if (inifile->sections[isection].entries[ientry].value != NULL)
            {
                newinifile->sections[isection].entries[ientry].value = strdup(inifile->sections[isection].entries[ientry].value);
            }
        }
    }

    return true;
}

void FlushAllPrivateProfile(void)
{
    if (!INImanInitialised()) return;

    int ifile;

    for (ifile = 0 ; ifile < iniman->fileCnt ; ifile++)
    {
        saveINIfile(&iniman->files[ifile], true);
    }
}

void FlushPrivateProfile(char *filename)
{
    if (!INImanInitialised()) return;

    INIfile *inifile = searchFile(filename);

    if (inifile == NULL) return;

    saveINIfile(inifile, true);
}

void SetBackupPrivateProfile(char *filename)
{
    if (!INImanInitialised()) return;

    INIfile *inifile = searchFile(filename);

    if (inifile == NULL) return;

    inifile->backup = true;
}

INIman *InitPrivateProfile(INIman *inimanp)
{
    if (inimanp == NULL)
    {
        if (iniman == NULL)
        {
            iniman = &_iniman;
        }
    }
    else if (iniman == NULL)
    {
        iniman = inimanp;
    }

    return iniman;
}
