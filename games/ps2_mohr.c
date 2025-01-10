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


//variables
#define TAU 6.2831853f
#define PS2_MOHR_CAMERA_BASE_POINTER 0x35E260
#define PS2_MOHR_CAMERA_BASE_OFFSET_X 0x274
#define PS2_MOHR_CAMERA_BASE_OFFSET_Y 0x278
#define PS2_MOHR_CAMERA_BASE_OFFSET_FOV 0xE30//zoom sensitivity
#define PS2_MOHR_IS_MMG_FLAG_OFFSET 0xDB8//detect machine gun
#define PS2_MOHR_CAMERA_BASE_OFFSET_MMG_X 0xE1C//offset for mg - X axis
#define PS2_MOHR_CAMERA_BASE_OFFSET_MMG_Y 0xE20//offset for mg - Y axis

static uint8_t PS2_MOHR_Status(void);
static uint8_t PS2_MOHR_DetectCamera(void);
static void PS2_MOHR_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Medal of Honor: Rising Sun",
	PS2_MOHR_Status,
	PS2_MOHR_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_MOHRISINGSUN = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static uint32_t tempcambase = 0;
static uint16_t MG_FLAG = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_MOHR_Status(void)
{
	//SLUS_203.68 - 53 4C 55 53 5F 32 30 33 2E 36 38
	//SLUS_207.53 - 53 4C 55 53 5F 32 30 37 2E 35 33
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553 && PS2_MEM_ReadWord(0x00093394) == 0x5F323037) && PS2_MEM_ReadWord(0x00093398) == 0x2E35333B;
}


//==========================================================================
// Purpose: Camera object check. Don't write where you're not supposed to.
//==========================================================================
static uint8_t PS2_MOHR_DetectCamera(void)
{
	uint32_t tempcambase = PS2_MEM_ReadPointer(PS2_MOHR_CAMERA_BASE_POINTER);
	if (tempcambase != 0)
	{
		camBase = tempcambase;
		return 1;
	}

	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_MOHR_Inject(void)
{	
	if (!PS2_MOHR_DetectCamera())
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	//sensitivity
	float looksensitivity = (float)sensitivity;
	float scale = 10000.f;

	MG_FLAG = PS2_MEM_ReadUInt16(camBase + PS2_MOHR_IS_MMG_FLAG_OFFSET);
	float fov = PS2_MEM_ReadFloat(camBase + PS2_MOHR_CAMERA_BASE_OFFSET_FOV);
	
	if (MG_FLAG == 0x0000)
	{
	float MMGX = PS2_MEM_ReadFloat(camBase + PS2_MOHR_CAMERA_BASE_OFFSET_MMG_X);
	float MMGY = PS2_MEM_ReadFloat(camBase + PS2_MOHR_CAMERA_BASE_OFFSET_MMG_Y);
	
	MMGX += (float)xmouse * looksensitivity / scale * -1 * (fov / 43.f);
	MMGY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * (fov / 43.f);

	PS2_MEM_WriteFloat(camBase + PS2_MOHR_CAMERA_BASE_OFFSET_MMG_X, (float)MMGX);
	PS2_MEM_WriteFloat(camBase + PS2_MOHR_CAMERA_BASE_OFFSET_MMG_Y, (float)MMGY);
	}

	else {	
	//value of the read address is the address of the camera object

	float camX = PS2_MEM_ReadFloat(camBase + PS2_MOHR_CAMERA_BASE_OFFSET_X);
	float camY = PS2_MEM_ReadFloat(camBase + PS2_MOHR_CAMERA_BASE_OFFSET_Y);
	
	camX += (float)xmouse * looksensitivity / scale * -1 * (fov / 43.f);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * (fov / 43.f);
	camY = ClampFloat(camY, -1.450000048f, 1.450000048f);

	PS2_MEM_WriteFloat(camBase + PS2_MOHR_CAMERA_BASE_OFFSET_X, (float)camX);
	PS2_MEM_WriteFloat(camBase + PS2_MOHR_CAMERA_BASE_OFFSET_Y, (float)camY);
	}
}