/*****************************************************************************/
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
/*****************************************************************************/
/*
	Keyboard layout data

	key translation tables used to convert keyboard oem scan codes / key 
	combinations into CoCo keyboard row/col values

	ScanCode1 and ScanCode2 are used to determine what actual
	key presses are translated into a specific CoCo key

	Keyboard ScanCodes are from dinput.h

	The code expects SHIFTed entries in the table to use AG_KEY_LSHIFT (not AG_KEY_RSHIFT)
	The key handling code turns AG_KEY_RSHIFT into AG_KEY_LSHIFT

	These do not need to be in any particular order, 
	the code sorts them after they are copied to the run-time table
	each table is terminated at the first entry with ScanCode1+2 == 0

	PC Keyboard:
	+---------------------------------------------------------------------------------+
	| [Esc]   [F1][F2][F3][F4][F5][F6][F7][F8][F9][F10][F11][F12]   [Prnt][Scr][Paus] |
	|                                                                                 |
	| [`~][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [Inst][Hom][PgUp] |
	| [  Tab][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [Dlet][End][PgDn] |
	| [  Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                     |
	| [  Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
	| [Cntl][Win][Alt][        Space       ][Alt][Win][Prp][Cntl]   [LftA][DnA][RgtA] |
	+---------------------------------------------------------------------------------+


	TODO: explain and add link or reference to CoCo 'scan codes' for each key
*/
/*****************************************************************************/

#include "keyboardLayout.h"
#include <agar/core.h>
#include <agar/gui.h>


/*****************************************************************************/
/**
	Original VCC key translation table for DECB

	VCC BASIC Keyboard:

	+--------------------------------------------------------------------------------+
	[  ][F1][F2][  ][  ][Rst][RGB][  ][Thr][Pwr][StB][FSc][  ]   [    ][   ][    ]   |
    |                                                                                |
	| [ ][1!][2"][3#][4$][5%][6&][7'][8(][9)][0 ][:*][-=][BkSpc]   [    ][Clr][    ] |
	| [    ][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [    ][Esc][    ] |
	| [ Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:][  ][Enter]                     |
	| [ Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
	| [Cntl][   ][Alt][       Space       ][ @ ][   ][   ][Cntl]   [LftA][DnA][RgtA] |
	+--------------------------------------------------------------------------------+
	*/
keytranslationentry_t keyTranslationsCoCoSDL[] =
{
	// ScanCode1,     ScanCode2,      Row1,  Col1,  Row2, Col2    Char  
	{ AG_KEY_A,          0,              1,     1,     0,    0 }, //   A
	{ AG_KEY_B,          0,              1,     2,     0,    0 }, //   B
	{ AG_KEY_C,          0,              1,     3,     0,    0 }, //   C
	{ AG_KEY_D,          0,              1,     4,     0,    0 }, //   D
	{ AG_KEY_E,          0,              1,     5,     0,    0 }, //   E
	{ AG_KEY_F,          0,              1,     6,     0,    0 }, //   F
	{ AG_KEY_G,          0,              1,     7,     0,    0 }, //   G
	{ AG_KEY_H,          0,              2,     0,     0,    0 }, //   H
	{ AG_KEY_I,          0,              2,     1,     0,    0 }, //   I
	{ AG_KEY_J,          0,              2,     2,     0,    0 }, //   J
	{ AG_KEY_K,          0,              2,     3,     0,    0 }, //   K
	{ AG_KEY_L,          0,              2,     4,     0,    0 }, //   L
	{ AG_KEY_M,          0,              2,     5,     0,    0 }, //   M
	{ AG_KEY_N,          0,              2,     6,     0,    0 }, //   N
	{ AG_KEY_O,          0,              2,     7,     0,    0 }, //   O
	{ AG_KEY_P,          0,              4,     0,     0,    0 }, //   P
	{ AG_KEY_Q,          0,              4,     1,     0,    0 }, //   Q
	{ AG_KEY_R,          0,              4,     2,     0,    0 }, //   R
	{ AG_KEY_S,          0,              4,     3,     0,    0 }, //   S
	{ AG_KEY_T,          0,              4,     4,     0,    0 }, //   T
	{ AG_KEY_U,          0,              4,     5,     0,    0 }, //   U
	{ AG_KEY_V,          0,              4,     6,     0,    0 }, //   V
	{ AG_KEY_W,          0,              4,     7,     0,    0 }, //   W
	{ AG_KEY_X,          0,              8,     0,     0,    0 }, //   X
	{ AG_KEY_Y,          0,              8,     1,     0,    0 }, //   Y
	{ AG_KEY_Z,          0,              8,     2,     0,    0 }, //   Z
	{ AG_KEY_0,          0,             16,     0,     0,    0 }, //   0
	{ AG_KEY_1,          0,             16,     1,     0,    0 }, //   1
	{ AG_KEY_2,          0,             16,     2,     0,    0 }, //   2
	{ AG_KEY_3,          0,             16,     3,     0,    0 }, //   3
	{ AG_KEY_4,          0,             16,     4,     0,    0 }, //   4
	{ AG_KEY_5,          0,             16,     5,     0,    0 }, //   5
	{ AG_KEY_6,          0,             16,     6,     0,    0 }, //   6
	{ AG_KEY_7,          0,             16,     7,     0,    0 }, //   7
	{ AG_KEY_8,          0,             32,     0,     0,    0 }, //   8
	{ AG_KEY_9,          0,             32,     1,     0,    0 }, //   9
	{ AG_KEY_1,          AG_KEY_LSHIFT,    16,     1,    64,    7 }, //   !
	{ AG_KEY_2,          AG_KEY_LSHIFT,    16,     2,    64,    7 }, //   "
	{ AG_KEY_3,          AG_KEY_LSHIFT,    16,     3,    64,    7 }, //   #
	{ AG_KEY_4,          AG_KEY_LSHIFT,    16,     4,    64,    7 }, //   $
	{ AG_KEY_5,          AG_KEY_LSHIFT,    16,     5,	   64,    7 }, //   %
	{ AG_KEY_6,          AG_KEY_LSHIFT,    16,     6,    64,    7 }, //   &
	{ AG_KEY_7,          AG_KEY_LSHIFT,    16,     7,    64,    7 }, //   '
	{ AG_KEY_8,          AG_KEY_LSHIFT,    32,     0,    64,    7 }, //   (
	{ AG_KEY_9,          AG_KEY_LSHIFT,    32,     1,    64,    7 }, //   )
	{ AG_KEY_0,          AG_KEY_LSHIFT,    16,     0,    64,    7 }, //   CAPS LOCK (DECB SHIFT-0, OS-9 CTRL-0 does not need ot be emulated specifically)
	{ AG_KEY_SPACE,      0,              8,     7,     0,    0 }, //   SPACE

	{ AG_KEY_COMMA,      0,             32,     4,     0,    0 }, //   ,
	{ AG_KEY_PERIOD,     0,             32,     6,     0,    0 }, //   .
	{ AG_KEY_SLASH,      AG_KEY_LSHIFT,    32,     7,    64,    7 }, //   ?
	{ AG_KEY_SLASH,      0,             32,     7,     0,    0 }, //   /
	{ AG_KEY_MINUS,      AG_KEY_LSHIFT,    32,     2,    64,    7 }, //   *
	{ AG_KEY_MINUS,      0,             32,     2,     0,    0 }, //   :
	{ AG_KEY_SEMICOLON,  AG_KEY_LSHIFT,    32,     3,    64,    7 }, //   +
	{ AG_KEY_SEMICOLON,  0,             32,     3,     0,    0 }, //   ;
	{ AG_KEY_EQUALS,     AG_KEY_LSHIFT,    32,     5,    64,    7 }, //   =
	{ AG_KEY_EQUALS,     0,             32,     5,     0,    0 }, //   -
//	{ AG_KEY_GRAVE,      AG_KEY_LSHIFT,    16,     3,    64,    4 }, //   ~ (tilde) (CoCo CTRL-3)

	// added
	{ AG_KEY_UP,    0,              8,     3,     0,    0 }, //   UP
	{ AG_KEY_DOWN,  0,              8,     4,     0,    0 }, //   DOWN
	{ AG_KEY_LEFT,  0,              8,     5,     0,    0 }, //   LEFT
	{ AG_KEY_RIGHT, 0,              8,     6,     0,    0 }, //   RIGHT

	{ AG_KEY_KP8,    0,              8,     3,     0,    0 }, //   UP
	{ AG_KEY_KP2,    0,              8,     4,     0,    0 }, //   DOWN
	{ AG_KEY_KP4,    0,              8,     5,     0,    0 }, //   LEFT
	{ AG_KEY_KP6,    0,              8,     6,     0,    0 }, //   RIGHT

	{ AG_KEY_RETURN,     0,             64,     0,     0,    0 }, //   ENTER
	{ AG_KEY_HOME,    0,             64,     1,     0,    0 }, //   HOME (CLEAR)
	{ AG_KEY_ESCAPE,    0,             64,     2,     0,    0 }, //   ESCAPE (BREAK)
	{ AG_KEY_F1,         0,             64,     5,     0,    0 }, //   F1
	{ AG_KEY_F2,         0,             64,     6,     0,    0 }, //   F2
	{ AG_KEY_BACKSPACE,       0,              8,     5,     0,    0 }, //   BACKSPACE -> CoCo left arrow

//	{ AG_KEY_CAPSLOCK,   0,             64,     4,    16,    0 }, //   CAPS LOCK (CoCo CTRL-0 for OS-9)
	{ AG_KEY_CAPSLOCK,   0,             64,     7,    16,    0 }, //   CAPS LOCK (CoCo SHIFT-0 for DECB)

	// these produce the square bracket characters in DECB 
	// but it does not match what the real CoCo does
	//{ AG_KEY_LBRACKET,   0,           64,     7,    8,    4 }, //   [
	//{ AG_KEY_RBRACKET,   0,           64,     7,    8,    6 }, //   ]
	// added from OS-9 layout
	{ AG_KEY_LEFTBRACKET,   0,             64,     4,    32,    0 }, //   [ (CoCo CTRL 8)
	{ AG_KEY_RIGHTBRACKET,   0,             64,     4,    32,    1 }, //   ] (CoCo CTRL 9)
	{ AG_KEY_LEFTBRACKET,   AG_KEY_LSHIFT,    64,     4,    32,    4 }, //   { (CoCo CTRL ,)
	{ AG_KEY_RIGHTBRACKET,   AG_KEY_LSHIFT,    64,     4,    32,    6 }, //   } (CoCo CTRL .)

//	{ AG_KEY_SCROLL,     0,              1,     0,    64,    7 }, //   SCROLL (CoCo SHIFT @)
	{ AG_KEY_RALT,       0,              1,     0,     0,    0 }, //   @

	{ AG_KEY_LALT,       0,             64,     3,     0,    0 }, //   ALT
	{ AG_KEY_LCTRL,   0,             64,     4,     0,    0 }, //   CTRL
	{ AG_KEY_LSHIFT,     0,             64,     7,     0,    0 }, //   SHIFT (the code converts AG_KEY_RSHIFT to AG_KEY_LSHIFT)

	{ 0,              0,              0,     0,     0,    0 }  // terminator
};

/*****************************************************************************/
/**
	Original VCC key translation table for OS-9

	PC Keyboard:
	+---------------------------------------------------------------------------------+
	| [Esc]   [F1][F2][F3][F4][F5][F6][F7][F8][F9][F10][F11][F12]   [Prnt][Scr][Paus] |
	|                                                                                 |
	| [`~][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [Inst][Hom][PgUp] |
	| [  Tab][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [Dlet][End][PgDn] |
	| [  Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                     |
	| [  Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
	| [Cntl][Win][Alt][        Space       ][Alt][Win][Prp][Cntl]   [LftA][DnA][RgtA] |
	+---------------------------------------------------------------------------------+

	VCC OS-9 Keyboard

	+---------------------------------------------------------------------------------+
	| [  ][F1][F2][  ][  ][Rst][RGB][  ][Thr][Pwr][StB][FSc][  ]   [    ][   ][    ]  |
	|                                                                                 |
	| [`][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [    ][Clr][    ]  |
	| [    ][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [    ][Esc][    ]  |
	| [ Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                      |
	| [ Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]        |
	| [Cntl][   ][Alt][       Space       ][Alt][   ][   ][Cntl]   [LftA][DnA][RgtA]  |
	+---------------------------------------------------------------------------------+
	*/
keytranslationentry_t keyTranslationsNaturalSDL[] =
{
	// ScanCode1,     ScanCode2,      Row1,  Col1,  Row2, Col2    Char  
	{ AG_KEY_A,          0,              1,     1,     0,    0 }, //   A
	{ AG_KEY_B,          0,              1,     2,     0,    0 }, //   B
	{ AG_KEY_C,          0,              1,     3,     0,    0 }, //   C
	{ AG_KEY_D,          0,              1,     4,     0,    0 }, //   D
	{ AG_KEY_E,          0,              1,     5,     0,    0 }, //   E
	{ AG_KEY_F,          0,              1,     6,     0,    0 }, //   F
	{ AG_KEY_G,          0,              1,     7,     0,    0 }, //   G
	{ AG_KEY_H,          0,              2,     0,     0,    0 }, //   H
	{ AG_KEY_I,          0,              2,     1,     0,    0 }, //   I
	{ AG_KEY_J,          0,              2,     2,     0,    0 }, //   J
	{ AG_KEY_K,          0,              2,     3,     0,    0 }, //   K
	{ AG_KEY_L,          0,              2,     4,     0,    0 }, //   L
	{ AG_KEY_M,          0,              2,     5,     0,    0 }, //   M
	{ AG_KEY_N,          0,              2,     6,     0,    0 }, //   N
	{ AG_KEY_O,          0,              2,     7,     0,    0 }, //   O
	{ AG_KEY_P,          0,              4,     0,     0,    0 }, //   P
	{ AG_KEY_Q,          0,              4,     1,     0,    0 }, //   Q
	{ AG_KEY_R,          0,              4,     2,     0,    0 }, //   R
	{ AG_KEY_S,          0,              4,     3,     0,    0 }, //   S
	{ AG_KEY_T,          0,              4,     4,     0,    0 }, //   T
	{ AG_KEY_U,          0,              4,     5,     0,    0 }, //   U
	{ AG_KEY_V,          0,              4,     6,     0,    0 }, //   V
	{ AG_KEY_W,          0,              4,     7,     0,    0 }, //   W
	{ AG_KEY_X,          0,              8,     0,     0,    0 }, //   X
	{ AG_KEY_Y,          0,              8,     1,     0,    0 }, //   Y
	{ AG_KEY_Z,          0,              8,     2,     0,    0 }, //   Z
	{ AG_KEY_0,          0,             16,     0,     0,    0 }, //   0
	{ AG_KEY_1,          0,             16,     1,     0,    0 }, //   1
	{ AG_KEY_2,          0,             16,     2,     0,    0 }, //   2
	{ AG_KEY_3,          0,             16,     3,     0,    0 }, //   3
	{ AG_KEY_4,          0,             16,     4,     0,    0 }, //   4
	{ AG_KEY_5,          0,             16,     5,     0,    0 }, //   5
	{ AG_KEY_6,          0,             16,     6,     0,    0 }, //   6
	{ AG_KEY_7,          0,             16,     7,     0,    0 }, //   7
	{ AG_KEY_8,          0,             32,     0,     0,    0 }, //   8
	{ AG_KEY_9,          0,             32,     1,     0,    0 }, //   9
	{ AG_KEY_1,          AG_KEY_LSHIFT,    16,     1,    64,    7 }, //   !
	{ AG_KEY_2,          AG_KEY_LSHIFT,     1,     0,     0,    0 }, //   @
	{ AG_KEY_3,          AG_KEY_LSHIFT,    16,     3,    64,    7 }, //   #
	{ AG_KEY_4,          AG_KEY_LSHIFT,    16,     4,    64,    7 }, //   $
	{ AG_KEY_5,          AG_KEY_LSHIFT,    16,     5,	   64,    7 }, //   %
	{ AG_KEY_6,          AG_KEY_LSHIFT,    16,     7,    64,    4 }, //   ^ (CoCo CTRL 7)
	{ AG_KEY_7,          AG_KEY_LSHIFT,    16,     6,    64,    7 }, //   &
	{ AG_KEY_8,          AG_KEY_LSHIFT,    32,     2,    64,    7 }, //   *
	{ AG_KEY_9,          AG_KEY_LSHIFT,    32,     0,    64,    7 }, //   (
	{ AG_KEY_0,          AG_KEY_LSHIFT,    32,     1,    64,    7 }, //   )

	{ AG_KEY_SEMICOLON,  0,             32,     3,     0,    0 }, //   ;
	{ AG_KEY_SEMICOLON,  AG_KEY_LSHIFT,    32,     2,     0,    0 }, //   :

	{ AG_KEY_QUOTE, 0,             16,     7,    64,    7 }, //   '
	{ AG_KEY_QUOTE, AG_KEY_LSHIFT,    16,     2,    64,    7 }, //   "

	{ AG_KEY_COMMA,      0,             32,     4,     0,    0 }, //   ,
	{ AG_KEY_PERIOD,     0,             32,     6,     0,    0 }, //   .
	{ AG_KEY_SLASH,      AG_KEY_LSHIFT,    32,     7,    64,    7 }, //   ?
	{ AG_KEY_SLASH,      0,             32,     7,     0,    0 }, //   /
	{ AG_KEY_EQUALS,     AG_KEY_LSHIFT,    32,     3,    64,    7 }, //   +
	{ AG_KEY_EQUALS,     0,             32,     5,    64,    7 }, //   =
	{ AG_KEY_MINUS,      0,             32,     5,     0,    0 }, //   -
	{ AG_KEY_MINUS,      AG_KEY_LSHIFT,    32,     5,    64,    4 }, //   _ (underscore) (CoCo CTRL -)

	// added
	{ AG_KEY_UP,    0,              8,     3,     0,    0 }, //   UP
	{ AG_KEY_DOWN,  0,              8,     4,     0,    0 }, //   DOWN
	{ AG_KEY_LEFT,  0,              8,     5,     0,    0 }, //   LEFT
	{ AG_KEY_RIGHT, 0,              8,     6,     0,    0 }, //   RIGHT

	{ AG_KEY_KP8,    0,              8,     3,     0,    0 }, //   UP
	{ AG_KEY_KP2,    0,              8,     4,     0,    0 }, //   DOWN
	{ AG_KEY_KP4,    0,              8,     5,     0,    0 }, //   LEFT
	{ AG_KEY_KP6,    0,              8,     6,     0,    0 }, //   RIGHT
	{ AG_KEY_SPACE,      0,              8,     7,     0,    0 }, //   SPACE

	{ AG_KEY_RETURN,     0,             64,     0,     0,    0 }, //   ENTER
	{ AG_KEY_HOME,    0,             64,     1,     0,    0 }, //   HOME (CLEAR)
	{ AG_KEY_ESCAPE,    0,             64,     2,     0,    0 }, //   ESCAPE (BREAK)
	{ AG_KEY_F1,         0,             64,     5,     0,    0 }, //   F1
	{ AG_KEY_F2,         0,             64,     6,     0,    0 }, //   F2
	{ AG_KEY_BACKSPACE,       0,              8,     5,     0,    0 }, //   BACKSPACE -> CoCo left arrow

	{ AG_KEY_LEFTBRACKET,   0,             64,     4,    32,    0 }, //   [ (CoCo CTRL 8)
	{ AG_KEY_RIGHTBRACKET,   0,             64,     4,    32,    1 }, //   ] (CoCo CTRL 9)
	{ AG_KEY_LEFTBRACKET,   AG_KEY_LSHIFT,    64,     4,    32,    4 }, //   { (CoCo CTRL ,)
	{ AG_KEY_RIGHTBRACKET,   AG_KEY_LSHIFT,    64,     4,    32,    6 }, //   } (CoCo CTRL .)

	{ AG_KEY_BACKSLASH,  0,             32,     7,    64,    4 }, //   '\' (Back slash) (CoCo CTRL /)
	{ AG_KEY_BACKSLASH,  AG_KEY_LSHIFT,    16,     1,    64,    4 }, //   | (Pipe) (CoCo CTRL 1)
	{ AG_KEY_GRAVE,      AG_KEY_LSHIFT,    16,     3,    64,    4 }, //   ~ (tilde) (CoCo CTRL 3)

	{ AG_KEY_CAPSLOCK,   0,             64,     4,    16,    0 }, //   CAPS LOCK (CoCo CTRL 0 for OS-9)
//	{ AG_KEY_CAPSLOCK,   0,             64,     7,    16,    0 }, //   CAPS LOCK (CoCo Shift 0 for DECB)

//	{ AG_KEY_SCROLL,     0,              1,     0,    64,    7 }, //   SCROLL (CoCo SHIFT @)

	{ AG_KEY_LALT,       0,             64,     3,     0,    0 }, //   ALT
	{ AG_KEY_LCTRL,   0,             64,     4,     0,    0 }, //   CTRL
	{ AG_KEY_LSHIFT,     0,             64,     7,     0,    0 }, //   SHIFT (the code converts AG_KEY_RSHIFT to AG_KEY_LSHIFT)

	{ 0,              0,              0,     0,     0,    0 }  // terminator
};

/*****************************************************************************/
/**
Compact natural key translation table (no number pad)

PC Keyboard:
+---------------------------------------------------------------------------------+
| [Esc]   [F1][F2][F3][F4][F5][F6][F7][F8][F9][F10][F11][F12]   [Prnt][Scr][Paus] |
|                                                                                 |
| [`~][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [Inst][Hom][PgUp] |
| [     ][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [Dlet][End][PgDn] |
| [  Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                     |
| [  Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
| [Cntl][Win][Alt][        Space       ][Alt][Win][Prp][Cntl]   [LftA][DnA][RgtA] |
+---------------------------------------------------------------------------------+

Compact natural Keyboard

+-----------------------------------------------------------------------------------+
| [  ][F1][F2][  ][  ][Rst][RGB][  ][Thr][Pwr][StB][FSc][  ]   [    ][   ][    ]    |
|                                                                                   |
| [BRK~][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [    ][   ][    ] |
|    [ CLR][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [    ][   ][    ] |
|    [ Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                     |
|    [ Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
|    [Cntl][   ][Alt][       Space       ][   ][   ][   ][Cntl]   [LftA][DnA][RgtA] |
+-----------------------------------------------------------------------------------+

Differences from Natural layout:

	CLR				TAB
	ESC/BREAK		GRAVE (`)
*/
keytranslationentry_t keyTranslationsCompactSDL[] =
{
	// ScanCode1,     ScanCode2,      Row1,  Col1,  Row2, Col2    Char  
	{ AG_KEY_A,          0,              1,     1,     0,    0 }, //   A
	{ AG_KEY_B,          0,              1,     2,     0,    0 }, //   B
	{ AG_KEY_C,          0,              1,     3,     0,    0 }, //   C
	{ AG_KEY_D,          0,              1,     4,     0,    0 }, //   D
	{ AG_KEY_E,          0,              1,     5,     0,    0 }, //   E
	{ AG_KEY_F,          0,              1,     6,     0,    0 }, //   F
	{ AG_KEY_G,          0,              1,     7,     0,    0 }, //   G
	{ AG_KEY_H,          0,              2,     0,     0,    0 }, //   H
	{ AG_KEY_I,          0,              2,     1,     0,    0 }, //   I
	{ AG_KEY_J,          0,              2,     2,     0,    0 }, //   J
	{ AG_KEY_K,          0,              2,     3,     0,    0 }, //   K
	{ AG_KEY_L,          0,              2,     4,     0,    0 }, //   L
	{ AG_KEY_M,          0,              2,     5,     0,    0 }, //   M
	{ AG_KEY_N,          0,              2,     6,     0,    0 }, //   N
	{ AG_KEY_O,          0,              2,     7,     0,    0 }, //   O
	{ AG_KEY_P,          0,              4,     0,     0,    0 }, //   P
	{ AG_KEY_Q,          0,              4,     1,     0,    0 }, //   Q
	{ AG_KEY_R,          0,              4,     2,     0,    0 }, //   R
	{ AG_KEY_S,          0,              4,     3,     0,    0 }, //   S
	{ AG_KEY_T,          0,              4,     4,     0,    0 }, //   T
	{ AG_KEY_U,          0,              4,     5,     0,    0 }, //   U
	{ AG_KEY_V,          0,              4,     6,     0,    0 }, //   V
	{ AG_KEY_W,          0,              4,     7,     0,    0 }, //   W
	{ AG_KEY_X,          0,              8,     0,     0,    0 }, //   X
	{ AG_KEY_Y,          0,              8,     1,     0,    0 }, //   Y
	{ AG_KEY_Z,          0,              8,     2,     0,    0 }, //   Z
	{ AG_KEY_0,          0,             16,     0,     0,    0 }, //   0
	{ AG_KEY_1,          0,             16,     1,     0,    0 }, //   1
	{ AG_KEY_2,          0,             16,     2,     0,    0 }, //   2
	{ AG_KEY_3,          0,             16,     3,     0,    0 }, //   3
	{ AG_KEY_4,          0,             16,     4,     0,    0 }, //   4
	{ AG_KEY_5,          0,             16,     5,     0,    0 }, //   5
	{ AG_KEY_6,          0,             16,     6,     0,    0 }, //   6
	{ AG_KEY_7,          0,             16,     7,     0,    0 }, //   7
	{ AG_KEY_8,          0,             32,     0,     0,    0 }, //   8
	{ AG_KEY_9,          0,             32,     1,     0,    0 }, //   9
	{ AG_KEY_1,          AG_KEY_LSHIFT,    16,     1,    64,    7 }, //   !
	{ AG_KEY_2,          AG_KEY_LSHIFT,     1,     0,     0,    0 }, //   @
	{ AG_KEY_3,          AG_KEY_LSHIFT,    16,     3,    64,    7 }, //   #
	{ AG_KEY_4,          AG_KEY_LSHIFT,    16,     4,    64,    7 }, //   $
	{ AG_KEY_5,          AG_KEY_LSHIFT,    16,     5,	   64,    7 }, //   %
	{ AG_KEY_6,          AG_KEY_LSHIFT,    16,     7,    64,    4 }, //   ^ (CoCo CTRL 7)
	{ AG_KEY_7,          AG_KEY_LSHIFT,    16,     6,    64,    7 }, //   &
	{ AG_KEY_8,          AG_KEY_LSHIFT,    32,     2,    64,    7 }, //   *
	{ AG_KEY_9,          AG_KEY_LSHIFT,    32,     0,    64,    7 }, //   (
	{ AG_KEY_0,          AG_KEY_LSHIFT,    32,     1,    64,    7 }, //   )

	{ AG_KEY_SEMICOLON,  0,             32,     3,     0,    0 }, //   ;
	{ AG_KEY_SEMICOLON,  AG_KEY_LSHIFT,    32,     2,     0,    0 }, //   :

	{ AG_KEY_QUOTE, 0,             16,     7,    64,    7 }, //   '
	{ AG_KEY_QUOTE, AG_KEY_LSHIFT,    16,     2,    64,    7 }, //   "

	{ AG_KEY_COMMA,      0,             32,     4,     0,    0 }, //   ,
	{ AG_KEY_PERIOD,     0,             32,     6,     0,    0 }, //   .
	{ AG_KEY_SLASH,      AG_KEY_LSHIFT,    32,     7,    64,    7 }, //   ?
	{ AG_KEY_SLASH,      0,             32,     7,     0,    0 }, //   /
	{ AG_KEY_EQUALS,     AG_KEY_LSHIFT,    32,     3,    64,    7 }, //   +
	{ AG_KEY_EQUALS,     0,             32,     5,    64,    7 }, //   =
	{ AG_KEY_MINUS,      0,             32,     5,     0,    0 }, //   -
	{ AG_KEY_MINUS,      AG_KEY_LSHIFT,    32,     5,    64,    4 }, //   _ (underscore) (CoCo CTRL -)

															   // added
	{ AG_KEY_UP,    0,              8,     3,     0,    0 }, //   UP
	{ AG_KEY_DOWN,  0,              8,     4,     0,    0 }, //   DOWN
	{ AG_KEY_LEFT,  0,              8,     5,     0,    0 }, //   LEFT
	{ AG_KEY_RIGHT, 0,              8,     6,     0,    0 }, //   RIGHT

	{ AG_KEY_KP8,    0,              8,     3,     0,    0 }, //   UP
	{ AG_KEY_KP2,    0,              8,     4,     0,    0 }, //   DOWN
	{ AG_KEY_KP4,    0,              8,     5,     0,    0 }, //   LEFT
	{ AG_KEY_KP6,    0,              8,     6,     0,    0 }, //   RIGHT
	{ AG_KEY_SPACE,      0,              8,     7,     0,    0 }, //   SPACE

	{ AG_KEY_RETURN,     0,             64,     0,     0,    0 }, //   ENTER
	{ AG_KEY_HOME,        0,             64,     1,     0,    0 }, //   HOME (CLEAR)
	{ AG_KEY_ESCAPE,      0,             64,     2,     0,    0 }, //   ESCAPE (BREAK)
	{ AG_KEY_F1,         0,             64,     5,     0,    0 }, //   F1
	{ AG_KEY_F2,         0,             64,     6,     0,    0 }, //   F2
	{ AG_KEY_BACKSPACE,       0,              8,     5,     0,    0 }, //   BACKSPACE -> CoCo left arrow

	{ AG_KEY_LEFTBRACKET,   0,             64,     4,    32,    0 }, //   [ (CoCo CTRL 8)
	{ AG_KEY_RIGHTBRACKET,   0,             64,     4,    32,    1 }, //   ] (CoCo CTRL 9)
	{ AG_KEY_LEFTBRACKET,   AG_KEY_LSHIFT,    64,     4,    32,    4 }, //   { (CoCo CTRL ,)
	{ AG_KEY_RIGHTBRACKET,   AG_KEY_LSHIFT,    64,     4,    32,    6 }, //   } (CoCo CTRL .)

	{ AG_KEY_BACKSLASH,  0,             32,     7,    64,    4 }, //   '\' (Back slash) (CoCo CTRL /)
	{ AG_KEY_BACKSLASH,  AG_KEY_LSHIFT,    16,     1,    64,    4 }, //   | (Pipe) (CoCo CTRL 1)
	{ AG_KEY_GRAVE,      AG_KEY_LSHIFT,    16,     3,    64,    4 }, //   ~ (tilde) (CoCo CTRL 3)

	{ AG_KEY_CAPSLOCK,   0,             64,     4,    16,    0 }, //   CAPS LOCK (CoCo CTRL 0 for OS-9)
//	{ AG_KEY_CAPSLOCK,   0,             64,     7,    16,    0 }, //   CAPS LOCK (CoCo Shift 0 for DECB)

//	{ AG_KEY_SCROLL,     0,              1,     0,    64,    7 }, //   SCROLL (CoCo SHIFT @)

	{ AG_KEY_LALT,       0,             64,     3,     0,    0 }, //   ALT
	{ AG_KEY_LCTRL,   0,             64,     4,     0,    0 }, //   CTRL
	{ AG_KEY_LSHIFT,     0,             64,     7,     0,    0 }, //   SHIFT (the code converts AG_KEY_RSHIFT to AG_KEY_LSHIFT)

	{ 0,              0,              0,     0,     0,    0 }  // terminator
};

/*****************************************************************************/
