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
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "main.h"
#include "memory.h"
#include "mouse.h"
#include "./games/game.h"

enum EDITINGCURRENT {EDITINGSENSITIVITY = 0, EDITINGCROSSHAIR};

static uint8_t mousetoggle = 0;
static uint8_t selectedoption = EDITINGSENSITIVITY;
static uint8_t locksettings = 0;
static uint8_t welcomed = 0;

uint8_t sensitivity = 20;
uint8_t crosshair = 3;
uint8_t invertpitch = 0;
int isHooked = 0;
uint8_t uncapTickrate = 0;
uint8_t optionToggle = 0;

float out = 0;
float out2 = 0;
float out3 = 0;
float preSinOut = 0;
float preCosOut = 0;
float totalAngleOut = 0;
uint32_t uIntOut1 = 0x0;
uint32_t uIntOut2 = 0x0;
char titleOut[256];
uint64_t emuoffsetOut = 0;

int32_t main(void);
static void quit(void);
static void GUI_TitleShowHookStatus(int hooked);
static void GUI_Init(void);
static void GUI_Welcome(void);
static void GUI_Interact(void);
static void GUI_Update(void);
static void GUI_ListGames(void);
static void GUI_Clear(void);
static void INI_Load(void);
static void INI_Save(const uint8_t showerror);

//==========================================================================
// Purpose: run everything like a god damn speed demon
//==========================================================================
int32_t main(void)
{
	timeBeginPeriod(1);

	// TODO: Warn if multiple valid emulators are running
	// Hide mouse when hooked to emulator and supported game is running
	GUI_Init();
	if(!MEM_Init()) // close if dolphin or duckstation was not detected
	{
		printf("\n Mouse Injector for %s %s\n%s\n\n   Supported emulator not detected. Closing...", DOLPHINVERSION, BUILDINFO, LINE);
		Sleep(3000);
		return 0;
	}
	if(!MOUSE_Init()) // close if mouse was not detected
	{
		printf("\n Mouse Injector for %s %s\n%s\n\n   Mouse not detected. Closing...", DOLPHINVERSION, BUILDINFO, LINE);
		MEM_Quit();
		Sleep(3000);
		return 0;
	}
	INI_Load(); // load settings
	if(!welcomed) // if not welcomed before
		GUI_Welcome(); // show welcome message - wait for user input before continuing
	GUI_Update(); // update screen with options
	atexit(quit); // set function to run when program is closed
	int initialHookOccurred = 0;
	while(1)
	{
		GUI_Interact(); // check hotkey input
		int hooked = 0;
		if(mousetoggle)
		{
			if(GAME_Status()) // if supported game has been detected
			{
				MOUSE_Update(GAME_Tickrate()); // update xmouse and ymouse vars so injection use latest mouse input
				GAME_Inject(); // inject mouselook to game
				if (initialHookOccurred == 0) // update GUI on initial hook
				{
					GUI_Update();
					initialHookOccurred = 1;
				}
				hooked = 1;
			}
			else // dolphin has no game loaded or game not found, wait 100 ms and try again
			{
				hooked = 0;
				initialHookOccurred = 0;
				MEM_FindRamOffset();
				Sleep(100);
			}

			Sleep(GAME_Tickrate());
		}
		else 
		{
			Sleep(100);
		}

		GUI_TitleShowHookStatus(hooked);

		// update GUI for debug output
		// GUI_Update();
	}

	timeEndPeriod(1);
	return 0;
}
//==========================================================================
// Purpose: run when quit
//==========================================================================
static void quit(void)
{
	INI_Save(0);
	MOUSE_Quit();
	MEM_Quit();
}
//==========================================================================
// Purpose: display in the title whether an emulator is hooked and
//			supported game is running
//==========================================================================
static void GUI_TitleShowHookStatus(int hooked)
{
	if (hooked == isHooked)
		return;
	if (hooked)
		SetConsoleTitle("Mouse Injector | Hooked");
	else
		SetConsoleTitle("Mouse Injector | Unhooked");
	isHooked = hooked;
}
//==========================================================================
// Purpose: initialize console
//==========================================================================
static void GUI_Init(void)
{
	SetConsoleTitle("Mouse Injector");
	system("mode 80, 27"); // set window height and width
}
//==========================================================================
// Purpose: prints the welcome message
// Changed Globals: welcomed
//==========================================================================
static void GUI_Welcome(void)
{
	printf("\n    Mouse Injector for %s %s\n%s\n\n   Addendum - Please Read before Use\n\n\n", DOLPHINVERSION, BUILDINFO, LINE);
	printf("    1)  This is a unfinished alpha - expect issues and crashes\n\n");
	printf("    2)  All sub-systems are unsupported - use arrow keys for sentries/cameras\n\n");
	printf("    3)  NetPlay is unsupported - it will not synchronize memory injection\n\n");
	printf("    4)  Press insert in the main menu to list supported games (NTSC only)\n\n");
	printf("    5)  Read readme.txt for a quick start guide - thank you and enjoy\n\n\n");
	printf("   Press CTRL+1 to confirm you've read this message...\n%s\n", LINE);
	while(!welcomed)
	{
		if(K_CTRL1) // if user pressed CTRL+1
			welcomed = 1;
		Sleep(250);
	}
}
//==========================================================================
// Purpose: checks keyboard and will update internal values
// Changed Globals: mousetoggle, selectedoption, invertpitch, sensitivity, crosshair, locksettings
//==========================================================================
static void GUI_Interact(void)
{
	uint8_t updateinterface = 0, updatequick = 0;
	if(K_4) // mouse toggle (4)
	{
		MOUSE_Lock();
		MOUSE_Update(GAME_Tickrate());
		mousetoggle = !mousetoggle;
		updateinterface = 1;
	}
	if(K_5 && !locksettings && !updateinterface) // mouse sensitivity (5)
	{
		selectedoption = EDITINGSENSITIVITY;
		updateinterface = 1;
	}
	if(K_6 && !locksettings && !updateinterface && GAME_CrosshairSwaySupported()) // crosshair sensitivity (6)
	{
		selectedoption = EDITINGCROSSHAIR;
		updateinterface = 1;
	}
	if(K_7 && !locksettings && !updateinterface) // invert pitch toggle (7)
	{
		invertpitch = !invertpitch;
		updateinterface = 1;
	}
	if(K_8 && !locksettings && !updateinterface) // invert pitch toggle (7)
	{
		optionToggle = !optionToggle;
		updateinterface = 1;
	}
	if(K_PLUS && !locksettings && !updateinterface) // numpad plus (+)
	{
		if(selectedoption == EDITINGSENSITIVITY && sensitivity < 200)
			sensitivity++, updateinterface = 1;
		if(selectedoption == EDITINGCROSSHAIR && crosshair < 18 && GAME_CrosshairSwaySupported())
			crosshair++, updateinterface = 1;
		updatequick = 1;
	}
	if(K_MINUS && !locksettings && !updateinterface) // numpad minus (-)
	{
		if(selectedoption == EDITINGSENSITIVITY && sensitivity > 1)
			sensitivity--, updateinterface = 1;
		if(selectedoption == EDITINGCROSSHAIR && crosshair > 0 && GAME_CrosshairSwaySupported())
			crosshair--, updateinterface = 1;
		updatequick = 1;
	}
	if(K_INSERT && !locksettings && !updateinterface && !mousetoggle) // show list of supported games (INSERT)
	{
		GUI_ListGames();
		Sleep(10 * 1000); // wait 10 seconds
		K_INSERT; // flush input
		updateinterface = 1;
		updatequick = 1;
	}
	if(K_CTRL0 && !updateinterface) // hide/show settings (CTRL+0)
	{
		locksettings = !locksettings;
		updateinterface = 1;
	}
	if(updateinterface)
	{
		GUI_Update();
		Sleep(updatequick ? 100 : 200);
	}
}
//==========================================================================
// Purpose: update interface
//==========================================================================
static void GUI_Update(void)
{
	GUI_Clear();
	GAME_Status(); // refresh driver for title
	// printf("\n Mouse Injector for %s %s - %s\n", GAME_Name(), BUILDINFO, hookedEmulatorName); // title
	printf("\n Mouse Injector for %s - %s\n", hookedEmulatorName, GAME_Name()); // title
	printf("%s\n\n   Main Menu - Press [#] to Use Menu\n\n\n", LINE);
	printf(mousetoggle ? "   [4] - [ON] Mouse Injection\n\n" : "   [4] - [OFF] Mouse Injection\n\n");
	if(!locksettings)
	{
		printf("   [5] - Mouse Sensitivity: %d%%", sensitivity * 5);
		printf(selectedoption == EDITINGSENSITIVITY ? " [+/-]\n\n" : "\n\n");
		printf("   [6] - Crosshair Sway: ");
		if(GAME_CrosshairSwaySupported())
			printf(crosshair ? "%d%%" : "Locked", crosshair * 100 / 6);
		else
			printf("Not Available For Game");
		printf(selectedoption == EDITINGCROSSHAIR ? " [+/-]\n\n" : "\n\n");
		printf(invertpitch ? "   [7] - [ON] Invert Pitch\n\n" : "   [7] - [OFF] Invert Pitch\n\n");
		if (GAME_OptionSupported())
			printf("   [8] - %s\n\n", GAME_OptionMessage());
		printf("\n\n\n\n\n");
		printf("   [CTRL+0] - Lock Settings\n\n");
	}
	else
	{
		printf("\n\n\n\n\n\n\n\n\n\n\n");
		printf("   [CTRL+0] - Unlock Settings\n\n");
	}
	if(mousetoggle || locksettings)
		printf(" Note: [+/-] to Change Values\n%s\n", LINE);
	else
		printf(" Note: [+/-] to Change Values - [Insert] for Supported Games\n%s\n", LINE);
	
	// printf("tankBase: %X\n", uIntOut1);
	// printf("turretbase: %X\n", uIntOut2);
	// printf("sin: %f\n", out2);
	// printf("pre cos: %f\n", preCosOut);
	// printf("cos: %f\n", out3);
	// printf("onFootTotalAngle: %f", totalAngleOut);
	// printf("emuoffset: %llX", emuoffsetOut);
	// printf("words?: %s", titleOut);
}
//==========================================================================
// Purpose: print list of supported games
//==========================================================================
// Driver quality report
// TS2: camera/sentry interfaces not supported (hardcoded per level setup)
// TS3: all non-first person interfaces are not supported (many are hardcoded)
// NF: vehicle interface is semi-functional (fudged min/max) - last level is broken
// MOHF: works except for minecart level (level hardcode that breaks turret support)
// MOHER: fully supported
// MOHRS: looking down scope while in turret interface is broken (needs assembly hack)
// CODBRO: fully supported
// DHV: aiming sometimes breaks, sentry interface not supported
// SERIOUS: vehicle/submarine interfaces not supported
//==========================================================================
static void GUI_ListGames(void)
{
	GUI_Clear();
	printf("\n Supported Games (NTSC Only)\t\tGame IDs     Mouse Support\n%s\n\n", LINE);
	printf("    TimeSplitters 2\t\t\t GTSE4F\t\t Fair\n\n");
	printf("    TimeSplitters: Future Perfect\t G3FE69\t\t Poor\n\n");
	printf("    007: NightFire\t\t\t GO7E69\t\t Poor\n\n");
	printf("    Medal of Honor: Frontline\t\t GMFE69\t\t Fair\n\n");
	printf("    Medal of Honor: European Assault\t GONE69\t\t Good\n\n");
	printf("    Medal of Honor: Rising Sun\t\t GR8E69\t\t Poor\n\n");
	printf("    Call of Duty 2: Big Red One\t\t GQCE52\t\t Good\n\n");
	printf("    Die Hard: Vendetta\t\t\t GDIE7D\t\t Fair\n\n");
	printf("    Serious Sam: Next Encounter\t\t G3BE9G\t\t Fair\n\n");
	printf("    Trigger Man\t\t\t\t GG2E4Z\t\t Good\n\n");
	printf("   Returning to Main Menu in 10 Seconds...\n%s\n", LINE);
}
//==========================================================================
// Purpose: clear screen without using system("cls")
//==========================================================================
static void GUI_Clear(void)
{
	DWORD n; // number of characters written
	DWORD size; // number of visible characters
	COORD coord = {0}; // top left screen position
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE consolehandle = GetStdHandle(STD_OUTPUT_HANDLE); // get a handle to the console
	GetConsoleScreenBufferInfo(consolehandle, &csbi);
	size = csbi.dwSize.X * csbi.dwSize.Y; // find the number of characters to overwrite
	FillConsoleOutputCharacter(consolehandle, TEXT(' '), size, coord, &n); // overwrite the screen buffer with whitespace
	GetConsoleScreenBufferInfo(consolehandle, &csbi);
	FillConsoleOutputAttribute(consolehandle, csbi.wAttributes, size, coord, &n);
	SetConsoleCursorPosition(consolehandle, coord); // reset the cursor to the top left position
}
//==========================================================================
// Purpose: loads settings stored in mouseinjector.ini
// Changes Globals: sensitivity, crosshair, invertpitch, locksettings, welcomed
//==========================================================================
static void INI_Load(void)
{
	FILE *fileptr; // create a file pointer and open mouseinjector.ini from same dir as our program
	if((fileptr = fopen("mouseinjector.ini", "r")) != NULL) // if the INI exists
	{
		char line[128][128]; // char array used for file to write to
		char lines[128]; // maximum lines read size
		uint8_t counter = 0; // used to assign each line to a array
		while(fgets(lines, sizeof(lines), fileptr) != NULL && counter < 11) // read the first 10 lines
		{
			strcpy(line[counter], lines); // read file lines and assign value to line array
			counter++; // add 1 to counter, so the next line can be read
		}
		fclose(fileptr); // close the file stream
		if(counter == 5) // check if mouseinjector.ini length is valid
		{
			sensitivity = ClampInt(atoi(line[0]), 1, 200);
			crosshair = ClampInt(atoi(line[1]), 0, 18);
			invertpitch = !(!atoi(line[2]));
			locksettings = !(!atoi(line[3]));
			welcomed = !(!atoi(line[4]));
		}
		else
		{
			MessageBox(HWND_DESKTOP, "Loading mouseinjector.ini failed!\n\nUnknown values detected, loading default settings.", "Error", MB_ICONERROR | MB_OK); // tell the user loading mouseinjector.ini failed
			INI_Save(1); // overwrite mouseinjector.ini with valid settings
		}
	}
	else // if loading file failed
	{
		MessageBox(HWND_DESKTOP, "Loading mouseinjector.ini failed!\n\nAttempting to create mouseinjector.ini file.", "Error", MB_ICONERROR | MB_OK); // tell the user loading mouseinjector.ini failed
		INI_Save(1); // create mouseinjector.ini
	}
}
//==========================================================================
// Purpose: saves current settings to mouseinjector.ini
//==========================================================================
static void INI_Save(const uint8_t showerror)
{
	FILE *fileptr; // create a file pointer and open mouseinjector.ini from same dir as our program
	if((fileptr = fopen("mouseinjector.ini", "w")) != NULL) // if the INI exists
	{
		fprintf(fileptr, "%u\n%u\n%u\n%u\n%u", sensitivity, crosshair, invertpitch, locksettings, welcomed); // write current settings to mouseinjector.ini
		fclose(fileptr); // close the file stream
	}
	else if(showerror) // if saving file failed
		MessageBox(HWND_DESKTOP, "Saving mouseinjector.ini failed!", "Error", MB_ICONERROR | MB_OK); // tell the user saving mouseinjector.ini failed
}

// dx = change to value
void AccumulateAddRemainder(float *value, float *accumulator, float dir, float dx)
{
	if (dir == 0) return;

	// add integer part of dx
	if (dir < 0)
		*value += ceil(dx);
	else
		*value += floor(dx);
		// *value += (uint16_t)dx;

	// determine remainder
	float r = fmod(dx, 1.f);

	// if remainder + accumulation > 1, add 1
	if (abs(r + *accumulator) >= 1)
	{
		if (dir > 0)
			*value += 1;
		else
			*value -= 1;
	}

	// add remainder to accumlation
	*accumulator = fmod(r + *accumulator, 1.f);
}