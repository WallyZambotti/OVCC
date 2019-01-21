#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <stdbool.h>

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
    NULL, NULL, NULL, NULL, false, 0
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

#define BUCKETSIZE 128

static INIsection *recordSection(char * iniline)
{
    if (inifile.sectionCnt % BUCKETSIZE == 0) 
    {
        inifile.sections = realloc(inifile.sections, BUCKETSIZE * sizeof(INIsection));
        inifile.sections[inifile.sectionCnt].name = iniline;
        inifile.sections[inifile.sectionCnt].entries = NULL;
        inifile.sectionCnt++;
    }
}

static void *recordEntry(INIsection *section, char *name, char *value)
{
    if (section->entryCnt % BUCKETSIZE == 0)
    {
        section->entries = realloc(section->entries, BUCKETSIZE * sizeof(INIentry));
        strdup(section->entries[section->entryCnt].name, name);
        strdup(section->entries[section->entryCnt].value, value);
        section->entryCnt++;
    }
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
        fprintf(inifile.file, "%s\n", inifile.sections[isection].name);

        for(ientry = 0 ; ientry < inifile.sections[isection].entryCnt ; ientry++)
        {
            fprintf(inifile.file, "%s=%s\n", 
                inifile.sections[isection].entries[ientry].name,
                inifile.sections[isection].entries[ientry].value);

            if (freemem)
            {
                free(inifile.sections[isection].entries[ientry].name);
                free(inifile.sections[isection].entries[ientry].value);
            }
        }

        if (freemem)
        {
            free(inifile.sections[isection].name);
        }
    }

    fclose(inifile.file);
    free(inifile.name);
    inifile.name = NULL;
}

static bool loadINIfile(char *name)
{
    char iniline[MAX_LINE_LEN];
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

    regcomp(&sectexp, "^\[[^[]*\]", REG_EXTENDED | REG_NOSUB);
    regcomp(&entryexp, "(^[^=]*)(=)(.*)", REG_EXTENDED);

    while (readline(inifile.file, iniline)) 
    {
        if (!regexec(&sectexp, iniline, 0, NULL, 0))
        {
            section = recordSection(iniline);
        }
        else if (!regexec(&entryexp, iniline, 4, entparts, 0))
        {
            char *entname = strndup(entparts[1].rm_so, entparts[1].rm_eo - entparts[1].rm_so);
            char *entval;

            if (entparts[3].rm_so != -1)
            {
                entval = strndup(entparts[3].rm_so, entparts[3].rm_eo - entparts[3].rm_so);
            }
            else
            {
                entval = strdup("");
            }

            recordEntry(section, entname, entval);
        }
    }

    fclose(inifile.file);

    return true;
}

int GetPrivateProfileString(char *section, char *entry, char *defaultval, char *buffer, int bufferlen, char *filename)
{
    if (loadINIfile(filename) != true)
    {
        fprintf(stderr, "iniman : cannot load inifile %d : %s\n", errno, filename);
        strncpy(buffer, defaultval, bufferlen);
        return(strlen(buffer));
    }
}
