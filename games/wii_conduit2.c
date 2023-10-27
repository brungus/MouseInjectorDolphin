//==========================================================================
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

#define C2_CAMY 0x82965784
#define C2_CAMX_BASE_POINTER 0x8296C1FC
// offset from camXBase
#define C2_HAND_SIN_X 0x50
#define C2_HAND_COS_X 0x58
#define C2_CAMX 0x294

#define C2_FACE_DIR_SIN_X 0x80826A20
#define C2_FACE_DIR_COS_X 0x80826A28

static uint8_t WII_C2_Status(void);
static uint8_t WII_C2_DetectCamXBase(void);
static void WII_C2_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Conduit 2",
	WII_C2_Status,
	WII_C2_Inject,
	1, // if tickrate is any lower, mouse input will get sluggish
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_WII_CONDUIT2 = &GAMEDRIVER_INTERFACE;

static uint32_t camXBase = 0;
static float scale = 1.5f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t WII_C2_Status(void)
{
	// SC2E8P
	return (MEM_ReadUInt(0x80000000) == 0x53433245U && 
			MEM_ReadUInt(0x80000004) == 0x38500000U);
}

static uint8_t WII_C2_DetectCamXBase(void)
{
	uint32_t tempCamBase = MEM_ReadUInt(C2_CAMX_BASE_POINTER);
	if (tempCamBase)// &&
		// MEM_ReadUInt(tempCamBase + GE_ONFOOT_PLAYER_SANITY_1) == GE_ONFOOT_PLAYER_SANITY_1_VALUE &&
		// MEM_ReadUInt(tempCamBase + GE_ONFOOT_PLAYER_SANITY_2) == GE_ONFOOT_PLAYER_SANITY_2_VALUE)
	{
		camXBase = tempCamBase;
		return 1;
	}
	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void WII_C2_Inject(void)
{
	// TODO: need extended MEM functions for Wii
	//			also new MEM limit checks, and change original one back
	// TODO: gun sway

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	if (!WII_C2_DetectCamXBase())
		return;
	// WII_C2_DetectCamXBase();
	// camXBase = MEM_ReadUInt(C2_CAMX_BASE_POINTER);
	
	const float looksensitivity = (float)sensitivity / 40.f;
	// float fov = MEM_ReadFloat(playerBase + GE_ONFOOT_FOV) / 49.f;
	float fov = 1.f;

	float camX = MEM_ReadFloat(camXBase + C2_CAMX);
	camX += (float)xmouse * looksensitivity * scale * fov;

	while (camX < 0)
		camX += 4096.f;
	while (camX >= 4096.f)
		camX -= 4096.f;

	// float handSinX = MEM_ReadFloat(camXBase + C2_HAND_SIN_X);
	// float handCosX = MEM_ReadFloat(camXBase + C2_HAND_COS_X);

	float angle = (camX / 4096.f) * TAU;
	float handSinX = sin(angle);
	float handCosX = cos(angle);
	MEM_WriteFloat(camXBase + C2_HAND_SIN_X, -(float)handSinX);
	MEM_WriteFloat(camXBase + C2_HAND_COS_X, -(float)handCosX);
	// MEM_WriteFloat(C2_FACE_DIR_SIN_X, handSinX);
	// MEM_WriteFloat(C2_FACE_DIR_COS_X, handCosX);

	float camY = MEM_ReadFloat(C2_CAMY);
	// camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * (360.f / TAU) / scale * fov;
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * scale * fov;
	camY = ClampFloat(camY, -800.f, 800.f);

	MEM_WriteFloat(camXBase + C2_CAMX, camX);
	MEM_WriteFloat(C2_CAMY, camY);
}