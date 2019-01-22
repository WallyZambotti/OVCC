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
#include "iniman.h"

#define MAX_LINE_LEN 512

typedef struct 
{
    char        *name;
    char        *value;
} INIentry;

typedef struct 
{
    char        *name;
    INIentry    *entries;
    short int   entryCnt;
} INIsection;

typedef struct 
{
    char        *name;
    FILE        *file;
    INIsection  *sections;
    bool        dirty;
    short int   sectionCnt;
} INIfile;

static INIfile inifile = 
{
    NULL, NULL, NULL, false, 0
};

static int readline(FILE *fp, char *bp)
{   char c = '\0';
    int i = 0;

    while( (c = getc(fp)) != '\n')
    {   
        if(c == EOF) 
        {
            return(0);
        }

        bp[i++] = c;
    }

    bp[i] = '\0';
    return(1);
}

#define SECTIONBUCKETSIZE 128
#define ENTRYBUCKETSIZE 32

static INIsection *recordSection(char *iniline)
{
    if (inifile.sectionCnt % SECTIONBUCKETSIZE == 0) 
    {
        inifile.sections = realloc(inifile.sections, SECTIONBUCKETSIZE * sizeof(INIsection));
    }
        
    inifile.sections[inifile.sectionCnt].name = strdup(iniline);
    inifile.sections[inifile.sectionCnt].entries = NULL;

    return &inifile.sections[inifile.sectionCnt++];
}

static void *recordEntry(INIsection *section, char *name, char *value)
{
    if (section->entryCnt % ENTRYBUCKETSIZE == 0)
    {
        section->entries = realloc(section->entries, ENTRYBUCKETSIZE * sizeof(INIentry));
    }

    section->entries[section->entryCnt].name = strdup(name);
    section->entries[section->entryCnt].value = strdup(value);

    return &section->entries[section->entryCnt++];
}

static bool saveINIfile(bool freemem)
{
    int isection, ientry;

    if ((inifile.file = fopen(inifile.name, "w")) == NULL)
    {
        return false;
    }

    for(isection = 0 ; isection < inifile.sectionCnt ; isection++)
    {
        if (inifile.sections[isection].name != NULL)
        {
            fprintf(inifile.file, "%s\n", inifile.sections[isection].name); 
        }

        for(ientry = 0 ; ientry < inifile.sections[isection].entryCnt ; ientry++)
        {
            if (inifile.sections[isection].entries[ientry].name != NULL)
            {
                fprintf(inifile.file, "%s=%s\n", 
                    inifile.sections[isection].entries[ientry].name,
                    inifile.sections[isection].entries[ientry].value);

                if (freemem)
                {
                    free(inifile.sections[isection].entries[ientry].name);
                }
            }

            if (freemem)
            {
                free(inifile.sections[isection].entries[ientry].value);                
            }
        }

        if (freemem && inifile.sections[isection].name != NULL)
        {
            free(inifile.sections[isection].name);
        }
    }

    fclose(inifile.file);
    free(inifile.name);
    inifile.name = NULL;
    return true;
}

static bool loadINIfile(char *name)
{
    char iniline[MAX_LINE_LEN];
    char dummy[1] = "";
    regex_t sectexp;
    regex_t entryexp;
    regmatch_t entparts[4];
    INIsection *section;

    if (inifile.name != NULL) 
    {
        if (strcmp(inifile.name, name) == 0)
        {
            return true;
        }

        if (saveINIfile(true) != true)
        {
            fprintf(stderr, "iniman : cannot save inifile %d : %s\n", errno, inifile.name);
            return false;
        }
    }

    inifile.file = fopen(name, "r");

    if (inifile.file == NULL)
    {
        return false;
    }

    inifile.name = strdup(name);

    regcomp(&sectexp, "^\\[[^[]*\\]", REG_EXTENDED | REG_NOSUB);
    regcomp(&entryexp, "(^[^=]*)(=)(.*)", REG_EXTENDED);

    while (readline(inifile.file, iniline)) 
    {
        if (!regexec(&sectexp, iniline, 0, NULL, 0))
        {
            section = recordSection(iniline);
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

    fclose(inifile.file);

    return true;
}

static INIsection *searchSection(char *section)
{
    int isection;

    for (isection = 0 ; isection < inifile.sectionCnt ; isection++)
    {
        if (! strcmp (inifile.sections[isection].name, section))
        {
            return (&inifile.sections[isection]);
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
    if (loadINIfile(filename) != true)
    {
        fprintf(stderr, "iniman : cannot load inifile %d : %s\n", errno, filename);
        strncpy(buffer, defaultval, bufferlen);
        return(strlen(buffer));
    }

    INIsection *sectionp = searchSection(section);

    if (sectionp == NULL)
    {
        strncpy(buffer, defaultval, bufferlen);
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
    if (loadINIfile(filename) != true)
    {
        fprintf(stderr, "iniman : cannot load inifile %d : %s\n", errno, filename);
        return(0);
    }

    INIsection *sectionp = searchSection(section);

    if (sectionp == NULL)
    {
        sectionp = recordSection(section);
    }

    if (sectionp == NULL) // New Section and New Entry
    {
        sectionp = recordSection(section);
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
    if (loadINIfile(filename) != true)
    {
        fprintf(stderr, "iniman : cannot load inifile %d : %s\n", errno, filename);
        return(0);
    }

    INIsection *sectionp = searchSection(section);

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
    if (loadINIfile(filename) != true)
    {
        fprintf(stderr, "iniman : cannot load inifile %d : %s\n", errno, filename);
        return(0);
    }

    INIsection *sectionp = searchSection(section);

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

void FlushPrivateProfile(void)
{
    saveINIfile(true);
}
