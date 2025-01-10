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

#define PI 3.14159265f // 0x40490FDB
#define TAU 6.2831853f // 0x40C90FDB

//CAM Y FROM -1.04719758 TO 0.6108651161
#define STA_CAMY 0xB7F820
#define STA_CAMX 0xB7F824
#define STA_IS_BUSY 0x201EB8

//no idea what all this is for 
/*#define STA_CAMY2 0xB7F830
#define STA_CAMX2 0xB7F834

#define STA_CAMX_SIN 0xB7F860
#define STA_CAMX_SIN2 0xB7F888
#define STA_CAMX_SIN3 0xB7F9E0
#define STA_CAMX_SIN4 0xB7FA08
#define STA_CAMX_COS 0xB7F868
#define STA_CAMX_COS2 0xB7F880
#define STA_CAMX_COS3 0xB7F9E8
#define STA_CAMX_COS4 0xB7FA00*/

static uint8_t PS2_STA_Status(void);
static void PS2_STA_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Shadow Tower: Abyss",
	PS2_STA_Status,
	PS2_STA_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_STABYSS = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_STA_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5053U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323532U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E31373BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_STA_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

  if(PS2_MEM_ReadUInt(STA_IS_BUSY))
    return;
	float looksensitivity = (float)sensitivity;
	float scale = 12000.f;

	float camX = PS2_MEM_ReadFloat(STA_CAMX);
	float camY = PS2_MEM_ReadFloat(STA_CAMY);
	/*float camX2 = PS2_MEM_ReadFloat(STA_CAMX2);
	float camY2 = PS2_MEM_ReadFloat(STA_CAMY2);*/

	// camX += (float)xmouse * looksensitivity / scale;
	float dx = (float)xmouse * looksensitivity / scale;
	camX += dx;
	//camX2 += dx;
	// camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
	float dy = (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
	camY -= dy;
	//camY2 -= dy;

	while (camX > PI)
		camX -= TAU;
	while (camX < -PI)
		camX += TAU;

  while (camY > 0.6108651161f)
    camY = 0.6108651161f;
  while (camY < -1.04719758f)
    camY = -1.04719758f;
  

	/*float nScale = 1.5f;
	float camXSin = PS2_MEM_ReadFloat(STA_CAMX_SIN) / nScale;
	float camXCos = PS2_MEM_ReadFloat(STA_CAMX_COS) / nScale;
	float angle = atan(camXSin / camXCos);
	if (camXCos < 0)
		angle += TAU / 2.f;
	
	camXSin = sin(angle) * nScale;
	camXCos = cos(angle) * nScale;*/
	
  //I have no idea what the last guy was doing here, you dont need to be doing all this
	/*PS2_MEM_WriteFloat(STA_CAMX_COS, camXCos);
	PS2_MEM_WriteFloat(STA_CAMX_COS2, -camXCos);
	PS2_MEM_WriteFloat(STA_CAMX_COS3, -camXCos);
	PS2_MEM_WriteFloat(STA_CAMX_COS4, camXCos);
	PS2_MEM_WriteFloat(STA_CAMX_SIN, camXSin);
	PS2_MEM_WriteFloat(STA_CAMX_SIN2, camXSin);
	PS2_MEM_WriteFloat(STA_CAMX_SIN3, camXSin);
	PS2_MEM_WriteFloat(STA_CAMX_SIN4, camXSin);
	PS2_MEM_WriteFloat(STA_CAMX2, (float)camX);*/
  //perhaps they were trying to rectify the visual glitches that appear as a result of turning too quickly?
  //i dont know, i saw the different representations of the angles in the game memory but i dont see a good reason to edit them



  //you really only need these two
	PS2_MEM_WriteFloat(STA_CAMX, camX);
  PS2_MEM_WriteFloat(STA_CAMY, camY);
}