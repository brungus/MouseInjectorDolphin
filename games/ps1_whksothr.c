//===========================================================
// Mouse Injector for Dolphin
//==========================================================================
// Copyright (C) 2019-2020 Carnivorous
// All rights reserved.
//
// Mouse Injector is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, visit http://www.gnu.org/licenses/gpl-2.0.html
//==========================================================================
#include <stdint.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define SOTHR_CURSORX 0xD5D8C
#define SOTHR_CURSORY 0xD5D90

static uint8_t PS1_SOTHR_Status(void);
static void PS1_SOTHR_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Warhammer: Shadow of the Horned Rat",
	PS1_SOTHR_Status,
	PS1_SOTHR_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_WARHAMMERSOTHR = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_SOTHR_Status(void)
{
	return (PS1_MEM_ReadWord(0x92EC) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x92F0) == 0x5F303031U && 
			PS1_MEM_ReadWord(0x92F4) == 0x2E31373BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_SOTHR_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	uint16_t cursorX = PS1_MEM_ReadHalfword(SOTHR_CURSORX);
	uint16_t cursorY = PS1_MEM_ReadHalfword(SOTHR_CURSORY);
	float cursorXF = (float)cursorX;
	float cursorYF = (float)cursorY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 4.f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&cursorXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * scale;
	AccumulateAddRemainder(&cursorYF, &yAccumulator, -ym, dy);

	cursorXF = ClampFloat(cursorXF, 0.f, 65535.f);
	cursorYF = ClampFloat(cursorYF, 0.f, 65535.f);

	PS1_MEM_WriteHalfword(SOTHR_CURSORX, (uint16_t)cursorXF);
	PS1_MEM_WriteHalfword(SOTHR_CURSORY, (uint16_t)cursorYF);
}