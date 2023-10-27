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

#define MDK_ROTX 0xBE020
#define MDK_AIM_CAMY 0xC43AC
#define MDK_FOV 0xC43B0
#define MDK_IS_NOT_AIMING 0x1FFDD8
#define MDK_IS_BOMBING 0xB8874
#define MDK_BOMBINGX 0xBDDA8
#define MDK_BOMBINGY 0xBDDAC

static uint8_t PS1_MDK_Status(void);
static void PS1_MDK_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"MDK",
	PS1_MDK_Status,
	PS1_MDK_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_MDK = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static float bombXAccumulator = 0.f;
static float bombYAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_MDK_Status(void)
{
	// SLUS_004.26
	return (PS1_MEM_ReadWord(0x92A4) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x92A8) == 0x5F303034U && 
			PS1_MEM_ReadWord(0x92AC) == 0x2E32363BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_MDK_Inject(void)
{
	// TODO: disable during
	//			pause
	//			loading
	
	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 10000.f;

	if (!PS1_MEM_ReadHalfword(MDK_IS_NOT_AIMING))
	{
		// First person aiming
		const float fov = ((float)PS1_MEM_ReadInt(MDK_FOV)) / 65536.f;

		int32_t rotX = PS1_MEM_ReadInt(MDK_ROTX);
		float rotXF = (float)rotX;

		float dx = -(float)xmouse * looksensitivity * scale * fov;
		AccumulateAddRemainder(&rotXF, &xAccumulator, -xmouse, dx);
		PS1_MEM_WriteInt(MDK_ROTX, (int32_t)rotXF);

		int32_t camY = PS1_MEM_ReadInt(MDK_AIM_CAMY);
		float camYF = (float)camY;
		float ym = (float)(invertpitch ? -ymouse : ymouse);
		float dy = ym * looksensitivity * scale * fov;
		AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);
		camYF = ClampFloat(camYF, -3276800.f, 3276800.f);
		PS1_MEM_WriteInt(MDK_AIM_CAMY, (int32_t)camYF);
	}
	else if(PS1_MEM_ReadInt16(MDK_IS_BOMBING))
	{
		// bombing mini-game
		int32_t bombX = PS1_MEM_ReadInt(MDK_BOMBINGX);
		int32_t bombY = PS1_MEM_ReadInt(MDK_BOMBINGY);
		float bombXF = (float)bombX;
		float bombYF = (float)bombY;

		float dx = (float)xmouse * looksensitivity * scale;
		AccumulateAddRemainder(&bombXF, &xAccumulator, xmouse, dx);
		bombXF = ClampFloat(bombXF, 4194304.f, 16777216.f);
		PS1_MEM_WriteInt(MDK_BOMBINGX, (int32_t)bombXF);

		float ym = (float)(invertpitch ? -ymouse : ymouse);
		float dy = ym * looksensitivity * scale;
		AccumulateAddRemainder(&bombYF, &yAccumulator, ym, dy);
		bombYF = ClampFloat(bombYF, 4194304.f, 11534336.f);
		PS1_MEM_WriteInt(MDK_BOMBINGY, (int32_t)bombYF);
	}
	else
	{
		// normal onFoot
		int32_t rotX = PS1_MEM_ReadInt(MDK_ROTX);
		float rotXF = (float)rotX;

		float dx = -(float)xmouse * looksensitivity * scale;
		AccumulateAddRemainder(&rotXF, &xAccumulator, -xmouse, dx);
		PS1_MEM_WriteInt(MDK_ROTX, (int32_t)rotXF);
	}

}