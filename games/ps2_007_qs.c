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
//pointer to a pointer 4D87FC, AE4 offset to cam x, so many pointers, 
//i ll need debugger for this, AE4 an offset, it shows addresses near the camera base...
//debugger points to register s4 (54DFF0), v0 calc will lead me to this
//debugged this base: pointer + offset -> pointer -> address
#define PS2_007QS_PTR_TO_CAMERA_PTR 0x3CA1C8
#define PS2_007QS_PTR_TO_CAMERA_PTR_OFFSET 0xC14//set static by this instruction - lw s4,0x14C(v0)
#define PS2_007QS_CAMERA_BASE_OFFSET_X 0xAE4//X - axis
#define PS2_007QS_CAMERA_BASE_OFFSET_Y 0xAE0//Y - axis
#define PS2_007QS_FOV 0x4598B0 // value 64

static uint8_t PS2_007QS_Status(void);
static uint8_t PS2_007QS_DetectCamera(void);
static void PS2_007QS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"007 - Quantum of Solace",
	PS2_007QS_Status,
	PS2_007QS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_007QS = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static uint32_t tempcambase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_007QS_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553 && PS2_MEM_ReadWord(0x00093394) == 0x5F323138) && PS2_MEM_ReadWord(0x00093398) == 0x2E31333B;
}

//==========================================================================
// Purpose: Camera object check. Don't write where you're not supposed to.
//==========================================================================
static uint8_t PS2_007QS_DetectCamera(void)
{
	uint32_t pointertocambase = PS2_MEM_ReadPointer(PS2_007QS_PTR_TO_CAMERA_PTR);
	uint16_t offsettocambase = 0x14C;
	tempcambase = PS2_MEM_ReadPointer(pointertocambase + offsettocambase);
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
static void PS2_007QS_Inject(void)
{
	if (!PS2_007QS_DetectCamera())
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	//sensitivity
	float fov = PS2_MEM_ReadFloat(PS2_007QS_FOV);
	float looksensitivity = (float)sensitivity;
	float scale = 10000.f;
	
	//value of the read address is the address of the camera object

	float camX = PS2_MEM_ReadFloat(camBase + PS2_007QS_CAMERA_BASE_OFFSET_X);
	float camY = PS2_MEM_ReadFloat(camBase + PS2_007QS_CAMERA_BASE_OFFSET_Y);
	
	camX += (float)xmouse * looksensitivity / scale * (fov / 64.f);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * (fov / 64.f);

	PS2_MEM_WriteFloat(camBase + PS2_007QS_CAMERA_BASE_OFFSET_X, (float)camX);
	PS2_MEM_WriteFloat(camBase + PS2_007QS_CAMERA_BASE_OFFSET_Y, (float)camY);
}