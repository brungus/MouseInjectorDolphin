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

#define TAU 6.2831853f // 0x40C90FDB

#define DAH_PLAYERBASE_POINTER 0x3E0A88 // also saucer base, changes when swapped
#define DAH_PLAYERBASE_SANITY 0x0
#define DAH_PLAYERBASE_SANITY_VALUE 0x0040E120
// offset from playerBase
#define DAH_PLAYER_PROPERTIES_BASE_POINTER 0x1C
#define DAH_CAMBASE_POINTER 0x134
// offsets from camBase
#define DAH_CAMY 0xA4
#define DAH_CAMX 0xBC

#define DAH_PKBASE_POINTER_POINTER 0x138 // offset from playerBase
#define DAH_PKBASE_POINTER 0x58 // offset from pkTargetBasePointer
#define DAH_PK_VALUE_1 0x54
#define DAH_PKBASE_SANITY_1_VALUE 0xD8074100
#define DAH_PKBASE_SANITY_2_VALUE 0xF74F93F4
// offset from PKBase
#define DAH_PKBASE_SANITY_1 0x0
#define DAH_PKBASE_SANITY_2 0x4


#define DAH_SAUCER_X_BASE_POINTER 0x130 // offset from saucerBase
// offset from saucerXBase
#define DAH_SAUCERX 0x18

#define DAH_PK_TARGET_XVEL_SIN 0x1B068F0
#define DAH_PK_TARGET_XVEL_COS 0x1B068F4
#define DAH_PK_TARGET_YVEL 0x1B068F8

#define DAH_BASE_TYPE_PLAYER_SANITY 0x0040E120
#define DAH_BASE_TYPE_SAUCER_SANITY 0x0040EE88

#define BASE_TYPE_PLAYER 1
#define BASE_TYPE_SAUCER 2

#define DAH_IS_BUSY 0x3EA808
#define DAH_BUSY_TRUE 0x3

#define DAH_RIGHT_ANALOG_X 0x3EAD6C
#define DAH_RIGHT_ANALOG_Y 0x3EAD78

#define DAH_IS_LOCKED_ON_SEARCH 0x3EACDC

#define DAH_PLAYER_PROPERTIES_SANITY_1_VALUE 0xE8C03A6C
#define DAH_PLAYER_PROPERTIES_SANITY_2_VALUE 0x4902AEAD
// offsets from playerPropertiesBase
#define DAH_PLAYER_PROPERTIES_SANITY_1 0x0
#define DAH_PLAYER_PROPERTIES_SANITY_2 0x50
#define DAH_PLAYER_GRAVITY 0x1CC

#define DAH_FPS 0x3EF80C

static uint8_t PS2_DAH_Status(void);
static uint8_t PS2_DAH_DetectPlayer(void);
static uint8_t PS2_DAH_DetectPKBase(void);
static uint8_t PS2_DAH_DetectPlayerProperties(void);
static void PS2_DAH_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Destroy All Humans",
	PS2_DAH_Status,
	PS2_DAH_Inject,
	1, // 1000 Hz tickrate
	0, // crosshair sway not supported for driver
	"Increase Crypto's Gravity (fix for 60FPS): Enabled",
	"Increase Crypto's Gravity (fix for 60FPS): Disabled",
};

const GAMEDRIVER *GAME_PS2_DESTROYALLHUMANS = &GAMEDRIVER_INTERFACE;

static float scale = 10000.f;
static uint32_t playerBase = 0;
static uint32_t camBase = 0;
static uint32_t saucerXBase = 0;
static uint32_t playerPropertiesBase = 0;
static uint8_t baseType = 0;
static uint8_t increaseGravity = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_DAH_Status(void)
{
	// SLUS_209.45
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323039U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E34353BU);
}

static uint8_t PS2_DAH_DetectPlayer(void)
{
	baseType = 0;
	uint32_t tempPlayerBase = PS2_MEM_ReadPointer(DAH_PLAYERBASE_POINTER);
	if (tempPlayerBase)
	{
		if (PS2_MEM_ReadUInt(tempPlayerBase + DAH_PLAYERBASE_SANITY) == DAH_BASE_TYPE_PLAYER_SANITY)
			baseType = BASE_TYPE_PLAYER;
		if (PS2_MEM_ReadUInt(tempPlayerBase + DAH_PLAYERBASE_SANITY) == DAH_BASE_TYPE_SAUCER_SANITY)
			baseType = BASE_TYPE_SAUCER;

		if (baseType)
		{
			playerBase = tempPlayerBase;
			return 1;
		}
	}
	return 0;
}

static uint8_t PS2_DAH_DetectPKBase(void)
{
	uint32_t tempPointer = PS2_MEM_ReadPointer(playerBase + DAH_PKBASE_POINTER_POINTER);
	uint32_t pkBase = PS2_MEM_ReadPointer(tempPointer + DAH_PKBASE_POINTER);
	if (pkBase &&
		PS2_MEM_ReadWord(pkBase + DAH_PKBASE_SANITY_1) == DAH_PKBASE_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(pkBase + DAH_PKBASE_SANITY_2) == DAH_PKBASE_SANITY_2_VALUE)
	{
		if (PS2_MEM_ReadUInt(pkBase + DAH_PK_VALUE_1))
			return 1;
	}
	return 0;
}

static uint8_t PS2_DAH_DetectPlayerProperties(void)
{
	uint32_t tempBase = PS2_MEM_ReadPointer(playerBase + DAH_PLAYER_PROPERTIES_BASE_POINTER);
	if(tempBase &&
		PS2_MEM_ReadWord(tempBase + DAH_PLAYER_PROPERTIES_SANITY_1) == DAH_PLAYER_PROPERTIES_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(tempBase + DAH_PLAYER_PROPERTIES_SANITY_2) == DAH_PLAYER_PROPERTIES_SANITY_2_VALUE)
	{
		playerPropertiesBase = tempBase;
		return 1;
	}
	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_DAH_Inject(void)
{
	// TODO: add to 60FPS cheat
	//			jetpack is broken, can keep going up with little boost

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// pause, save, hints, cutscenes, ship landing, entering ship, return to mothership
	if (PS2_MEM_ReadUInt(DAH_IS_BUSY) == DAH_BUSY_TRUE)
		return;

	float looksensitivity = (float)sensitivity / scale;

	if (!PS2_DAH_DetectPlayer())
		return;

	// set higher gravity for 60FPS
	if (!optionToggle)
	{
		if (PS2_MEM_ReadUInt(DAH_FPS) == 0x1)
		{
			if (PS2_DAH_DetectPlayerProperties())
			{
				if (FloatsEqual(PS2_MEM_ReadFloat(playerPropertiesBase + DAH_PLAYER_GRAVITY), -9.800000191))
					PS2_MEM_WriteFloat(playerPropertiesBase + DAH_PLAYER_GRAVITY, -12.75f);
			}
		}
	}
	
	if (baseType == BASE_TYPE_PLAYER)
	{
		if (PS2_DAH_DetectPKBase())
		{
			// emulate stick movement for PK
			float maxMouse = 45.f; // arbitrary value
			if (abs(xmouse) > 0)
			{
				float dir = (xmouse < 0) ? -1 : 1;
				float stickXPos = (float)xmouse * (float)sensitivity;
				if (stickXPos > maxMouse)
					stickXPos = maxMouse * dir;
				stickXPos = (stickXPos / maxMouse) * 0.9999999;
				PS2_MEM_WriteFloat(DAH_RIGHT_ANALOG_X, stickXPos);
			}
			if (abs(ymouse) > 0)
			{
				float dir = (ymouse < 0) ? -1 : 1;
				float stickYPos = (float)ymouse * (float)sensitivity;
				if (stickYPos > maxMouse)
					stickYPos = maxMouse * dir;
				stickYPos = (stickYPos / maxMouse) * 0.9999999;
				PS2_MEM_WriteFloat(DAH_RIGHT_ANALOG_Y, -stickYPos);
			}
		}
		else
		{
			camBase = PS2_MEM_ReadPointer(playerBase + DAH_CAMBASE_POINTER);

			float camX = PS2_MEM_ReadFloat(camBase + DAH_CAMX);
			float camY = PS2_MEM_ReadFloat(camBase + DAH_CAMY);

			camX -= (float)xmouse * looksensitivity;
			camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;

			camY = ClampFloat(camY, -1.047197342f, 0.3490658104f);

			PS2_MEM_WriteFloat(camBase + DAH_CAMX, (float)camX);
			PS2_MEM_WriteFloat(camBase + DAH_CAMY, (float)camY);
		}

	}
	else if (baseType == BASE_TYPE_SAUCER)
	{
		saucerXBase = PS2_MEM_ReadPointer(playerBase + DAH_SAUCER_X_BASE_POINTER);
		float saucerX = PS2_MEM_ReadFloat(saucerXBase + DAH_SAUCERX);
		saucerX -= (float)xmouse * looksensitivity;
		PS2_MEM_WriteFloat(saucerXBase + DAH_SAUCERX, (float)saucerX);
	}

}