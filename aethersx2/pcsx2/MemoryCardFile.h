/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

struct McdSizeInfo
{
	u16 SectorSize; // Size of each sector, in bytes.  (only 512 and 1024 are valid)
	u16 EraseBlockSizeInSectors; // Size of the erase block, in sectors (max is 16)
	u32 McdSizeInSectors; // Total size of the card, in sectors (no upper limit)
	u8 Xor; // Checksum of previous data
};

extern uint FileMcd_GetMtapPort(uint slot);
extern uint FileMcd_GetMtapSlot(uint slot);
extern bool FileMcd_IsMultitapSlot(uint slot);
//extern wxFileName FileMcd_GetSimpleName(uint slot);
extern std::string FileMcd_GetDefaultName(uint slot);
extern bool isValidNewFilename(wxString filenameStringToTest, wxDirName atBasePath, wxString& out_errorMessage, uint minNumCharacters = 5);


uint FileMcd_ConvertToSlot(uint port, uint slot);
void FileMcd_EmuOpen();
void FileMcd_EmuClose();
s32 FileMcd_IsPresent(uint port, uint slot);
void FileMcd_GetSizeInfo(uint port, uint slot, McdSizeInfo* outways);
bool FileMcd_IsPSX(uint port, uint slot);
s32 FileMcd_Read(uint port, uint slot, u8* dest, u32 adr, int size);
s32 FileMcd_Save(uint port, uint slot, const u8* src, u32 adr, int size);
s32 FileMcd_EraseBlock(uint port, uint slot, u32 adr);
u64 FileMcd_GetCRC(uint port, uint slot);
void FileMcd_NextFrame(uint port, uint slot);
bool FileMcd_ReIndex(uint port, uint slot, const wxString& filter);
