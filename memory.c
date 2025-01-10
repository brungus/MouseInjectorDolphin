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
#include <stdio.h> // for text file debug output
#include <stdint.h>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <tchar.h>
#include <inttypes.h> // hex conversion for memory debug
#include "main.h"
#include "memory.h"

static uint64_t emuoffset = 0;
static uint32_t aramoffset = 0x02000000; // REQUIRES that MMU is off
static HANDLE emuhandle = NULL;
static int isPS1handle = 0;
static int isN64handle = 0;
static int isMupenhandle = 0;
static int isBizHawkhandle = 0;
static int isBSNEShandle = 0;
static int isPcsx2handle = 0;
static int isRetroArchHandle = 0;
static int isKronosHandle = 0;
static int isBeetlePSXHandle = 0;
static int isFlycastHandle = 0;
static int isBizHawkSNESHandle = 0;
static int isBSNESMercuryHandle = 0;
static int isMesenHandle = 0;
static int isRPCS3Handle = 0;
static int isPPSSPPHandle = 0;
static int isBizHawkGenesisHandle = 0;
static int isBizHawkSaturnHandle = 0;
static int isBizHawkPlayStationHandle = 0;
static int isNOMONEYPSXHandle = 0;
static int isProject64Handle = 0;
char hookedEmulatorName[80];

uint8_t MEM_Init(void);
void MEM_Quit(void);
void MEM_UpdateEmuoffset(void);
int32_t MEM_ReadInt(const uint32_t addr);
uint32_t MEM_ReadUInt(const uint32_t addr);
uint16_t MEM_ReadUInt16(const uint32_t addr);
uint8_t MEM_ReadUInt8(const uint32_t addr);
float MEM_ReadFloat(const uint32_t addr);
void MEM_WriteInt(const uint32_t addr, int32_t value);
void MEM_WriteUInt(const uint32_t addr, uint32_t value);
void MEM_WriteFloat(const uint32_t addr, float value);
static void MEM_ByteSwap32(uint32_t *input);
static void MEM_ByteSwap64(uint64_t *input);

int32_t ARAM_ReadInt(const uint32_t addr);
uint32_t ARAM_ReadUInt(const uint32_t addr);
float ARAM_ReadFloat(const uint32_t addr);
void ARAM_WriteUInt(const uint32_t addr, uint32_t value);
void ARAM_WriteFloat(const uint32_t addr, float value);

uint32_t PS1_MEM_ReadPointer(const uint32_t addr);
uint32_t PS1_MEM_ReadWord(const uint32_t addr);
uint32_t PS1_MEM_ReadUInt(const uint32_t addr);
int32_t PS1_MEM_ReadInt(const uint32_t addr);
int16_t PS1_MEM_ReadInt16(const uint32_t addr);
uint16_t PS1_MEM_ReadHalfword(const uint32_t addr);
uint8_t PS1_MEM_ReadByte(const uint32_t addr);
void PS1_MEM_WriteInt(const uint32_t addr, int32_t value);
void PS1_MEM_WriteInt16(const uint32_t addr, int16_t value);
void PS1_MEM_WriteWord(const uint32_t addr, uint32_t value);
void PS1_MEM_WriteHalfword(const uint32_t addr, uint16_t value);
void PS1_MEM_WriteByte(const uint32_t addr, uint8_t value);
// static void MEM_ByteSwap16(uint16_t *input);

uint32_t N64_MEM_ReadUInt(const uint32_t addr);
int16_t N64_MEM_ReadInt16(const uint32_t addr);
float N64_MEM_ReadFloat(const uint32_t addr);
void N64_MEM_WriteFloat(const uint32_t addr, float value);
void N64_MEM_WriteUInt(const uint32_t addr, uint32_t value);
void N64_MEM_WriteInt16(const uint32_t addr, int16_t value);
void N64_MEM_WriteByte(const uint32_t addr, uint8_t value);

uint8_t SNES_MEM_ReadByte(const uint32_t addr);
uint16_t SNES_MEM_ReadWord(const uint32_t addr);
void SNES_MEM_WriteByte(const uint32_t addr, uint8_t value);
void SNES_MEM_WriteWord(const uint32_t addr, uint16_t value);

uint32_t PS2_MEM_ReadPointer(const uint32_t addr);
uint32_t PS2_MEM_ReadWord(const uint32_t addr);
uint32_t PS2_MEM_ReadUInt(const uint32_t addr);
uint32_t PS2_MEM_ReadUInt16(const uint32_t addr);
int16_t PS2_MEM_ReadInt16(const uint32_t addr);
uint8_t PS2_MEM_ReadUInt8(const uint32_t addr);
float PS2_MEM_ReadFloat(const uint32_t addr);
void PS2_MEM_WriteWord(const uint32_t addr, uint32_t value);
void PS2_MEM_WriteUInt(const uint32_t addr, uint32_t value);
void PS2_MEM_WriteUInt16(const uint32_t addr, uint16_t value);
void PS2_MEM_WriteInt16(const uint32_t addr, int16_t value);
void PS2_MEM_WriteFloat(const uint32_t addr, float value);
DWORD Process_ID = 0;
char PS2_EXE_Name[64];
FARPROC RemoteAddress(HANDLE hProc, HMODULE hMod, const char* procName);
HMODULE RemoteHandle(DWORD ProcessPID, const TCHAR* modName);

uint32_t SD_MEM_ReadWord(const uint32_t addr);
float SD_MEM_ReadFloat(const uint32_t addr);
void SD_MEM_WriteFloat(const uint32_t addr, float value);

uint32_t PS3_MEM_ReadUInt(const uint32_t addr);
float PS3_MEM_ReadFloat(const uint32_t addr);
uint32_t PS3_MEM_ReadPointer(const uint32_t addr);
void PS3_MEM_WriteFloat(const uint32_t addr, float value);

uint32_t PSP_MEM_ReadWord(const uint32_t addr);
uint32_t PSP_MEM_ReadPointer(const uint32_t addr);
uint32_t PSP_MEM_ReadUInt(const uint32_t addr);
uint16_t PSP_MEM_ReadUInt16(const uint32_t addr);
float PSP_MEM_ReadFloat(const uint32_t addr);
void PSP_MEM_WriteUInt16(const uint32_t addr, uint16_t value);
void PSP_MEM_WriteFloat(const uint32_t addr, float value);

void printdebug(uint64_t val);

//==========================================================================
// Purpose: initialize dolphin handle and setup for memory injection
// Changed Globals: emuhandle
//==========================================================================
uint8_t MEM_Init(void)
{
	emuhandle = NULL;
	HANDLE processes; // will store a snapshot of all processes
	PROCESSENTRY32 pe32; // stores basic info of a process, using this one to read the ProcessID from
	processes = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // make process snapshot
	pe32.dwSize = sizeof(PROCESSENTRY32); // correct size
	Process32First(processes, &pe32); // read info about the first process into pe32
	do // loop to find emulator
	{
		if(strcmp(pe32.szExeFile, "Dolphin.exe") == 0) // if dolphin was found
		{
			strcpy(hookedEmulatorName, "Dolphin");
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "duckstation-qt-x64-ReleaseLTCG.exe") == 0) // if DuckStation was found
		{
			strcpy(hookedEmulatorName, "DuckStation");
			isPS1handle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			Process_ID = pe32.th32ProcessID;
			break;
		}
		if(strcmp(pe32.szExeFile, "EmuHawk.exe") == 0) // if EmuHawk was found, 2.8 oldest tested working - 2.9 not supported
		{
			// strcpy(hookedEmulatorName, "BizHawk N64");
			strcpy(hookedEmulatorName, "BizHawk - No core loaded");
			// isN64handle = 1;
			isBizHawkhandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "RMG.exe") == 0) // if simple64 was found
		{
			strcpy(hookedEmulatorName, "Rosalie's Mupen GUI");
			isN64handle = 1;
			isMupenhandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "simple64-gui.exe") == 0) // if simple64 was found
		{
			strcpy(hookedEmulatorName, "simple64");
			isN64handle = 1;
			isMupenhandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "retroarch.exe") == 0) // if retroarch was found, for N64 games using Mupen64Plus-Next
		{
			strcpy(hookedEmulatorName, "RetroArch - No core loaded");
			isRetroArchHandle = 1;
			// isN64handle = 1;
			// isMupenhandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "bsnes.exe") == 0) // if BSNES was found
		{
			strcpy(hookedEmulatorName, "BSNES");
			isBSNEShandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		const char* pcsx2Executables[] = {"pcsx2-qtx64-avx2.exe","pcsx2-qtx64.exe","pcsx2-qt.exe"};
		size_t numExecutables = sizeof(pcsx2Executables) / sizeof(pcsx2Executables[0]);
		for (size_t i = 0; i < numExecutables; ++i) {
			if (strcmp(pe32.szExeFile, pcsx2Executables[i]) == 0) {
				strncpy(hookedEmulatorName, pcsx2Executables[i], strlen(pcsx2Executables[i]) - 4);
				hookedEmulatorName[strlen(pcsx2Executables[i]) - 4] = '\0';
			isPcsx2handle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			Process_ID = pe32.th32ProcessID;
			strncpy(PS2_EXE_Name, pe32.szExeFile, sizeof(PS2_EXE_Name) - 1);
			break;
			}
		}

		if(strcmp(pe32.szExeFile, "flycast.exe") == 0) 
		{
			strcpy(hookedEmulatorName, "Flycast");
			isFlycastHandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "rpcs3.exe") == 0) 
		{
			strcpy(hookedEmulatorName, "RPCS3");
			isRPCS3Handle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "PPSSPPWindows.exe") == 0 || strcmp(pe32.szExeFile, "PPSSPPWindows64.exe") == 0) 
		{
			strcpy(hookedEmulatorName, "PPSSPP");
			isPPSSPPHandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "NO$PSX.EXE") == 0) 
		{
			strcpy(hookedEmulatorName, "NO$PSX");
			isNOMONEYPSXHandle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
		if(strcmp(pe32.szExeFile, "Project64.exe") == 0) 
		{
			strcpy(hookedEmulatorName, "Project64");
			isProject64Handle = 1;
			emuhandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
			break;
		}
	}
	while(Process32Next(processes, &pe32)); // loop continued until Process32Next deliver NULL or its interrupted with the "break" above
	CloseHandle(processes);
	return (emuhandle != NULL);
}
//==========================================================================
// Purpose: close emuhandle safely
// Changed Globals: emuhandle
//==========================================================================
void MEM_Quit(void)
{
	if(emuhandle != NULL)
		CloseHandle(emuhandle);
}
//==========================================================================
// Purpose: update emuoffset pointer to location of gamecube memory
// Changed Globals: emuoffset
//==========================================================================
uint8_t MEM_FindRamOffset(void)
{
	emuoffset = 0;

	MEMORY_BASIC_INFORMATION info; // store a snapshot of memory information

	PVOID gamecube_ptr = NULL;

	uint32_t lastRegionSize = 0;
	uint32_t lastlastRegionSize = 0;

	//--------------------Duckstation RAM export pointer-----------------------
	if (isPS1handle == 1) {
		const TCHAR* processName = "duckstation-qt-x64-ReleaseLTCG.exe";
		const TCHAR* moduleName = processName;
		const char* symbol = "RAM";

		printf("Scanning memory.\n");
		HANDLE snapshot = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Process_ID);
		HMODULE hMod = RemoteHandle(Process_ID, moduleName);
		if (!hMod) {
			return;
		}
		FARPROC addr = RemoteAddress(snapshot, hMod, symbol);
		CloseHandle(snapshot);

		if (addr != 0) {
			uint64_t pointerAddress = addr;
			uint64_t foundValue;
			ReadProcessMemory(emuhandle, (LPCVOID)pointerAddress, &foundValue, sizeof(foundValue), NULL);
			emuoffset = foundValue;
			printf(".\n");
			printf("..\n");
			printf("DEBUG - DUCKSTATION MEMORY BASE FOUND AT: 0x%p\n",foundValue);
		}
	}
	//-----------------------------------------------------------------------
	//--------------------PCSX2 RAM export pointer---------------------------
	//Using EEmem address causes some sort out of sync value updating in some games, seems like a PCSX2 thing or something with the injector?? -> use of a pointer to a different copy of PS2 virtual memory (hotfixed below)
	if (isPcsx2handle == 1) {
		const TCHAR* processName = PS2_EXE_Name;
		const TCHAR* moduleName = processName;
		const char* symbol = "EEmem";
		int chunk_size = 4096;

		HANDLE snapshot = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Process_ID);
		HMODULE hMod = RemoteHandle(Process_ID, moduleName);
		if (!hMod) {
			return;
		}
		FARPROC addr = RemoteAddress(snapshot, hMod, symbol);
		CloseHandle(snapshot);
		//search from EEmem SLUS string offset to different copy of virtual PS2 RAM
		uint64_t PS2_Address = addr;
		if (addr != 0) {
			uint64_t pointerAddress = addr;
			uint64_t EEmem;
			uint64_t AddressCopy;
			uint64_t OtherCopyBase;
			BYTE* buffer;
			SIZE_T bytesRead;
			LPCVOID address = 0;

			ReadProcessMemory(emuhandle, (LPCVOID)pointerAddress, &EEmem, sizeof(EEmem), NULL); //EEmem is now set > offset this by 0x100000 > load up a chunk of data > loop to find ram copy that does not cause issues with injector, substract the 0x93390 and set emuoffset
			
			unsigned char* chunk = (unsigned char*)malloc(chunk_size);
			DWORD_PTR offset = EEmem + 0x100000; //use an offset where the gamecode is, not 93390 that is determined by BIOS and not every games uses this offset
			LPCVOID remotePtr = (LPCVOID)offset;
			ReadProcessMemory(emuhandle, remotePtr, chunk, chunk_size, &bytesRead); // array gets loaded up correctly

			printf("Scanning memory.\n");
			while (VirtualQueryEx(emuhandle, address, &info, sizeof(info)) && info.RegionSize > 0) { //memory scan loop
				if (info.State == MEM_COMMIT && (info.Protect == PAGE_READONLY || info.Protect == PAGE_READWRITE)) {
					buffer = (BYTE*)malloc(info.RegionSize);
					if (buffer && ReadProcessMemory(emuhandle, info.BaseAddress, buffer, info.RegionSize, &bytesRead)) {
						for (SIZE_T i = 0; i <= bytesRead - chunk_size; i++) {
							if (memcmp(buffer + i, chunk, chunk_size) == 0) { // if found match, exit, break out of the nested loop on the first match, will find EEmem if it wont find other memory
								AddressCopy = (uint64_t*)((BYTE*)info.BaseAddress + i);
								goto breakout;
							}
						}
					}
				}
				address = (BYTE*)info.BaseAddress + info.RegionSize;
			}

			breakout:
			free(chunk);// free the memory
			free(buffer);

			OtherCopyBase = AddressCopy - 0x100000;
			printf(".\n");
			printf("..\n");
			printf("...\n");
			printf("DEBUG - PCSX2 EEMEM BASE FOUND AT: 0x%p\n",EEmem);
			printf("DEBUG - PCSX2 MEMORY BASE FOUND AT: 0x%p\n",OtherCopyBase);
			emuoffset = OtherCopyBase;

			// TODO: check the hotfix, clean up the code etc, remove the ridiculous printfs and code in normal debug page, and maybe some simple ui
		}
	}
	//------------------------------------------------------------------------

	// if (strcmp(hookedEmulatorName, "DuckStation") == 0)
	// {
	// 	uint64_t output; // temp var
	// 	// UINT_PTR addr = 0x7FF6A3B80000;

	// 	HMODULE hMods[1024];
	// 	DWORD cbNeeded;
	// 	unsigned int i;
	// 	UINT_PTR addr;

	// 	if (EnumProcessModules(emuhandle, hMods, sizeof(hMods), &cbNeeded))
	// 	{
	// 		for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
	// 		{
	// 			TCHAR szModName[MAX_PATH];

	// 			if (GetModuleBaseName(emuhandle, hMods[i], szModName,
	// 									sizeof(szModName) / sizeof(TCHAR)))
	// 			{
	// 				if (strcmp(szModName, "duckstation-qt-x64-ReleaseLTCG.exe") == 0)
	// 				{
	// 					MODULEINFO modinfo;

	// 					if (GetModuleInformation(emuhandle, hMods[i], &modinfo, sizeof(modinfo)))
	// 					{
	// 						memcpy(&addr, &(modinfo.lpBaseOfDll), sizeof(modinfo.lpBaseOfDll));
	// 					}
	// 				}
	// 			}
	// 		}
	// 	}

	// 	// UINT_PTR addr = (UINT_PTR)GetModuleHandle("duckstation-qt-x64-ReleaseLTCG.exe");
	// 	uint64_t offset = 0x7E16C0;
	// 	// uint64_t offset = 0x84A720;
	// 	ReadProcessMemory(emuhandle, (LPVOID)(addr + offset), &output, sizeof(output), NULL);
	// 	emuoffset = output;
	// 	return (emuoffset != 0x0);
	// }

	while (VirtualQueryEx(emuhandle, gamecube_ptr, &info, sizeof(info))) // loop continues until we reach the last possible memory region
	{
		gamecube_ptr = info.BaseAddress + info.RegionSize; // update address to next region of memory for loop

		// set region size to look for based on the emulator detected
		uint32_t emuRegionSize = 0x2000000; // Dolphin
		if (isBizHawkhandle == 1) {
			char bizHawkTitle[255];
			HWND foreground = GetForegroundWindow();
			int a = GetWindowText(foreground, bizHawkTitle, 256);

			// check window title for loaded core
			if (strstr(bizHawkTitle, "SNES") != NULL) {
				strcpy(hookedEmulatorName, "BizHawk SNES");
				// SNES region size is not static but region before is? (size: 0x19E000?)
				emuRegionSize = 0x1E000;
				// emuRegionSize = 0x19000;
				// emuRegionSize = 0x13000;
				isBizHawkSNESHandle = 1;
			}
			else if (strstr(bizHawkTitle, "Nintendo 64") != NULL) {
				strcpy(hookedEmulatorName, "BizHawk N64");
				emuRegionSize = 0x22D0000; 	// BizHawk 2.8 (Mupen64Plus)
				isN64handle = 1;
			}
			else if (strstr(bizHawkTitle, "Genesis") != NULL) {
				strcpy(hookedEmulatorName, "BizHawk Genesis");
				emuRegionSize = 0xE3000; 	// BizHawk 2.8 
				isBizHawkGenesisHandle = 1;
			}
			else if (strstr(bizHawkTitle, "Saturn") != NULL) {
				strcpy(hookedEmulatorName, "BizHawk Saturn");
				emuRegionSize = 0x282000; 	// BizHawk 2.8 
				isBizHawkSaturnHandle = 1;
			}
			else if (strstr(bizHawkTitle, "PlayStation") != NULL) {
				strcpy(hookedEmulatorName, "BizHawk PlayStation");
				emuRegionSize = 0xD902000; 	// BizHawk 2.8 
				isBizHawkPlayStationHandle = 1;
			}
		}
		else if (isRetroArchHandle == 1) {
			char retroArchTitle[255];
			HWND foreground = GetForegroundWindow();
			int a = GetWindowText(foreground, retroArchTitle, 256);

			// check window title for loaded core
			if (strstr(retroArchTitle, "Mupen") != NULL) {
				strcpy(hookedEmulatorName, "RetroArch Mupen64Plus-Next");
				emuRegionSize = 0x20011000; // Mupen64Plus core
				isN64handle = 1;
				isMupenhandle = 1;
			}
			else if (strstr(retroArchTitle, "Kronos") != NULL) {
				strcpy(hookedEmulatorName, "RetroArch Kronos");
				emuRegionSize = 0x101000; // Kronos core
				isKronosHandle = 1;
			}
			else if (strstr(retroArchTitle, "Beetle PSX HW") != NULL) {
				strcpy(hookedEmulatorName, "RetroArch Beetle PSX HW");
				emuRegionSize = 0x200000;
			}
			else if (strstr(retroArchTitle, "Beetle PSX") != NULL) {
				strcpy(hookedEmulatorName, "RetroArch Beetle PSX");
				emuRegionSize = 0x200000;
			}
			else if (strstr(retroArchTitle, "PCSX-ReARMed") != NULL) {
				strcpy(hookedEmulatorName, "RetroArch PCSX-ReARMed");
				emuRegionSize = 0x210000;
			}
			else if (strstr(retroArchTitle, "DuckStation") != NULL) {
				strcpy(hookedEmulatorName, "RetroArch DuckStation");
				emuRegionSize = 0x200000;
			}
			else if (strstr(retroArchTitle, "SwanStation") != NULL) {
				strcpy(hookedEmulatorName, "RetroArch SwanStation");
				emuRegionSize = 0x200000;
			}
			else if (strstr(retroArchTitle, "Flycast") != NULL) {
				strcpy(hookedEmulatorName, "RetroArch Flycast");
				emuRegionSize = 0x10000;
				isFlycastHandle = 1;
			}
			else if (strstr(retroArchTitle, "bsnes-mercury") != NULL) {
				strcpy(hookedEmulatorName, "RetroArch bsnes-mercury");
				emuRegionSize = 0x39000;
				isBSNESMercuryHandle = 1;
			}
		}
		/*else if (isPS1handle == 1) {
			// emuRegionSize = 0x200000; 		// DuckStation
			emuRegionSize = 0x800000;		// DuckStation, newer version 0.1-5943 (unsure which version changed region size)
			// TODO: check version and set region size
			//		 maybe look for a pointer to the right region and get size from that?
		}*/ else if (isN64handle == 1) {
			if (isMupenhandle)
				emuRegionSize = 0x20011000; // RetroArch(Mupen64Plus core)/simple64/RMG
			else
				emuRegionSize = 0x22D0000; 	// BizHawk 2.8 (Mupen64Plus)
		} else if (isBSNEShandle == 1) {
			emuRegionSize = 0x34000;
		} /*else if (isPcsx2handle == 1) {
			// emuRegionSize = 0x80000;
			emuRegionSize = 0x1000;
		}*/ else if (isFlycastHandle == 1) {
			emuRegionSize = 0x10000;
		} else if (isMesenHandle == 1) {
			emuRegionSize = 0x1BF000;
		} else if (isRPCS3Handle == 1) {
			// TODO: if rpcs3 just set offset to 0x330000000 since it seems to be static?
			emuRegionSize = 0xCC00000; // Killzone HD
			// emuRegionSize = 0x100000; // HAZE
		} else if (isPPSSPPHandle == 1) {
			emuRegionSize = 0x1F00000;
		} else if (isNOMONEYPSXHandle == 1) {
			emuRegionSize = 0x459000;
		} else if (isProject64Handle == 1) {
			// emuRegionSize = 0x5BDED000;
			emuRegionSize = 0x800000;
		}

		// PCSX2: MEM_MAPPED
		// PPSSPP: MEM_MAPPED
		DWORD regionType = MEM_MAPPED; // Dolphin and DuckStation regions are type MEM_MAPPED
		if (isN64handle == 1 || isKronosHandle == 1)
			regionType = MEM_PRIVATE;  // All N64 emulator regions are type MEM_PRIVATE
		if (isBSNEShandle == 1 || isBSNESMercuryHandle == 1)
			regionType = MEM_IMAGE;
		if (isNOMONEYPSXHandle == 1)
		{
			// regionType = MEM_IMAGE;
			regionType = MEM_PRIVATE;
			// regionType = MEM_MAPPED;
		}
		if (isProject64Handle == 1)
			regionType = MEM_PRIVATE;


		// if (isBSNEShandle == 1)
		// {
		// 	// if BSNES, just look until you find region at 0xB140000
		// 	PSAPI_WORKING_SET_EX_INFORMATION wsinfo;
		// 	wsinfo.VirtualAddress = info.BaseAddress;

		// 	if (info.BaseAddress == 0xB14000)
		// 	{
		// 		if (QueryWorkingSetEx(emuhandle, &wsinfo, sizeof(wsinfo))) { // query extended info about the memory page at the current virtual address space
		// 			if (wsinfo.VirtualAttributes.Valid) { // check if the address space is valid
		// 				memcpy(&emuoffset, &(info.BaseAddress), sizeof(info.BaseAddress)); // copy the base address location to our emuoffset pointer
		// 				emuoffset += 0x2D7C; // WRAM always here? 0xB14000 + 0x2D7C, but region size is not fixed

		// 				return (emuoffset != 0x0);
		// 			}
		// 		}
		// 	}
		// }

		// check if region is the size of region where console memory is located
		uint8_t regionFound = 0;
		if (info.RegionSize == emuRegionSize && ((info.Type == regionType) || isN64handle))
			regionFound = 1;
		
		// if (isBizHawkSNESHandle && !regionFound)
		// {
		// 	if (info.Type == regionType)
		// 	{
		// 		if (info.RegionSize >= (uint32_t)0x11000 && info.RegionSize <= (uint32_t)0x20000)
		// 		{
		// 			// handle the other possible region size
		// 			emuRegionSize = info.RegionSize;
		// 			regionFound = 1;
		// 		}
		// 	}
		// }

		if (regionFound == 1) { // why '|| isN64handle'? one N64 emulator is not MEM_PRIVATE?
		// if (info.RegionSize == emuRegionSize) {
			// printdebug(info.Type); // debug
			PSAPI_WORKING_SET_EX_INFORMATION wsinfo;
			wsinfo.VirtualAddress = info.BaseAddress;

			if (QueryWorkingSetEx(emuhandle, &wsinfo, sizeof(wsinfo))) { // query extended info about the memory page at the current virtual address space
				if (wsinfo.VirtualAttributes.Valid) { // check if the address space is valid
					memcpy(&emuoffset, &(info.BaseAddress), sizeof(info.BaseAddress)); // copy the base address location to our emuoffset pointer

					// output region start
					// emuoffsetOut = emuoffset;

					if (isBSNESMercuryHandle == 1) {
						if (lastRegionSize != 0xB7000)
							continue;
						emuoffset += 0x7F1C;
					}
					else if (isProject64Handle == 1) {
						if (lastRegionSize != 0xE000)
							continue;
					}
					else if (isRPCS3Handle == 1) {
						if (lastRegionSize != 0xFF70000)
							continue;
					}
					else if (isNOMONEYPSXHandle == 1) {
						emuoffset += 0x30100;
					}
					else if (isBizHawkPlayStationHandle == 1) {
						emuoffset += 0x4FEAE0;
					}
					else if (isBizHawkSaturnHandle == 1) {
						emuoffset += 0x81D60;
					}
					else if (isBizHawkSNESHandle == 1) {
						emuoffset += 0x40;
					}
					else if (isKronosHandle == 1){
						emuoffset += 0x40;
					}
					else if (isFlycastHandle == 1) {
						if (lastRegionSize != 0x2000000) // region before is always size 0x2000000?
							continue;
					}
					else if (isN64handle == 1)
					{
						if (isMupenhandle) { // RetroArch/simple64/RMG (mupen64plus GUIs)
							// simple64/RMG/retroarch: determine buffer size before RDRAM as it changes every startup
							emuoffset += 0x1000;
							// while (PS1_MEM_ReadWord(0x0) == 0) // look for non-zero bytes at first address to signify the start of RDRAM
							while (MEM_ReadUInt(0x80000000) == 0) // look for non-zero bytes at first address to signify the start of RDRAM
								emuoffset += 0x1000; // RDRAM always begins at a multiple 0x1000 away from the initial emuhandle offset
						}
						else {
							// BizHawk has small offset before N64 RDRAM
							emuoffset += 0x8E0;
						}
					} else if (isBSNEShandle == 1) {
						emuoffset += 0x2D7C; // WRAM always here? 0xB14000 + 0x2D7C, but region size is not fixed
					} /*else if (isPcsx2handle == 1) {
						// check if region before 0x80000 has a size in a range, 0x1000 <= regionsize <= 0xF000
						// if (lastRegionSize != 0x80000 || lastlastRegionSize != 0x1000)
						if (lastRegionSize != 0x80000 || lastlastRegionSize > 0xF000)
							continue;
					// } else if (isPPSSPPHandle) {
					// 	if (lastRegionSize != 0x3800000 && lastlastRegionSize != 0x200000)
					// 		continue;
					}*/

					// printdebug(lastRegionSize); // debug
					// printdebug(info.RegionSize); // debug
					// printdebug(&(info.BaseAddress)); // debug

					return (emuoffset != 0x0);
				}
			}
		}

		lastlastRegionSize = lastRegionSize;
		lastRegionSize = info.RegionSize;
	}
}
//==========================================================================
// Purpose: read int from memory
// Parameter: address location
//==========================================================================
int32_t MEM_ReadInt(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	int32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}
//==========================================================================
// Purpose: read unsigned int from memory
// Parameter: address location
//==========================================================================
uint32_t MEM_ReadUInt(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	MEM_ByteSwap32(&output); // byteswap
	return output;
}

// uint64_t MEM_ReadUInt64(const uint64_t addr)
// {
// 	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
// 		return 0;
// 	uint64_t output; // temp var used for output of function
// 	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
// 	MEM_ByteSwap64(&output); // byteswap
// 	return output;
// }

uint16_t MEM_ReadUInt16(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	uint16_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	// MEM_ByteSwap32(&output); // byteswap
	// TODO: needs byteswap to be proper
	return output;
}

uint8_t MEM_ReadUInt8(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	uint8_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	return output;
}
//==========================================================================
// Purpose: read float from memory
// Parameter: address location
//==========================================================================
float MEM_ReadFloat(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	float output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}
//==========================================================================
// Purpose: write int to memory
// Parameter: address location and value
//==========================================================================
void MEM_WriteInt(const uint32_t addr, int32_t value)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or writing to outside of memory range
		return;
	MEM_ByteSwap32((uint32_t *)&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &value, sizeof(value), NULL);
}
//==========================================================================
// Purpose: write unsigned int to memory
// Parameter: address location and value
//==========================================================================
void MEM_WriteUInt(const uint32_t addr, uint32_t value)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or writing to outside of memory range
		return;
	MEM_ByteSwap32(&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &value, sizeof(value), NULL);
}
//==========================================================================
// Purpose: write float to memory
// Parameter: address location and value
//==========================================================================
void MEM_WriteFloat(const uint32_t addr, float value)
{
	if(!emuoffset || NOTWITHINMEMRANGE(addr)) // if gamecube memory has not been init by dolphin or writing to outside of memory range
		return;
	MEM_ByteSwap32((uint32_t *)&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &value, sizeof(value), NULL);
}
//==========================================================================
// Purpose: byteswap input value
// Parameter: pointer of value (must be 4 byte long)
//==========================================================================
static void MEM_ByteSwap32(uint32_t *input)
{
	const uint8_t *inputarray = ((uint8_t *)input); // set byte array to input
	*input = (uint32_t)((inputarray[0] << 24) | (inputarray[1] << 16) | (inputarray[2] << 8) | (inputarray[3])); // reassign input to swapped value
}

static void MEM_ByteSwap64(uint64_t *input)
{
	const uint8_t *inputarray = ((uint8_t *)input); // set byte array to input
	// *input = (uint64_t)((inputarray[0] << 24) | (inputarray[1] << 16) | (inputarray[2] << 8) | (inputarray[3])); // reassign input to swapped value
	*input = (uint64_t)((inputarray[0] << 56) | (inputarray[1] << 48) | (inputarray[2] << 40) | (inputarray[3] << 32) | (inputarray[4] << 24) | (inputarray[5] << 16) | (inputarray[6] << 8) | (inputarray[7])); // reassign input to swapped value
}

//==========================================================================
// Purpose: read int from ARAM ***REQUIRES MMU TO BE DISABLED IN DOLPHIN***
// Parameter: address location
//==========================================================================
int32_t ARAM_ReadInt(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINARAMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	int32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + aramoffset + (addr - 0x7E000000)), &output, sizeof(output), NULL);
	MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}
//==========================================================================
// Purpose: read unsigned int from ARAM ***REQUIRES MMU TO BE DISABLED IN DOLPHIN***
// Parameter: address location
//==========================================================================
uint32_t ARAM_ReadUInt(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINARAMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + aramoffset + (addr - 0x7E000000)), &output, sizeof(output), NULL);
	MEM_ByteSwap32(&output); // byteswap
	return output;
}
//==========================================================================
// Purpose: read float from ARAM ***REQUIRES MMU TO BE DISABLED IN DOLPHIN***
// Parameter: address location
//==========================================================================
float ARAM_ReadFloat(const uint32_t addr)
{
	if(!emuoffset || NOTWITHINARAMRANGE(addr)) // if gamecube memory has not been init by dolphin or reading from outside of memory range
		return 0;
	float output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + aramoffset + (addr - 0x7E000000)), &output, sizeof(output), NULL);
	MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}
//==========================================================================
// Purpose: write unsigned int to ARAM ***REQUIRES MMU TO BE DISABLED IN DOLPHIN***
// Parameter: address location and value
//==========================================================================
void ARAM_WriteUInt(const uint32_t addr, uint32_t value)
{
	if(!emuoffset || NOTWITHINARAMRANGE(addr)) // if gamecube memory has not been init by dolphin or writing to outside of memory range
		return;
	MEM_ByteSwap32(&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + aramoffset + (addr - 0x7E000000)), &value, sizeof(value), NULL);
}
//==========================================================================
// Purpose: write float to ARAM ***REQUIRES MMU TO BE DISABLED IN DOLPHIN***
// Parameter: address location and value
//==========================================================================
void ARAM_WriteFloat(const uint32_t addr, float value)
{
	if(!emuoffset || NOTWITHINARAMRANGE(addr)) // if gamecube memory has not been init by dolphin or writing to outside of memory range
		return;
	MEM_ByteSwap32((uint32_t *)&value); // byteswap
	// ARAM offset = 0x02000000
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + aramoffset + (addr - 0x7E000000)), &value, sizeof(value), NULL);
}

uint32_t PS1_MEM_ReadPointer(const uint32_t addr)
{
	// assumes the address of a ps1 pointer in the form 0x80BbA1A2 - Bb = Bank, A1A2 = Address in bank
	// PS1 pointer stored in little endian (A2A1Bb80), ReadProcessMemory reads it in reverse resulting in 80BbA1A2
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output;
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return (output - 0x80000000); // return address minus the 0x8 on the front
}

uint32_t PS1_MEM_ReadWord(const uint32_t addr)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output;
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	MEM_ByteSwap32(&output); // byteswap
	return output;
}

uint32_t PS1_MEM_ReadUInt(const uint32_t addr)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output;
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

int32_t PS1_MEM_ReadInt(const uint32_t addr)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return 0;
	int32_t output;
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

int16_t PS1_MEM_ReadInt16(const uint32_t addr)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return 0;
	int16_t output;
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

uint16_t PS1_MEM_ReadHalfword(const uint32_t addr)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return 0;
	// read only 2 bytes
	uint16_t output;
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

uint8_t PS1_MEM_ReadByte(const uint32_t addr)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return 0;
	uint8_t output;
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

void PS1_MEM_WriteInt(const uint32_t addr, int32_t value)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void PS1_MEM_WriteInt16(const uint32_t addr, int16_t value)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void PS1_MEM_WriteWord(const uint32_t addr, uint32_t value)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void PS1_MEM_WriteHalfword(const uint32_t addr, uint16_t value)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void PS1_MEM_WriteByte(const uint32_t addr, uint8_t value)
{
	if(!emuoffset || PS1NOTWITHINMEMRANGE(addr))
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

//==========================================================================
// N64 addresses should not be byteswapped since they are stored little endian in emulator memory
//==========================================================================
uint32_t N64_MEM_ReadUInt(const uint32_t addr)
{
	if(!emuoffset || N64NOTWITHINMEMRANGE(addr)) // if n64 memory has not been init by emulator or reading from outside of memory range
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	return output;
}

int16_t N64_MEM_ReadInt16(const uint32_t addr)
{
	if(!emuoffset || N64NOTWITHINMEMRANGE(addr)) // if n64 memory has not been init by emulator or reading from outside of memory range
		return 0;
	int16_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	return output;
}

float N64_MEM_ReadFloat(const uint32_t addr)
{
	if(!emuoffset || N64NOTWITHINMEMRANGE(addr)) // if n64 memory has not been init by emulator or reading from outside of memory range
		return 0;
	float output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &output, sizeof(output), NULL);
	return output;
}

void N64_MEM_WriteUInt(const uint32_t addr, uint32_t value)
{
	if(!emuoffset || N64NOTWITHINMEMRANGE(addr)) // if n64 memory has not been init by emulator or writing to outside of memory range
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &value, sizeof(value), NULL);
}

void N64_MEM_WriteInt16(const uint32_t addr, int16_t value)
{
	if(!emuoffset || N64NOTWITHINMEMRANGE(addr)) // if n64 memory has not been init by emulator or writing to outside of memory range
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &value, sizeof(value), NULL);
}

void N64_MEM_WriteByte(const uint32_t addr, uint8_t value)
{
	if(!emuoffset || N64NOTWITHINMEMRANGE(addr)) // if n64 memory has not been init by emulator or writing to outside of memory range
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &value, sizeof(value), NULL);
}

void N64_MEM_WriteFloat(const uint32_t addr, float value)
{
	if(!emuoffset || N64NOTWITHINMEMRANGE(addr)) // if n64 memory has not been init by emulator or writing to outside of memory range
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + (addr - 0x80000000)), &value, sizeof(value), NULL);
}

uint8_t SNES_MEM_ReadByte(const uint32_t addr)
{
	if(!emuoffset || SNESNOTWITHINMEMRANGE(addr)) // if snes memory has not been init by emulator or reading from outside of memory range
		return 0;
	uint8_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

uint16_t SNES_MEM_ReadWord(const uint32_t addr) // 16bit word
{
	if(!emuoffset || SNESNOTWITHINMEMRANGE(addr)) // if snes memory has not been init by emulator or reading from outside of memory range
		return 0;
	uint16_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

void SNES_MEM_WriteByte(const uint32_t addr, uint8_t value)
{
	if(!emuoffset || SNESNOTWITHINMEMRANGE(addr)) // if snes memory has not been init by emulator or writing to outside of memory range
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void SNES_MEM_WriteWord(const uint32_t addr, uint16_t value) // 16bit word
{
	if(!emuoffset || SNESNOTWITHINMEMRANGE(addr)) // if snes memory has not been init by emulator or writing to outside of memory range
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}
//==========================================================================
//PS2 hooking via pointer. Use the base address of pcsx2.exe module -> point to static pointer/symbol "EEmem" -> Use its value as a "emuoffset". 
//==========================================================================
uint32_t PS2_MEM_ReadPointer(const uint32_t addr)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	// printdebug(1); // debug
	// MEM_ByteSwap32(&output); // byteswap
	return output;
}

uint32_t PS2_MEM_ReadWord(const uint32_t addr)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	// printdebug(1); // debug
	MEM_ByteSwap32(&output); // byteswap
	return output;
}

uint32_t PS2_MEM_ReadUInt(const uint32_t addr)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	// printdebug(1); // debug
	return output;
}

uint32_t PS2_MEM_ReadUInt16(const uint32_t addr)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr))
		return 0;
	uint16_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	// printdebug(1); // debug
	return output;
}

int16_t PS2_MEM_ReadInt16(const uint32_t addr)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr))
		return 0;
	int16_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

uint8_t PS2_MEM_ReadUInt8(const uint32_t addr)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr))
		return 0;
	uint8_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	// printdebug(1); // debug
	return output;
}

float PS2_MEM_ReadFloat(const uint32_t addr)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr)) 
		return 0;
	float output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	// MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}

void PS2_MEM_WriteWord(const uint32_t addr, uint32_t value)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr))
		return;
	MEM_ByteSwap32(&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void PS2_MEM_WriteUInt(const uint32_t addr, uint32_t value)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr))
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void PS2_MEM_WriteUInt16(const uint32_t addr, uint16_t value)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr))
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void PS2_MEM_WriteInt16(const uint32_t addr, int16_t value)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr))
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}


void PS2_MEM_WriteFloat(const uint32_t addr, float value)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr)) 
		return;
	// MEM_ByteSwap32((uint32_t *)&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

// TODO: give Dreamcast it's own within mem range
// =================================================
//		Sega Dreamcast
// =================================================
uint32_t SD_MEM_ReadWord(const uint32_t addr)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr)) 
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	// printdebug(1); // debug
	MEM_ByteSwap32(&output); // byteswap
	return output;
}

float SD_MEM_ReadFloat(const uint32_t addr)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr)) 
		return 0;
	float output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	// MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}

void SD_MEM_WriteFloat(const uint32_t addr, float value)
{
	if(!emuoffset || PS2NOTWITHINMEMRANGE(addr))
		return;
	// MEM_ByteSwap32((uint32_t *)&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}
// uint16_t SS_MEM_ReadHalfword(const uint32_t addr)
// {
// 	if(!emuoffset || SSNOTWITHINMEMRANGE(addr))
// 		return 0;
// 	// read only 2 bytes
// 	uint16_t output;
// 	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
// 	return output;
// }

uint32_t PS3_MEM_ReadUInt(const uint32_t addr)
{
	if(!emuoffset || PS3NOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}

uint32_t PS3_MEM_ReadPointer(const uint32_t addr)
{
	if(!emuoffset || PS3NOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	MEM_ByteSwap32((uint32_t *)&output); // byteswap
	if (output < 0x30000000)
		return 0; // not a pointer
	return output - 0x30000000;
}

float PS3_MEM_ReadFloat(const uint32_t addr)
{
	if(!emuoffset || PS3NOTWITHINMEMRANGE(addr))
		return 0;
	float output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}

void PS3_MEM_WriteFloat(const uint32_t addr, float value)
{
	if(!emuoffset || PS3NOTWITHINMEMRANGE(addr)) 
		return;
	MEM_ByteSwap32((uint32_t *)&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

uint32_t PSP_MEM_ReadWord(const uint32_t addr)
{
	if(!emuoffset || PSPNOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}

uint32_t PSP_MEM_ReadPointer(const uint32_t addr)
{
	if(!emuoffset || PSPNOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output - 0x8000000;
}

uint32_t PSP_MEM_ReadUInt(const uint32_t addr)
{
	if(!emuoffset || PSPNOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

uint16_t PSP_MEM_ReadUInt16(const uint32_t addr)
{
	if(!emuoffset || PSPNOTWITHINMEMRANGE(addr))
		return 0;
	uint32_t output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	return output;
}

float PSP_MEM_ReadFloat(const uint32_t addr)
{
	if(!emuoffset || PSPNOTWITHINMEMRANGE(addr)) 
		return 0;
	float output; // temp var used for output of function
	ReadProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &output, sizeof(output), NULL);
	// MEM_ByteSwap32((uint32_t *)&output); // byteswap
	return output;
}

void PSP_MEM_WriteUInt16(const uint32_t addr, uint16_t value)
{
	if(!emuoffset || PSPNOTWITHINMEMRANGE(addr))
		return;
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void PSP_MEM_WriteFloat(const uint32_t addr, float value)
{
	if(!emuoffset || PSPNOTWITHINMEMRANGE(addr))
		return;
	// MEM_ByteSwap32((uint32_t *)&value); // byteswap
	WriteProcessMemory(emuhandle, (LPVOID)(emuoffset + addr), &value, sizeof(value), NULL);
}

void printdebug(uint64_t val) //hexadecimal addresses debug
{
	FILE *fp;

	fp = fopen("test.txt", "w");
	if (fp != NULL) {
		char output[255];
		sprintf(output, "0x%" PRIx64, val);
		fprintf(fp, "%s", output);

		fclose(fp);
	} else {
	perror("Failed to open file");
	}
}

// =============================================================================================================================
//	Functions to look for specified symbol in the modules of a remote process, DLL injection would be much, much, much simpler
// =============================================================================================================================
HMODULE RemoteHandle(DWORD Process_ID, const TCHAR* modName) {
	HMODULE hMods[1024];
	DWORD cbNeeded;
	
	HANDLE snapshot = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Process_ID);
	if (!snapshot) return NULL;
	if (EnumProcessModulesEx(snapshot, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL)) {
		for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
			TCHAR szModName[MAX_PATH];
			if (GetModuleBaseName(snapshot, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
				if (_tcscmp(szModName, modName) == 0) {
					CloseHandle(snapshot);
					return hMods[i];
				}
			}
		}
	}
	CloseHandle(snapshot);
	return NULL;
}
FARPROC RemoteAddress(HANDLE snapshot, HMODULE hMod, const char* procName) {
	BYTE* base = (BYTE*)hMod;
	IMAGE_DOS_HEADER dosHeader;
	IMAGE_NT_HEADERS ntHeaders;
	IMAGE_EXPORT_DIRECTORY expDir;

	if (!ReadProcessMemory(snapshot, base, &dosHeader, sizeof(dosHeader), NULL)) return NULL; // Reads DOS header of the module
	if (!ReadProcessMemory(snapshot, base + dosHeader.e_lfanew, &ntHeaders, sizeof(ntHeaders), NULL)) return NULL; // Reads the PE header
	if (!ReadProcessMemory(snapshot, base + ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress, &expDir, sizeof(expDir), NULL)) return NULL; //Reads the exported directory

	DWORD* funcs = (DWORD*)malloc(expDir.NumberOfFunctions * sizeof(DWORD));
	DWORD* names = (DWORD*)malloc(expDir.NumberOfNames * sizeof(DWORD));
	WORD* ordinals = (WORD*)malloc(expDir.NumberOfNames * sizeof(WORD));
	if (!ReadProcessMemory(snapshot, base + expDir.AddressOfFunctions, funcs, expDir.NumberOfFunctions * sizeof(DWORD), NULL)) return NULL;
	if (!ReadProcessMemory(snapshot, base + expDir.AddressOfNames, names, expDir.NumberOfNames * sizeof(DWORD), NULL)) return NULL;
	if (!ReadProcessMemory(snapshot, base + expDir.AddressOfNameOrdinals, ordinals, expDir.NumberOfNames * sizeof(WORD), NULL)) return NULL;

	FARPROC addr = NULL;
	for (DWORD i = 0; i < expDir.NumberOfNames; i++) {
		char funcName[256];
		if (ReadProcessMemory(snapshot, base + names[i], funcName, sizeof(funcName), NULL) && strcmp(funcName, procName) == 0) {
			addr = (FARPROC)(base + funcs[ordinals[i]]);
			break;
		}
	}

	free(funcs);
	free(names);
	free(ordinals);
	
	return addr;
}