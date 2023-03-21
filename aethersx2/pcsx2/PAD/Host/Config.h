/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2020  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "PAD/Host/Global.h"

class PADconf
{
	u32 ff_intensity;
	u32 sensibility;

public:
	union
	{
		struct
		{
			u16 forcefeedback : 1;
			u16 reverse_lx : 1;
			u16 reverse_ly : 1;
			u16 reverse_rx : 1;
			u16 reverse_ry : 1;
			u16 mouse_l : 1;
			u16 mouse_r : 1;
			u16 _free : 9;             // The 9 remaining bits are unused, do what you wish with them ;)
		} pad_options[GAMEPAD_NUMBER]; // One for each pads
		u32 packed_options;            // Only first 8 bits of each 16 bits series are really used, rest is padding
	};

	std::map<u32, int> keysym_map[GAMEPAD_NUMBER];
	std::array<size_t, GAMEPAD_NUMBER> unique_id;

	PADconf() { init(); }

	void init()
	{
		packed_options = 0;
		ff_intensity = 0x7FFF; // set it at max value by default
		sensibility = 100;
		for (u32 pad = 0; pad < GAMEPAD_NUMBER; pad++)
		{
			keysym_map[pad].clear();
		}
		unique_id.fill(0);
	}

	void set_joy_uid(u32 pad, size_t uid)
	{
		if (pad < GAMEPAD_NUMBER)
			unique_id[pad] = uid;
	}

	size_t get_joy_uid(u32 pad)
	{
		if (pad < GAMEPAD_NUMBER)
			return unique_id[pad];
		else
			return 0;
	}

	/**
	 * Return (a copy of) private memner ff_instensity
	 **/
	u32 get_ff_intensity()
	{
		return ff_intensity;
	}

	/**
	 * Set intensity while checking that the new value is within
	 * valid range, more than 0x7FFF will cause pad not to rumble(and less than 0 is obviously bad)
	 **/
	void set_ff_intensity(u32 new_intensity)
	{
		if (new_intensity <= 0x7FFF)
		{
			ff_intensity = new_intensity;
		}
	}

	/**
	 * Set sensibility value.
	 * There will be an upper range, and less than 0 is obviously wrong.
	 * We are doing object oriented code, so members are definitely not supposed to be public.
	 **/
	void set_sensibility(u32 new_sensibility)
	{
		if (new_sensibility > 0)
		{
			sensibility = new_sensibility;
		}
		else
		{
			sensibility = 1;
		}
	}

	u32 get_sensibility()
	{
		return sensibility;
	}
};
extern PADconf g_conf;

static void set_keyboard_key(u32 pad, u32 keysym, int index)
{
	g_conf.keysym_map[pad][keysym] = index;
}

static int get_keyboard_key(u32 pad, u32 keysym)
{
	// You must use find instead of []
	// [] will create an element if the key does not exist and return 0
	std::map<u32, int>::iterator it = g_conf.keysym_map[pad].find(keysym);
	if (it != g_conf.keysym_map[pad].end())
		return it->second;
	else
		return -1;
}
