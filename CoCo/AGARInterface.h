#ifndef __AGARINTERFACE_H__
#define __AGARINTERFACE_H__
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

void UpdateAGAR(SystemState2 *);
bool CreateAGARWindow(SystemState2 *);
void ClsAGAR(unsigned int,SystemState2 *);
void DoClsAGAR(SystemState2 *);
unsigned char SetInfoBandAGAR( unsigned char);
unsigned char SetResizeAGAR(unsigned char);
unsigned char SetAspectAGAR(unsigned char);
float StaticAGAR(SystemState2 *);

#define MAX_LOADSTRING 100

#endif
