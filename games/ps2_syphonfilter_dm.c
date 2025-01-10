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
#define PS2_SFDM_CAMERA_PTR 0x62CAD4
#define PS2_SFDM_CAMERA_BASE_OFFSET_X 0x5D4//X - axis
#define PS2_SFDM_CAMERA_BASE_OFFSET_Y 0x5CC//Y - axis
//#define PS2_SFDN_PAUSE_FLAG
#define PS2_SFDN_PRECISE_T_FLAG 0xB98040
#define PS2_SFDN_PRECISE_AIM_FLAG 0xA88830
#define PS2_SFDM_PTR_TO_PB 0x5F4EC8 //5F4EC8 -> POINTER + 0x12E0 -> POINTER + 0x14 -> POINTER + 0x3F8 -> AIM X
//this pointer chain is still working the right way, but when you die one of offsets moves somewhere, prolly 12E0, yes it points to one 3F800000, I think -> only sometimes cant replicate 100%, debugging this is fun as it happens only sometimes, restarting mission works, not ideal
#define PS2_SFDM_OFFSET_TO_PRECISE_AIM_BASE_PTR_1 0x12E0
#define PS2_SFDM_OFFSET_TO_PRECISE_AIM_BASE_PTR_2 0x14
#define PS2_SFDM_PRECISE_AIM_OFFSET_X 0x3F8
#define PS2_SFDM_PRECISE_AIM_OFFSET_Y 0x3F0
#define PS2_SFDM_PRECISE_COVER_AIM_PTR 0x623BD4 // I dont believe in this simple pointer anymore, it is not, but the same i figured aim one
#define PS2_SFDM_PRECISE_AIM_COVER_OFFSET_X 0x3F8 //COVER IS ANOTHER POINTER TO POINTER TO POINTER
#define PS2_SFDM_PRECISE_AIM_COVER_OFFSET_Y 0x3F0
#define PS2_SFDM_PRECISE_AIM_SAFETY_1 0x48C
#define PS2_SFDM_PRECISE_AIM_SAFETY_2 0x4C0
#define PS2_SFDM_PRECISE_AIM_SAFETY_1_VALUE 0x3F800000
#define PS2_SFDM_PRECISE_AIM_SAFETY_2_VALUE 0x00000000 //better value here
#define PS2_SFDM_FOV 0x4598B0 // value 64

static uint8_t PS2_SFDM_Status(void);
static uint8_t PS2_SFDM_DetectCamera(void);
static void PS2_SFDM_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Syphon Filter - Dark Mirror",
	PS2_SFDM_Status,
	PS2_SFDM_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_SFDM = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static uint32_t preciseaimbase = 0;
static uint32_t Aim_Flag = 0;
static uint32_t T_Flag = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_SFDM_Status(void)
{
	//SCUS_973.62;
	return (PS2_MEM_ReadWord(0x00093390) == 0x53435553 && PS2_MEM_ReadWord(0x00093394) == 0x5F393733) && PS2_MEM_ReadWord(0x00093398) == 0x2E36323B;
}

//==========================================================================
// Purpose: Camera object check. Don't write where you're not supposed to.
//==========================================================================
static uint8_t PS2_SFDM_DetectCamera(void)
{
	uint32_t spielerbase = PS2_MEM_ReadPointer(PS2_SFDM_PTR_TO_PB); //read value of 0x5F4EC8 (0x1BFF490)
	uint32_t pointeraimbase = PS2_MEM_ReadPointer(spielerbase + PS2_SFDM_OFFSET_TO_PRECISE_AIM_BASE_PTR_1); //read value of 0x1BFF490+0x12E0 (0xB370E0)
	uint32_t tempaimbase = PS2_MEM_ReadPointer(pointeraimbase + PS2_SFDM_OFFSET_TO_PRECISE_AIM_BASE_PTR_2); //read value of 0xB370E0+0x14 (0xCA6910)
	uint32_t tempcambase = PS2_MEM_ReadPointer(PS2_SFDM_CAMERA_PTR);

	if (tempaimbase &&
		PS2_MEM_ReadPointer(tempaimbase + PS2_SFDM_PRECISE_AIM_SAFETY_1) == PS2_SFDM_PRECISE_AIM_SAFETY_1_VALUE &&
		PS2_MEM_ReadPointer(tempaimbase + PS2_SFDM_PRECISE_AIM_SAFETY_2) == PS2_SFDM_PRECISE_AIM_SAFETY_2_VALUE)
	{
		preciseaimbase = tempaimbase;
	}

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
static void PS2_SFDM_Inject(void)
{
	if (!PS2_SFDM_DetectCamera())
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	float looksensitivity = (float)sensitivity;
	float scale = 10000.f;

	Aim_Flag = PS2_MEM_ReadPointer(PS2_SFDN_PRECISE_AIM_FLAG);
	T_Flag = PS2_MEM_ReadPointer(PS2_SFDN_PRECISE_T_FLAG);

	if (Aim_Flag == 0x3F800000 || T_Flag == 0x00000000 &&
	 PS2_MEM_ReadPointer(preciseaimbase + PS2_SFDM_PRECISE_AIM_SAFETY_1) == PS2_SFDM_PRECISE_AIM_SAFETY_1_VALUE &&
	 PS2_MEM_ReadPointer(preciseaimbase + PS2_SFDM_PRECISE_AIM_SAFETY_2) == PS2_SFDM_PRECISE_AIM_SAFETY_2_VALUE) //dont dont dont write there unless really these
	
	{
	float camXa = PS2_MEM_ReadFloat(preciseaimbase + PS2_SFDM_PRECISE_AIM_OFFSET_X);
	float camYa = PS2_MEM_ReadFloat(preciseaimbase + PS2_SFDM_PRECISE_AIM_OFFSET_Y);
	
	camXa -= (float)xmouse * looksensitivity / scale;
	camYa -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;

	PS2_MEM_WriteFloat(preciseaimbase + PS2_SFDM_PRECISE_AIM_OFFSET_X, (float)camXa);
	PS2_MEM_WriteFloat(preciseaimbase + PS2_SFDM_PRECISE_AIM_OFFSET_Y, (float)camYa);
	}

	float camX = PS2_MEM_ReadFloat(camBase + PS2_SFDM_CAMERA_BASE_OFFSET_X);
	float camY = PS2_MEM_ReadFloat(camBase + PS2_SFDM_CAMERA_BASE_OFFSET_Y);
	
	camX -= (float)xmouse * looksensitivity / scale;
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;

	PS2_MEM_WriteFloat(camBase + PS2_SFDM_CAMERA_BASE_OFFSET_X, (float)camX);
	PS2_MEM_WriteFloat(camBase + PS2_SFDM_CAMERA_BASE_OFFSET_Y, (float)camY);
}