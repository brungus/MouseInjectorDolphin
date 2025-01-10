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
#define PS2_MOHF_CAMERA_BASE_POINTER 0x23F4A8
#define PS2_MOHF_CAMERA_BASE_OFFSET_X 0x2A4
#define PS2_MOHF_CAMERA_BASE_OFFSET_Y 0x2A8
#define PS2_MOHF_CAMERA_BASE_OFFSET_FOV 0x43C//zoom sensitivity
#define PS2_MOHF_GLOBAL_FOV 0x1455C8//in the end not needed, but working leftover
#define PS2_MOHF_IS_MMG_FLAG 0x33C728//detect machine gun
#define PS2_MOHF_CAMERA_BASE_OFFSET_MMG_X 0x42C//offset for mg - X axis
#define PS2_MOHF_CAMERA_BASE_OFFSET_MMG_Y 0x430//offset for mg - Y axis

static uint8_t PS2_MOHF_Status(void);
static uint8_t PS2_MOHF_DetectCamera(void);
static void PS2_MOHF_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Medal of Honor: Frontline",
	PS2_MOHF_Status,
	PS2_MOHF_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_MOHFRONTLINE = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static uint32_t tempcambase = 0;
static uint16_t T_Flag = 0;
static uint16_t fov16 = 0;
static uint32_t fov32 = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_MOHF_Status(void)
{
	//SLUS_203.68 - 53 4C 55 53 5F 32 30 33 2E 36 38
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553 && PS2_MEM_ReadWord(0x00093394) == 0x5F323033) && PS2_MEM_ReadWord(0x00093398) == 0x2E36383B;
}


//==========================================================================
// Purpose: Camera object check. Don't write where you're not supposed to.
//==========================================================================
static uint8_t PS2_MOHF_DetectCamera(void)
{
	uint32_t tempcambase = PS2_MEM_ReadPointer(PS2_MOHF_CAMERA_BASE_POINTER);
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
static void PS2_MOHF_Inject(void)
{
	if (!PS2_MOHF_DetectCamera())
		return;
	
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	//sensitivity
	float looksensitivity = (float)sensitivity;
	float scale = 10000.f;

	T_Flag = PS2_MEM_ReadUInt16(PS2_MOHF_IS_MMG_FLAG);
	fov16 = PS2_MEM_ReadUInt16(PS2_MOHF_GLOBAL_FOV);//read the last 16 bit part of the fov op
	fov32 = fov16 << 16;//move the 16 bits
	float globalfov = *((float *)&fov32);//turn the hex value into a float
	float fov = PS2_MEM_ReadFloat(camBase + PS2_MOHF_CAMERA_BASE_OFFSET_FOV);
	
	if (T_Flag == 0x0000)
	{
	float MMGX = PS2_MEM_ReadFloat(camBase + PS2_MOHF_CAMERA_BASE_OFFSET_MMG_X);
	float MMGY = PS2_MEM_ReadFloat(camBase + PS2_MOHF_CAMERA_BASE_OFFSET_MMG_Y);
	
	MMGX += (float)xmouse * looksensitivity / scale * -1 * (fov / globalfov);
	MMGY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * (fov / globalfov);

	PS2_MEM_WriteFloat(camBase + PS2_MOHF_CAMERA_BASE_OFFSET_MMG_X, (float)MMGX);
	PS2_MEM_WriteFloat(camBase + PS2_MOHF_CAMERA_BASE_OFFSET_MMG_Y, (float)MMGY);
	}

	else {	
	//value of the read address is the address of the camera object

	float camX = PS2_MEM_ReadFloat(camBase + PS2_MOHF_CAMERA_BASE_OFFSET_X);
	float camY = PS2_MEM_ReadFloat(camBase + PS2_MOHF_CAMERA_BASE_OFFSET_Y);
	
	camX += (float)xmouse * looksensitivity / scale * -1 * (fov / globalfov);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * (fov / globalfov);
	camY = ClampFloat(camY, -1.450000048f, 1.450000048f);

	PS2_MEM_WriteFloat(camBase + PS2_MOHF_CAMERA_BASE_OFFSET_X, (float)camX);
	PS2_MEM_WriteFloat(camBase + PS2_MOHF_CAMERA_BASE_OFFSET_Y, (float)camY);
	}
}