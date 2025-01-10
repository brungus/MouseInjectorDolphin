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

#define TAU 6.2831853f // 0x40C90FDB

#define FUGITIVE_HUNTER_CAMX 0x371C74
#define FUGITIVE_HUNTER_CAMY 0x371C70
#define FUGITIVE_HUNTER_IS_BUSY 0x349A44

static uint8_t PS2_FUGITIVE_HUNTER_Status(void);
static void PS2_FUGITIVE_HUNTER_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Fugitive Hunter - War on Terror",
	PS2_FUGITIVE_HUNTER_Status,
	PS2_FUGITIVE_HUNTER_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_FUGITIVE_HUNTER = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_FUGITIVE_HUNTER_Status(void)
{
  // 53 4C 55 53 5F 32 30 34 2E 36 34 3B
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323034U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E36343BU);
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_FUGITIVE_HUNTER_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	if(PS2_MEM_ReadUInt(FUGITIVE_HUNTER_IS_BUSY))
    return;

  float looksensitivity = (float)sensitivity; /// 40.f;
	float scale = 300.f;

	float camX = PS2_MEM_ReadFloat(FUGITIVE_HUNTER_CAMX);
	float camY = PS2_MEM_ReadFloat(FUGITIVE_HUNTER_CAMY);

  camX += (float)xmouse * looksensitivity / scale;
  
  camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;

  while (camX > 360)
		camX -= 360.f;
	while (camX < -360)
		camX += 360.f;

  while (camY > 85)
    camY = 85.f;
  while (camY < -83)
    camY = -83.f;

  PS2_MEM_WriteFloat(FUGITIVE_HUNTER_CAMX, camX);
  PS2_MEM_WriteFloat(FUGITIVE_HUNTER_CAMY, camY);
}