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

#define RMD_ROT_X 0xF49AD6
#define RMD_CAMY 0xF49B88
#define RMD_CAMX_WALKING 0xF40550
#define RMD_CAMY_WALKING 0xF40554

// #define MML_IS_BUSY 0x98A5B
// #define MML_IS_ROOM_TRANSITION 0x98824
// #define MML_IS_MAP 0xB526D

static uint8_t PSP_RMD_Status(void);
static void PSP_RMD_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Rockman DASH",
	PSP_RMD_Status,
	PSP_RMD_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PSP_ROCKMANDASH = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PSP_RMD_Status(void)
{
	// ULJM05030
	return (PSP_MEM_ReadWord(0xA41E30) == 0x554C4A4DU && 
			PSP_MEM_ReadWord(0xA41E34) == 0x30353033U && 
			PSP_MEM_ReadWord(0xA41E38) == 0x30000000U);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PSP_RMD_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	uint16_t rotX = PSP_MEM_ReadUInt16(RMD_ROT_X);
	uint16_t camY = PSP_MEM_ReadUInt16(RMD_CAMY);
	float rotXF = (float)rotX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&rotXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// clamp y-axis while walking
	if (camYF > 700 && camYF < 32000)
		camYF = 700;
	if (camYF < 65000 && camYF > 32000)
		camYF = 65000;
	
	while (rotXF >= 4096)
		rotXF -= 4096;
	while (rotXF < 0)
		rotXF += 4096;

	// camera follows rotation of MM
	float cameraX = rotXF - 2048;
	while (cameraX > 4096)
		cameraX -= 4096;
	while (cameraX < 0)
		cameraX += 4096;

	PSP_MEM_WriteUInt16(RMD_ROT_X, (uint16_t)rotXF);
	PSP_MEM_WriteUInt16(RMD_CAMX_WALKING, (uint16_t)cameraX);
	PSP_MEM_WriteUInt16(RMD_CAMY, (uint16_t)camYF);
	PSP_MEM_WriteUInt16(RMD_CAMY_WALKING, (uint16_t)camYF);
}