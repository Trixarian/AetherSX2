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

// To be continued...

#include "PrecompiledHeader.h"
#include "Dialogs.h"
#include <cstring>

#if defined(__unix__) || defined(__APPLE__)
#include <wx/wx.h>

void SysMessage(const char* fmt, ...)
{
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg) - 1] == '\n')
		msg[strlen(msg) - 1] = 0;

	wxMessageDialog dialog(nullptr, msg, "Info", wxOK);
	dialog.ShowModal();
}

void SysMessage(const wchar_t* fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	wxString msg;
	msg.PrintfV(fmt, list);
	va_end(list);

	wxMessageDialog dialog(nullptr, msg, "Info", wxOK);
	dialog.ShowModal();
}
#endif

void DspUpdate()
{
}

s32 DspLoadLibrary(wchar_t* fileName, int modnum)
{
	return 0;
}
