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

//Based on Delta Force: Urban Warfare patch.
#define TAU 6.2831853f
#define PI 3.1415926f
//#define PS1_RS6_CAMERABASE_X_POINTER - it is pointer to pointer to a pointer, 

//TODO: detect difficulty, it flips operatives offsets, needs to be detected, okay, detect current operative -> different offsets, same base at least, ??? does number of operatives affect offsets (NO!)? On easy C offset is green, on medium C offset is blue...
#define PS1_RS6_CAMERA_Y 0xAE8A4
#define PS1_RS6_CAMERA_X_SIN 0xA0D24 //X-axis functions similiar to Delta Force, by Rebellion as well. 
#define PS1_RS6_CAMERA_X_COS 0xA0D2C
#define PS1_RS6_CAMERA_X_COS_N 0xA0D3C
#define PS1_RS6_CAMERA_X_SIN_2 0xA0D44
//#define PS1_RS6_FOV
//#define PS1_RS6_IS_PAUSED
//#define PS1_RS6_DIFFICULTY_FLAG
//#define PS1_RS6_SELECTED_OPERATIVE


static uint8_t PS1_RS6_Status(void);
static void PS1_RS6_Inject(void);
//static uint8_t PS1_RS6_DetectCamYBase(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Tom Clancy's Rainbow Six",
	PS1_RS6_Status,
	PS1_RS6_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS1_RS6 = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static uint32_t camYBase = 0;
static uint32_t camXBase = 0;
static uint32_t fovBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//========================================================================== 

static uint8_t PS1_RS6_Status(void)
{
	//53 4C 55 53 - 5F 30 30 39 - 2E 34 37 3B - SLUS_009.47
	return (PS1_MEM_ReadWord(0x92D4) == 0x534C5553U && PS1_MEM_ReadWord(0x92D8) == 0x5F303039U && PS1_MEM_ReadWord(0x92DC) == 0x2E34373BU);
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_RS6_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	int32_t camXSin = PS1_MEM_ReadInt(PS1_RS6_CAMERA_X_SIN);
	int32_t camXCos = PS1_MEM_ReadInt(PS1_RS6_CAMERA_X_COS);
	float camXSinF = (float)(floor(camXSin)) / 65535.f;
	float camXCosF = (float)(floor(camXCos)) / 65535.f;

	float angle = (float)atan((float)camXSinF / (float)camXCosF);
	const float scale = 5.f;
	const float looksensitivity = (float)sensitivity / 200.f;

	if (camXCos < 0)
		angle += PI;

	angle += (float)xmouse * looksensitivity / 20.f / scale;

	while (angle > TAU)
		angle -= TAU;
	while (angle < 0)
		angle += TAU;
	
	PS1_MEM_WriteInt(camXBase + PS1_RS6_CAMERA_X_SIN, (int32_t)((float)sin(angle) * 65535.f)); //From Delta Force patch, adjusted for R6.
	PS1_MEM_WriteInt(camXBase + PS1_RS6_CAMERA_X_SIN_2, (int32_t)((float)sin(angle) * 65535.f));
	PS1_MEM_WriteInt(camXBase + PS1_RS6_CAMERA_X_COS, (int32_t)((float)cos(angle) * 65535.f));
	PS1_MEM_WriteInt(camXBase + PS1_RS6_CAMERA_X_COS_N, -(int32_t)((float)cos(angle) * 65535.f));

	int32_t camY = PS1_MEM_ReadInt(camYBase + PS1_RS6_CAMERA_Y);
	float camYF = (float)camY;

	float ym = (float)(invertpitch ? -ymouse : ymouse * -1);
	float dy = -ym * looksensitivity * 20.f / scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

	// Y-axis clamping. Game uses ranges: UP 3328-4096 / DOWN 0-768; however, it allows exceding negative "DOWN" range, which moves the camera up.
	if (camYF > 768)
		camYF = 768;
	if (camYF < -768)
		camYF = -768;

	PS1_MEM_WriteInt(camYBase + PS1_RS6_CAMERA_Y, (int32_t)camYF);

}
