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

#define DAH2_PLAYERBASE_POINTER 0x4E0020
#define DAH2_PLAYERBASE_SANITY 0x0
#define DAH2_PLAYERBASE_SANITY_VALUE 0x008A4D00
// offset from playerBase
#define DAH2_CAMBASE_POINTER 0x234
// offsets from camBase
#define DAH2_CAMY 0xC4
#define DAH2_CAMX 0xDC

#define DAH2_SAUCER_BASE_POINTER_4 0x494434
#define DAH2_SAUCER_BASE_SANITY_1 0x0
#define DAH2_SAUCER_BASE_SANITY_1_VALUE 0x004DAB18
#define DAH2_SAUCER_ROTX 0x14 // offset from final pointer

#define DAH2_SAUCER_POSY_BASE_POINTER_6 0x494DF8
#define DAH2_SAUCER_POSY_BASE_SANITY_1 0x0
#define DAH2_SAUCER_POSY_BASE_SANITY_1_VALUE 0x004E3A58
// offsets from saucerPosYBase
#define DAH2_SAUCER_POSY 0x78
#define DAH2_SAUCER_POSY2 0xB8
#define DAH2_SAUCER_POSY3 0x158
#define DAH2_SAUCER_POSY4 0x68
#define DAH2_IN_SAUCER 0x2C

#define DAH2_RIGHT_ANALOG_STICK_BASE_POINTER_2 0x4944EC
#define DAH2_RIGHT_ANALOG_STICK_BASE_SANITY_1 0x0
#define DAH2_RIGHT_ANALOG_STICK_BASE_SANITY_1_VALUE 0x004DD060 // more of a pk sanity
// offsets from rightAnalogStickBase
#define DAH2_RIGHT_ANALOG_STICK_X 0x224
#define DAH2_RIGHT_ANALOG_STICK_Y 0x228
#define DAH2_RIGHT_ANALOG_STICK_2_BASE_POINTER 0x94
// offsets from rightAnalogStick2Base
#define DAH2_RIGHT_ANALOG_STICK_2_X 0x3C
#define DAH2_RIGHT_ANALOG_STICK_2_Y 0x40

#define DAH2_RIGHT_ANALOG_STICK_X_STATIC_1 0x494AB4
#define DAH2_RIGHT_ANALOG_STICK_X_STATIC_2 0x494AB8
#define DAH2_RIGHT_ANALOG_STICK_Y_STATIC_1 0x494AC0
#define DAH2_RIGHT_ANALOG_STICK_Y_STATIC_2 0x494AC4

#define DAH2_IS_LOCKED_ON 0x600EB0
#define IS_LOCKED_ON_TRUE 0xBB956C0B

#define DAH2_IS_BUSY 0x4CA540
#define DAH2_IS_BUSY2 0x4CD6C4

static uint8_t PS2_DAH2_Status(void);
static uint8_t PS2_DAH2_DetectPlayer(void);
static uint8_t PS2_DAH2_DetectSaucer(void);
static uint8_t PS2_DAH2_DetectSaucerPosY(void);
static uint8_t PS2_DAH2_DetectRightAnalogStickBase(void);
static void PS2_DAH2_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Destroy All Humans 2",
	PS2_DAH2_Status,
	PS2_DAH2_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_DESTROYALLHUMANS2 = &GAMEDRIVER_INTERFACE;

static float scale = 15000.f;
static uint32_t playerBase = 0;
static uint32_t camBase = 0;
static uint32_t saucerBase = 0;
static uint32_t saucerPosYBase = 0;
static uint32_t rightAnalogStickBase = 0;
static uint32_t playerPropertiesBase = 0;
static uint8_t baseType = 0;
static uint8_t increaseGravity = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_DAH2_Status(void)
{
	// SLUS_214.39
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323134U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E33393BU);
}

static uint8_t PS2_DAH2_DetectPlayer(void)
{
	baseType = 0;
	uint32_t tempPlayerBase = PS2_MEM_ReadPointer(DAH2_PLAYERBASE_POINTER);
	if (tempPlayerBase)
	{
		if (PS2_MEM_ReadWord(tempPlayerBase + DAH2_PLAYERBASE_SANITY) == DAH2_PLAYERBASE_SANITY_VALUE)
		{
			playerBase = tempPlayerBase;
			return 1;
		}
	}
	return 0;
}

static uint8_t PS2_DAH_DetectSaucer(void)
{
	uint32_t tempBase = PS2_MEM_ReadPointer(DAH2_SAUCER_BASE_POINTER_4) + 0x8; // 3
	tempBase = PS2_MEM_ReadPointer(tempBase) + 0x0; // 2
	tempBase = PS2_MEM_ReadPointer(tempBase) + 0x180; // 1
	tempBase = PS2_MEM_ReadPointer(tempBase);
	if (tempBase &&
		PS2_MEM_ReadUInt(tempBase + DAH2_SAUCER_BASE_SANITY_1) == DAH2_SAUCER_BASE_SANITY_1_VALUE)
	{
		saucerBase = tempBase;
		return 1;
	}
	return 0;
}

static uint8_t PS2_DAH2_DetectSaucerPosY(void)
{
	uint32_t tempBase = PS2_MEM_ReadPointer(DAH2_SAUCER_POSY_BASE_POINTER_6) + 0x8; // 5
	tempBase = PS2_MEM_ReadPointer(tempBase) + 0x0; // 4
	tempBase = PS2_MEM_ReadPointer(tempBase) + 0x34; // 3
	tempBase = PS2_MEM_ReadPointer(tempBase) + 0x0; // 2
	tempBase = PS2_MEM_ReadPointer(tempBase) + 0xBC; // 1
	tempBase = PS2_MEM_ReadPointer(tempBase); // base

	if (tempBase &&
		PS2_MEM_ReadUInt(tempBase + DAH2_SAUCER_POSY_BASE_SANITY_1) == DAH2_SAUCER_POSY_BASE_SANITY_1_VALUE)
	{
		saucerPosYBase = tempBase;
		return 1;
	}
	return 0;
}

static uint8_t PS2_DAH2_DetectRightAnalogStickBase(void)
{
	uint32_t tempBase = PS2_MEM_ReadPointer(DAH2_RIGHT_ANALOG_STICK_BASE_POINTER_2) + 0xA4; // 1
	tempBase = PS2_MEM_ReadPointer(tempBase); // base
	if (tempBase &&
		PS2_MEM_ReadUInt(tempBase + DAH2_RIGHT_ANALOG_STICK_BASE_SANITY_1) == DAH2_RIGHT_ANALOG_STICK_BASE_SANITY_1_VALUE)
	{
		rightAnalogStickBase = tempBase;
		return 1;
	}
	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_DAH2_Inject(void)
{
	// TODO: disable camX movement while full sprint
	// TODO: check if people getting stuck in ground on saucer parts
	//			retrieval mission is due to 60FPS or MI cheats
	// 			look up DAH2 pcsx2/60FPS clipping issue
	//			may be due to disable auto-aim cheat
	//			most likely due to 300% overclock?
	//				seems like it happens less often when at 130%
	// TODO: disable cam during body snatch
	// 			and scan
	//			just disable while locked-on
	// TODO: look for pointer chain for playerBase
	//			use a value close to base to start

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// =0x2 when paused, map, hint displayed
	if (PS2_MEM_ReadUInt(DAH2_IS_BUSY))
		return;

	// =0x1 during conversation, in-game cutscene
	if (PS2_MEM_ReadUInt(DAH2_IS_BUSY2))
		return;

	float looksensitivity = (float)sensitivity / scale;

	if (!PS2_DAH2_DetectPlayer())
		return;
	
	if (PS2_MEM_ReadUInt(DAH2_IS_LOCKED_ON) == IS_LOCKED_ON_TRUE)
	{
		if (PS2_DAH2_DetectRightAnalogStickBase())
		{
			// PK
			uint32_t rightAnalogStickBase2 = PS2_MEM_ReadPointer(rightAnalogStickBase + DAH2_RIGHT_ANALOG_STICK_2_BASE_POINTER) + 0x70; // 1
			rightAnalogStickBase2 = PS2_MEM_ReadPointer(rightAnalogStickBase2);

			float maxMouse = 45.f; // arbitrary value
			if (abs(xmouse) > 0)
			{
				float dir = (xmouse < 0) ? -1 : 1;
				float stickXPos = (float)xmouse * (float)sensitivity;
				if (stickXPos > maxMouse)
					stickXPos = maxMouse * dir;
				stickXPos = (stickXPos / maxMouse) * 0.9999999;
				PS2_MEM_WriteFloat(rightAnalogStickBase + DAH2_RIGHT_ANALOG_STICK_X, stickXPos);
				PS2_MEM_WriteFloat(rightAnalogStickBase2 + DAH2_RIGHT_ANALOG_STICK_2_X, stickXPos);
				// PS2_MEM_WriteFloat(DAH2_RIGHT_ANALOG_STICK_X_STATIC_1, stickXPos);
				// PS2_MEM_WriteFloat(DAH2_RIGHT_ANALOG_STICK_X_STATIC_2, stickXPos);
			}
			if (abs(ymouse) > 0)
			{
				float dir = (ymouse < 0) ? -1 : 1;
				float stickYPos = (float)ymouse * (float)sensitivity;
				if (stickYPos > maxMouse)
					stickYPos = maxMouse * dir;
				stickYPos = (stickYPos / maxMouse) * 0.9999999;
				PS2_MEM_WriteFloat(rightAnalogStickBase + DAH2_RIGHT_ANALOG_STICK_Y, -stickYPos);
				PS2_MEM_WriteFloat(rightAnalogStickBase2 + DAH2_RIGHT_ANALOG_STICK_2_Y, -stickYPos);
				// PS2_MEM_WriteFloat(DAH2_RIGHT_ANALOG_STICK_Y_STATIC_1, stickYPos);
				// PS2_MEM_WriteFloat(DAH2_RIGHT_ANALOG_STICK_Y_STATIC_2, stickYPos);
			}
		}
	}
	else
	{
		// onFoot 
		camBase = PS2_MEM_ReadPointer(playerBase + DAH2_CAMBASE_POINTER);

		float camX = PS2_MEM_ReadFloat(camBase + DAH2_CAMX);
		float camY = PS2_MEM_ReadFloat(camBase + DAH2_CAMY);

		camX -= (float)xmouse * looksensitivity;
		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;

		camY = ClampFloat(camY, -1.047197342f, 0.3490658104f);

		PS2_MEM_WriteFloat(camBase + DAH2_CAMX, (float)camX);
		PS2_MEM_WriteFloat(camBase + DAH2_CAMY, (float)camY);
	}

	if (PS2_DAH_DetectSaucer())
	{
		// in saucer
		if (PS2_MEM_ReadUInt16(saucerBase + DAH2_IN_SAUCER))
		{
			float rotX = PS2_MEM_ReadFloat(saucerBase + DAH2_SAUCER_ROTX);
			rotX -= (float)xmouse * looksensitivity;
			PS2_MEM_WriteFloat(saucerBase + DAH2_SAUCER_ROTX, rotX);

			// if (PS2_DAH2_DetectSaucerPosY())
			// {
			// 	float posY = PS2_MEM_ReadFloat(saucerPosYBase + DAH2_SAUCER_POSY);
			// 	posY -= (float)(invertpitch ? -ymouse : ymouse) * (looksensitivity * 20.f);
			// 	PS2_MEM_WriteFloat(saucerPosYBase + DAH2_SAUCER_POSY, posY);
			// 	// PS2_MEM_WriteFloat(saucerPosYBase + DAH2_SAUCER_POSY2, posY);
			// 	// PS2_MEM_WriteFloat(saucerPosYBase + DAH2_SAUCER_POSY3, posY);
			// 	// PS2_MEM_WriteFloat(saucerPosYBase + DAH2_SAUCER_POSY4, posY);
			// }
		}
	}

}