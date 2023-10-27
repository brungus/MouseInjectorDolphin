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

#define DOC_CAM_BASE_PTR 0x4ED554
#define DOC_CAM_BASE_SANITY_1_VALUE 0xE8F94900
#define DOC_CAM_BASE_SANITY_2_VALUE 0xE0391300
// offsets from camBase
#define DOC_CAM_BASE_SANITY_1 0x50
#define DOC_CAM_BASE_SANITY_2 0x54
#define DOC_FOV 0x170
#define DOC_CAMY 0x174
#define DOC_CAMX 0x178

#define DOC_CURSORX 0x46ACA0
#define DOC_CURSORY 0x46ACA4

#define DOC_IS_BUSY 0x47CC94

// #define DOC_IS_PAUSED 0x4EB318
#define DOC_IS_NOT_PAUSED 0x4F0738

static uint8_t PS2_DOC_Status(void);
static uint8_t PS2_DOC_DetectCamBase(void);
static void PS2_DOC_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Dirge of Cerberus: Final Fantasy VII",
	PS2_DOC_Status,
	PS2_DOC_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_DIRGEOFCERBERUS = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_DOC_Status(void)
{
	// SLUS_214.19
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323134U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E31393BU);
}

static uint8_t PS2_DOC_DetectCamBase(void)
{
	// check if current camBase still valid
	if (PS2_MEM_ReadWord(camBase + DOC_CAM_BASE_SANITY_1) == DOC_CAM_BASE_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(camBase + DOC_CAM_BASE_SANITY_2) == DOC_CAM_BASE_SANITY_2_VALUE)
	{
		return 1;
	}

	uint32_t tempCamBase = PS2_MEM_ReadUInt(DOC_CAM_BASE_PTR);
	if (tempCamBase &&
		PS2_MEM_ReadWord(tempCamBase + DOC_CAM_BASE_SANITY_1) == DOC_CAM_BASE_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(tempCamBase + DOC_CAM_BASE_SANITY_2) == DOC_CAM_BASE_SANITY_2_VALUE)
	{
		camBase = tempCamBase;
		return 1;
	}

	return 0;
}

static void PS2_DOC_Inject(void)
{
	// TODO: update map arrow direction when map up and not moving
	//			change map transparency when moving mouse
	// TODO: do FULL game test
	// TODO: test on latest PCSX2 and start using the latest one for new hacks


	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	if (!PS2_MEM_ReadUInt(DOC_IS_NOT_PAUSED))
	{
		float looksensitivity = (float)sensitivity / 40.f;

		float cursorX = PS2_MEM_ReadFloat(DOC_CURSORX);
		float cursorY = PS2_MEM_ReadFloat(DOC_CURSORY);

		cursorX += (float)xmouse * looksensitivity;
		cursorY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;

		cursorX = ClampFloat(cursorX, 8.f, 504.f);
		cursorY = ClampFloat(cursorY, 8.f, 440.f);

		PS2_MEM_WriteFloat(DOC_CURSORX, cursorX);
		PS2_MEM_WriteFloat(DOC_CURSORY, cursorY);

		return;
	}

	if (PS2_MEM_ReadUInt(DOC_IS_BUSY))
		return;

	if (!PS2_DOC_DetectCamBase())
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 200.f;
	float fov = PS2_MEM_ReadFloat(camBase + DOC_FOV) / 73.5f;

	float camX = PS2_MEM_ReadFloat(camBase + DOC_CAMX);
	camX += (float)xmouse * looksensitivity * fov / scale;
	PS2_MEM_WriteFloat(camBase + DOC_CAMX, (float)camX);

	float camY = PS2_MEM_ReadFloat(camBase + DOC_CAMY);
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * fov / scale;
	camY = ClampFloat(camY, -0.9424778819f, 0.87266469f);
	PS2_MEM_WriteFloat(camBase + DOC_CAMY, (float)camY);

}