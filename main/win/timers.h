/***************************************************************************
						  timers.c  -  description
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

#include <chrono>
#include <queue>

#include "rom.h"

typedef std::chrono::high_resolution_clock::time_point time_point;

/// Frame invalidation flag: cleared by consumer
extern bool frame_changed;

/// Timepoints at which new frame happened
extern std::deque<time_point> new_frame_times;

/// Timepoints at which new VI happened
extern std::deque<time_point> new_vi_times;

/**
 * \brief Initializes timer code with the specified values
 * \param speed_modifier The current speed modifier
 * \param rom_header The current rom header
 */
void timer_init(int32_t speed_modifier, t_rom_header* rom_header);

/**
 * \brief To be called when a new frame is generated
 */
void timer_new_frame();

/**
 * \brief To be called when a VI is generated
 */
void timer_new_vi();
