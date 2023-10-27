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

#define GE_ONFOOT_PLAYER_POINTER 0x805D43EC
#define GE_ONFOOT_PLAYER_SANITY_1_VALUE 0x805733AC
#define GE_ONFOOT_PLAYER_SANITY_2_VALUE 0x80587420
// offsets from playerBase
#define GE_ONFOOT_PLAYER_SANITY_1 0x48
#define GE_ONFOOT_PLAYER_SANITY_2 0x70
// #define GE_ONFOOT_IS_ACTIONABLE 0x57C
#define GE_ONFOOT_CAMY 0xC64
#define GE_ONFOOT_FOV 0x71C

#define GE_ONFOOT_CAMXBASE_POINTER 0x806EB698
// offsets from camXBase
#define GE_ONFOOT_CAMX 0xDC
#define GE_ACTIONABLE_BASE_POINTER 0x5C
// offset from isActionableBase
#define GE_IS_VAULTING 0xB8D // byte
#define GE_IS_NOT_SUBDUE 0xB8E // byte

#define GE_IS_BUSY 0x805C9560

#define GE_ACTIONABLE 0x0000FFFF

static uint8_t WII_GE_Status(void);
static uint8_t WII_GE_DetectPlayerBase(void);
static void WII_GE_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"GoldenEye: 007",
	WII_GE_Status,
	WII_GE_Inject,
	1, // if tickrate is any lower, mouse input will get sluggish
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_WII_GOLDENEYE007 = &GAMEDRIVER_INTERFACE;

static uint32_t playerBase = 0;
static float scale = 300.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t WII_GE_Status(void)
{
	// SJBE52
	return (MEM_ReadUInt(0x80000000) == 0x534A4245U && 
			MEM_ReadUInt(0x80000004) == 0x35320000U);
}

static uint8_t WII_GE_DetectPlayerBase(void)
{
	uint32_t tempCamBase = MEM_ReadUInt(GE_ONFOOT_PLAYER_POINTER);
	if (tempCamBase &&
		MEM_ReadUInt(tempCamBase + GE_ONFOOT_PLAYER_SANITY_1) == GE_ONFOOT_PLAYER_SANITY_1_VALUE &&
		MEM_ReadUInt(tempCamBase + GE_ONFOOT_PLAYER_SANITY_2) == GE_ONFOOT_PLAYER_SANITY_2_VALUE)
	{
		playerBase = tempCamBase;
		return 1;
	}
	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void WII_GE_Inject(void)
{
	// TODO: fix ADS stuck on initial draw
	//			might be same as story camera
	// TODO: fix gun jittering
	//			probably just requires some other values be update
	//			look for redundant camera values
	//			or lock gun sway
	// TODO: story camera (mission 2 beginning, truck ride, slowmo)
	// TODO: change camYBase to playerBase
	// TODO: need different action value
	
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	if (MEM_ReadUInt(GE_IS_BUSY)) // pause, hint
		return;

	if (!WII_GE_DetectPlayerBase())
		return;

	uint32_t camXBase = MEM_ReadUInt(GE_ONFOOT_CAMXBASE_POINTER);
	if (!camXBase)
		return;
	
	uint32_t actionableBase = MEM_ReadUInt(camXBase + GE_ACTIONABLE_BASE_POINTER);
	if (MEM_ReadUInt8(actionableBase + GE_IS_VAULTING))
		return;
	if (!MEM_ReadUInt8(actionableBase + GE_IS_NOT_SUBDUE))
		return;
	
	const float looksensitivity = (float)sensitivity / 40.f;
	float fov = MEM_ReadFloat(playerBase + GE_ONFOOT_FOV) / 49.f;

	float camX = MEM_ReadFloat(camXBase + GE_ONFOOT_CAMX);
	camX += (float)xmouse * looksensitivity / scale * fov;
	// while (camX >= TAU)
	// 	camX -= TAU;
	// while (camX < 0)
	// 	camX += TAU;

	float camY = MEM_ReadFloat(playerBase + GE_ONFOOT_CAMY);
	// camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * (360.f / TAU) / scale * fov;
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * fov;
	camY = ClampFloat(camY, -0.7853981853f, 0.7853981853f);

	MEM_WriteFloat(camXBase + GE_ONFOOT_CAMX, camX);
	MEM_WriteFloat(playerBase + GE_ONFOOT_CAMY, camY);
}