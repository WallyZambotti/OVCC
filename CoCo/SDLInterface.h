#ifndef __SDLINTERFACE_H__
#define __SDLINTERFACE_H__
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

void UnlockScreenSDL(SystemState2 *);
unsigned char LockScreenSDL(SystemState2 *);
void SetStatusBarTextSDL( char *,SystemState2 *);
bool CreateSDLWindow(SystemState2 *);
void ClsSDL(unsigned int,SystemState2 *);
void DoClsSDL(SystemState2 *);
unsigned char SetInfoBandSDL( unsigned char);
unsigned char SetResizeSDL(unsigned char);
unsigned char SetAspectSDL(unsigned char);
float StaticSDL(SystemState2 *);

#define MAX_LOADSTRING 100

#endif
