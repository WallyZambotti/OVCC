#ifndef __JOYSTICKINPUTSDL_H__
#define __JOYSTICKINPUTSDL_H__
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

#include "SDL2/SDL.h"
#include "defines.h"

#define MAXSTICKS 10
#define STRLEN 64
#define JOYSTICK_AUDIO 0
#define JOYSTICK_JOYSTICK 1
#define JOYSTICK_MOUSE 2
#define JOYSTICK_KEYBOARD 3

void JoyStickPollSDL(SDL_Joystick ** ,unsigned char);
int EnumerateJoysticksSDL(void);
bool InitJoyStickSDL(unsigned char);

#endif
