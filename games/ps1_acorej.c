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

#define ACJ_CAMY 0x41218
#define ACJ_CAMX 0x1A2592
#define ACJ_IS_NOT_BUSY 0x1AC6D4
#define ACJ_IS_NOT_PAUSED 0x39AFC
#define ACJ_IS_MAP_OPEN 0x14C6CB
#define ACJ_IS_ABORT_PROMPT 0x1FE06E

#define ACJ_REV1_CAMX 0x1A264A
#define ACJ_REV1_IS_MAP_OPEN 0x14C783
#define ACJ_REV1_IS_NOT_BUSY 0x1AC78C

#define ACJ_REVISION_NUMBER 0x929C
#define ACJ_REVISION_1 0x26

static uint8_t PS1_ACJ_Status(void);
static void PS1_ACJ_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Armored Core (Japan)",
	PS1_ACJ_Status,
	PS1_ACJ_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_ARMOREDCOREJAPAN = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static uint32_t camXAddress = ACJ_CAMX;
static uint32_t isMapOpenAddress = ACJ_IS_MAP_OPEN;
static uint32_t isNotBusyAddress = ACJ_IS_NOT_BUSY;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_ACJ_Status(void)
{
	// SLPS_009.00
	return (PS1_MEM_ReadWord(0x928C) == 0x534C5053U && 
			PS1_MEM_ReadWord(0x9290) == 0x5F303039U && 
			PS1_MEM_ReadWord(0x9294) == 0x2E30303BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_ACJ_Inject(void)
{
	// TODO: set idle rotating animation with xmouse input
	// TODO: optional setting to account for AC turning speed

	if (PS1_MEM_ReadByte(ACJ_REVISION_NUMBER) == ACJ_REVISION_1)
	{
		camXAddress = ACJ_REV1_CAMX;
		isMapOpenAddress = ACJ_REV1_IS_MAP_OPEN;
		isNotBusyAddress = ACJ_REV1_IS_NOT_BUSY;
	}

	if (!PS1_MEM_ReadByte(isNotBusyAddress))
		return;
	
	if (!PS1_MEM_ReadByte(ACJ_IS_NOT_PAUSED))
		return;
	
	if (PS1_MEM_ReadByte(isMapOpenAddress))
		return;

	if (PS1_MEM_ReadByte(ACJ_IS_ABORT_PROMPT) == 0x1A)
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(camXAddress);
	uint16_t camY = PS1_MEM_ReadHalfword(ACJ_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = -(float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	PS1_MEM_WriteHalfword(camXAddress, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(ACJ_CAMY, (uint16_t)camYF);
}