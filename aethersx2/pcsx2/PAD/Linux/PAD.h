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

#include "Global.h"
#include "Host.h"
#include "common/mt_queue.h"
#include "SaveState.h"

enum PadOptions
{
	PADOPTION_FORCEFEEDBACK = 0x1,
	PADOPTION_REVERSELX = 0x2,
	PADOPTION_REVERSELY = 0x4,
	PADOPTION_REVERSERX = 0x8,
	PADOPTION_REVERSERY = 0x10,
	PADOPTION_MOUSE_L = 0x20,
	PADOPTION_MOUSE_R = 0x40,
};

struct WindowInfo;

extern FILE* padLog;
extern void initLogging();

extern HostKeyEvent event;
extern MtQueue<HostKeyEvent> g_ev_fifo;

s32 _PADopen(const WindowInfo& wi);
void _PADclose();
void PADsetMode(int pad, int mode);

void SysMessage(char* fmt, ...);

s32 PADinit();
void PADshutdown();
s32 PADopen(const WindowInfo& wi);
void PADsetLogDir(const char* dir);
void PADclose();
s32 PADsetSlot(u8 port, u8 slot);
s32 PADfreeze(FreezeAction mode, freezeData* data);
u8 PADstartPoll(int pad);
u8 PADpoll(u8 value);
HostKeyEvent* PADkeyEvent();
void PADupdate(int pad);
void PADconfigure();

#if defined(__unix__)
void PADWriteEvent(HostKeyEvent& evt);
#endif
