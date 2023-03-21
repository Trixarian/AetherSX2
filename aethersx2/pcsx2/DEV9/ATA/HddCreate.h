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

#ifndef PCSX2_CORE
#include <wx/progdlg.h>
#endif

#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include "ghc/filesystem.h"

class HddCreate
{
public:
	ghc::filesystem::path filePath;
	int neededSize;

	std::atomic_bool errored{false};

private:
#ifndef PCSX2_CORE
	wxProgressDialog* progressDialog;
#endif
	std::atomic_int written{0};

	std::thread fileThread;

	std::atomic_bool canceled{false};

	std::mutex completedMutex;
	std::condition_variable completedCV;
	bool completed = false;

	std::chrono::steady_clock::time_point lastUpdate;

public:
	void Start();

private:
	void SetFileProgress(int currentSize);
	void SetError();
	void WriteImage(ghc::filesystem::path hddPath, int reqSizeMB);
};
