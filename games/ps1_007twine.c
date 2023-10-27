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

#define TWINE_CAM_AIM_BASE_POINTER 0x84E0
#define TWINE_BASE_SANITY_1_VALUE 0x000B
#define TWINE_BASE_SANITY_2_VALUE 0x800B
// offsets from camAimBase
#define TWINE_BASE_SANITY_1 0x12
#define TWINE_BASE_SANITY_2 0x46
#define TWINE_CAM_IS_AIMING -0x8124 // actually control bits
#define TWINE_CAM_ZOOM -0x82F4
#define TWINE_CAMX_AIM -0x830C
#define TWINE_CAMY_AIM -0x8304
#define TWINE_CAMY_BASE_POINTER -0x833C
// offset from camYBase
#define TWINE_CAMY 0x50

#define TWINE_ROTX_BASE_POINTER -0x8468 // offset from camAimBase
// offset from rotXBase
#define TWINE_ROTX -0x102E

#define TWINE_IS_PAUSED_BASE_POINTER 0x8570
// offset from isPausedBase
#define TWINE_IS_PAUSED 0x73A4
#define TWINE_IS_CUTSCENE 0x75FC

static uint8_t PS1_TWINE_Status(void);
static void PS1_TWINE_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"007 The World Is Not Enough",
	PS1_TWINE_Status,
	PS1_TWINE_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_007THEWORLDISNOTENOUGH = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static uint32_t camAimBase = 0;
static uint32_t camYBase = 0;
static uint32_t rotXBase = 0;
static uint8_t isAiming = 0;
static uint8_t isAimingLast = 0;
static float aimingCenter = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_TWINE_Status(void)
{
	// SLUS_012.72
	return (PS1_MEM_ReadWord(0x9394) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x9398) == 0x5F303132U && 
			PS1_MEM_ReadWord(0x939C) == 0x2E37323BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_TWINE_Inject(void)
{
	// TODO: use menu (60 FPS) to find FPS cheat
	// TODO: lean aim boundaries
	// TODO: new patch that will work with Beetle PSX
	//			make sure to record all ops that are changed
	//			instead of nop, try changing to an op that
	//			similarly does nothing, like add zero, as the
	//			nops could be the issue

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	uint32_t isPausedBase = PS1_MEM_ReadPointer(TWINE_IS_PAUSED_BASE_POINTER);
	if (PS1_MEM_ReadHalfword(isPausedBase + TWINE_IS_PAUSED))
		return;
	if (PS1_MEM_ReadHalfword(isPausedBase + TWINE_IS_CUTSCENE))
		return;
	
	camAimBase = PS1_MEM_ReadPointer(TWINE_CAM_AIM_BASE_POINTER);
	if (PS1_MEM_ReadHalfword(camAimBase + TWINE_BASE_SANITY_1) != TWINE_BASE_SANITY_1_VALUE &&
		PS1_MEM_ReadHalfword(camAimBase + TWINE_BASE_SANITY_2) != TWINE_BASE_SANITY_2_VALUE)
	{
		return;
	}
	camYBase = PS1_MEM_ReadPointer(camAimBase + TWINE_CAMY_BASE_POINTER);
	rotXBase = PS1_MEM_ReadPointer(camAimBase + TWINE_ROTX_BASE_POINTER);
	
	uint16_t rotX = PS1_MEM_ReadHalfword(rotXBase + TWINE_ROTX);
	int16_t camY = PS1_MEM_ReadInt16(camYBase + TWINE_CAMY);
	uint16_t zoom = PS1_MEM_ReadHalfword(camAimBase + TWINE_CAM_ZOOM);
	if (!zoom) zoom = 1;
	float rotXF = (float)rotX;
	float camYF = (float)camY;
	float zoomF = (float)zoom / 4000.f;
	if (zoom == 1) zoomF = 1.f;

	const float looksensitivity = (float)sensitivity / 20.f;
	float scale = 1.f / zoomF;

	// isAiming = PS1_MEM_ReadByte(camAimBase + TWINE_CAM_IS_AIMING);

	// if (isAimingLast != isAiming)
	// {
	// 	// save rotX when started aiming
	// 	aimingCenter = rotXF;
	// }

	float dx = (float)xmouse * looksensitivity * scale * zoomF;
	AccumulateAddRemainder(&rotXF, &xAccumulator, xmouse, dx);

	// if (isAiming)
	// 	rotXF = ClampFloat(rotXF, aimingCenter - 340.f, aimingCenter + 340.f);

	// isAimingLast = isAiming;


	while (rotXF > 4096)
		rotXF -= 4096;
	while (rotXF < 0)
		rotXF += 4096;

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * scale * zoomF;
	AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

	// camY should be on a scale from -2048 to 2048
	while (camYF > 2048)
		camYF -= 4096;
	while (camYF < -2048)
		camYF += 4096;

	camYF = ClampFloat(camYF, -640.f, 800.f);

	PS1_MEM_WriteHalfword(rotXBase + TWINE_ROTX, (uint16_t)rotXF);
	PS1_MEM_WriteHalfword(camAimBase + TWINE_CAMX_AIM, (uint16_t)rotXF);
	PS1_MEM_WriteInt16(camYBase + TWINE_CAMY, (int16_t)camYF);
	PS1_MEM_WriteInt(camAimBase + TWINE_CAMY_AIM, (int32_t)camYF);
}