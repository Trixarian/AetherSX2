/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021 PCSX2 Dev Team
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

#include <string>
#include <string_view>
#include <optional>
#include <wx/string.h>

#include "common/Pcsx2Defs.h"
#include <optional>
#include <string>
#include <vector>

enum class CDVD_SourceType : uint8_t;

enum class VMState
{
	Shutdown,
	Starting,
	Running,
	Paused,
	Stopping,
};

struct VMBootParameters
{
	std::string source;
	std::string save_state;
	CDVD_SourceType source_type;
	std::string elf_override;
	std::optional<bool> fast_boot;
	std::optional<bool> fullscreen;
	std::optional<bool> batch_mode;
};

namespace VMManager
{
	/// Returns the current state of the VM.
	VMState GetState();

	/// Alters the current state of the VM.
	void SetState(VMState state);

	/// Returns true if there is an active virtual machine.
	bool HasValidVM();

	/// Returns the path of the disc currently running.
	std::string GetDiscPath();

	/// Returns the crc of the executable currently running.
	u32 GetGameCRC();

	/// Returns the serial of the disc/executable currently running.
	std::string GetGameSerial();

	/// Returns the name of the disc/executable currently running.
	std::string GetGameName();

	/// Reserves memory for the virtual machines.
	bool InitializeMemory();

	/// Completely releases all memory for the virtual machine.
	void ReleaseMemory();

	/// Initializes all system components.
	bool Initialize(const VMBootParameters& boot_params);

	/// Destroys all system components.
	void Shutdown(bool allow_save_resume_state = true);

	/// Resets all subsystems to a cold boot.
	void Reset();

	/// Runs the VM until the CPU execution is canceled.
	void Execute();

	/// Changes the pause state of the VM, resetting anything needed when unpausing.
	void SetPaused(bool paused);

	/// Reloads settings, and applies any changes present.
	void ApplySettings();

	/// Reloads game specific settings, and applys any changes present.
	void ReloadGameSettings();

	/// Reloads cheats/patches. If verbose is set, the number of patches loaded will be shown in the OSD.
	void ReloadPatches(bool verbose);

	/// Returns true if a resume save state should be saved/loaded.
	bool ShouldSaveResumeState();

	/// Returns the save state filename for the given game serial/crc.
	std::string GetSaveStateFileName(const char* game_serial, u32 game_crc, s32 slot);

	/// Returns true if there is a save state in the specified slot.
	bool HasSaveStateInSlot(const char* game_serial, u32 game_crc, s32 slot);

	/// Loads state from the specified file.
	bool LoadState(const char* filename);

	/// Loads state from the specified slot.
	bool LoadStateFromSlot(s32 slot);

	/// Saves state to the specified filename.
	bool SaveState(const char* filename);

	/// Saves state to the specified slot.
	bool SaveStateToSlot(s32 slot);

	/// Updates the host vsync state, as well as timer frequencies. Call when the speed limiter is adjusted.
	void SetLimiterMode(LimiterModeType type);

	/// Changes the disc in the virtual CD/DVD drive. Passing an empty will remove any current disc.
	/// Returns false if the new disc can't be opened.
	bool ChangeDisc(std::string path);

	/// Returns true if the specified path is an ELF.
	bool IsElfFileName(const std::string& path);

	/// Updates boot parameters for a given start filename. If it's an elf, it'll set elf_override, otherwise source.
	void SetBootParametersForPath(const std::string& path, VMBootParameters* params);

	/// Returns the path for the game settings ini file for the specified CRC.
	std::string GetGameSettingsPath(u32 game_crc);

	/// Internal callbacks, implemented in the emu core.
	namespace Internal
	{
		const std::string& GetElfOverride();
		bool IsExecutionInterrupted();
		void GameStartingOnCPUThread();
		void VSyncOnCPUThread();
	}
} // namespace VMManager


namespace Host
{
	/// Provided by the host; called when the running executable changes.
	void GameChanged(const std::string& disc_path, const std::string& game_serial, const std::string& game_name, u32 game_crc);

	/// Provided by the host; called once per frame at guest vsync.
	void PumpMessagesOnCPUThread();

	/// Provided by the host; called when a state is saved, and the frontend should invalidate its save state cache.
	void InvalidateSaveStateCache();
}
