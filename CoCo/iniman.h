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

#include <stdbool.h>

int GetPrivateProfileString(char *, char *, char *, char *, int , char *);
int WritePrivateProfileString(char *, char *, char *, char *);
int GetPrivateProfileInt(char *, char *, int , char *);
int WritePrivateProfileInt(char *, char *, int , char *);
bool DeletePrivateProfileEntry(char *, char *, char *);
bool DeletePrivateProfileSection(char *, char *);
void FlushPrivateProfile(void);