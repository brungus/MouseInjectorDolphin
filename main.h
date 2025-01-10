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
#include <string.h>

#define DOLPHINVERSION "emulators"
#define BUILDINFO "(v0.31 - hotfix - "__DATE__")"
#define LINE "__________________________________________________________________"
// input for interface
#define K_1 GetAsyncKeyState(0x31) // key '1'
#define K_2 GetAsyncKeyState(0x32) // key '2'
#define K_3 GetAsyncKeyState(0x33) // key '3'
#define K_4 GetAsyncKeyState(0x34) // key '4'
#define K_5 GetAsyncKeyState(0x35) // key '5'
#define K_6 GetAsyncKeyState(0x36) // key '6'
#define K_7 GetAsyncKeyState(0x37) // key '7'
#define K_8 GetAsyncKeyState(0x38) // key '8'
#define K_CTRL0 (GetAsyncKeyState(0x11) && GetAsyncKeyState(0x30) || GetAsyncKeyState(0x30) && GetAsyncKeyState(0x11)) // key combo control + '0'
#define K_CTRL1 (GetAsyncKeyState(0x11) && GetAsyncKeyState(0x31) || GetAsyncKeyState(0x31) && GetAsyncKeyState(0x11)) // key combo control + '1'
#define K_PLUS (GetAsyncKeyState(0x6B) || GetAsyncKeyState(0xBB)) // key '+'
#define K_MINUS (GetAsyncKeyState(0x6D) || GetAsyncKeyState(0xBD)) // key '-'
#define K_INSERT GetAsyncKeyState(0x2D) // key 'Insert'
#if _MSC_VER && !__INTEL_COMPILER // here because some MSVC versions only support __inline :/
#define inline __inline
#endif

inline float ClampFloat(const float value, const float min, const float max)
{
	const float test = value < min ? min : value;
	return test > max ? max : test;
}

inline int32_t ClampInt(const int32_t value, const int32_t min, const int32_t max)
{
	const int32_t test = value < min ? min : value;
	return test > max ? max : test;
}

inline uint16_t ClampHalfword(const uint16_t value, const uint16_t min, const uint16_t max)
{
	const int16_t test = value < min ? min : value;
	return test > max ? max : test;
}

inline uint8_t FloatsEqual(const float f1, const float f2)
{
	const float epsilon = 0.0001;
	return (f1 - f2) < epsilon;
}

extern void AccumulateAddRemainder(float *value, float *accumulator, float dir, float dx);

extern uint8_t sensitivity;
extern uint8_t crosshair;
extern uint8_t invertpitch;
extern uint8_t optionToggle;
extern float out;
extern float out2;
extern float out3;
extern float preSinOut;
extern float preCosOut;
extern float totalAngleOut;
extern uint32_t uIntOut1;
extern uint32_t uIntOut2;
extern char titleOut[256];
extern uint64_t emuoffsetOut;