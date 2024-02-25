/***************************************************************************
						  main_win.h  -  description
							 -------------------
	copyright            : (C) 2003 by ShadowPrince
	email                : shadow@emulation64.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <Windows.h>
#include <string>
#include <functional>
#include <filesystem>
#define MUPEN_VERSION "Mupen 64 1.1.7"

#define WM_EXECUTE_DISPATCHER (WM_USER + 10)
#define WM_SEEK_COMPLETED (WM_USER + 11)
#define WM_FOCUS_MAIN_WINDOW (WM_USER + 17)

extern BOOL CALLBACK CfgDlgProc(HWND hwnd, UINT Message, WPARAM wParam,
                                LPARAM lParam);

extern int last_wheel_delta;

// TODO: remove
extern int recording;

extern HWND mainHWND;
extern HINSTANCE app_instance;

extern HWND hwnd_plug;
extern HANDLE EmuThreadHandle;
extern DWORD start_rom_id;

void main_dispatcher_invoke(const std::function<void()>& func);
extern std::string app_path;

/**
 * \brief Starts the emulator
 * \param path Path to a rom or corresponding movie file
 * \returns The operation status code
 */
int start_rom(std::filesystem::path path);

/**
 * \brief Stops the emulator
 * \param stop_vcr Whether all VCR operations will be stopped. When resetting the ROM due to an in-movie restart, this needs to be false.
 */
void close_rom(bool stop_vcr = true);

/**
 * \brief Resets the emulator
 * \param reset_save_data Whether save data (e.g.: EEPROM, SRAM, Mempak) will be reset
 * \param stop_vcr Whether all VCR operations will be stopped. When resetting the ROM due to an in-movie restart, this needs to be false.
 */
void reset_rom(bool reset_save_data = false, bool stop_vcr = true);

extern void resumeEmu(BOOL quiet);
extern void pauseEmu(BOOL quiet);


/**
 * \brief Whether the statusbar needs to be updated with new input information
 */
extern bool is_primary_statusbar_invalidated;

/**
 * \brief Pauses the emulation during the object's lifetime, resuming it if previously paused upon being destroyed
 */
struct BetterEmulationLock
{
private:
	bool was_paused;
public:
	BetterEmulationLock();
	~BetterEmulationLock();
};
