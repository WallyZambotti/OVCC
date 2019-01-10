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
#include "joystickinputSDL.h"
#include "defines.h"
#include "stdbool.h"

static SDL_Joystick *Joysticks[MAXSTICKS];
char *StickName[MAXSTICKS];

int EnumerateJoysticksSDL(void)
{
	if (!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		if (SDL_Init(SDL_INIT_JOYSTICK) != 0)
		{
			fprintf(stderr, "Couldn't initialise SDL_Joystick\n");
			return 0;
		}
	}

	int JoyStickIndex=0;
	int maxJoyStick = SDL_NumJoysticks();

	for (JoyStickIndex = 0 ; JoyStickIndex < maxJoyStick ; JoyStickIndex++ )
	{
		//strcpy(StickName[JoyStickIndex], SDL_JoystickNameForIndex(JoyStickIndex));
		StickName[JoyStickIndex] = SDL_JoystickNameForIndex(JoyStickIndex);
		Joysticks[JoyStickIndex] = SDL_JoystickOpen(JoyStickIndex);
	}

	return maxJoyStick;
}

bool InitJoyStickSDL(unsigned char StickNumber)
{
	if (!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		if (SDL_Init(SDL_INIT_JOYSTICK) != 0)
		{
			fprintf(stderr, "Couldn't initialise SDL_Joystick\n");
			return 0;
		}
	}

	return(1); //return true on success
}

void JoyStickPollSDL(SDL_Joystick **js, unsigned char StickNumber)
{
	if (Joysticks[StickNumber] == NULL)
		return;

	*js = Joysticks[StickNumber];

return;
}
