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


#include "PrecompiledHeader.h"
#include "IopCommon.h"
#include "IsoFileFormats.h"
#include "common/FileSystem.h"
#include "common/StringUtil.h"

#include <errno.h>

void pxStream_OpenCheck(std::FILE* stream, const std::string& fname, const wxString& mode)
{
	if (stream)
		return;

	ScopedExcept ex(Exception::FromErrno(StringUtil::UTF8StringToWxString(fname), errno));
	ex->SetDiagMsg(pxsFmt(L"Unable to open the file for %s: %s", WX_STR(mode), WX_STR(ex->DiagMsg())));
	ex->Rethrow();
}

OutputIsoFile::OutputIsoFile()
{
	_init();
}

OutputIsoFile::~OutputIsoFile()
{
	Close();
}

void OutputIsoFile::_init()
{
	m_version = 0;

	m_offset = 0;
	m_blockofs = 0;
	m_blocksize = 0;
	m_blocks = 0;
}

void OutputIsoFile::Create(std::string filename, int version)
{
	Close();
	m_filename = std::move(filename);

	m_version = version;
	m_offset = 0;
	m_blockofs = 24;
	m_blocksize = 2048;

	m_outstream = FileSystem::OpenCFile(m_filename.c_str(), "wb");
	pxStream_OpenCheck(m_outstream, m_filename, L"writing");

	Console.WriteLn("isoFile create ok: %s ", m_filename.c_str());
}

// Generates format header information for blockdumps.
void OutputIsoFile::WriteHeader(int _blockofs, uint _blocksize, uint _blocks)
{
	m_blocksize = _blocksize;
	m_blocks = _blocks;
	m_blockofs = _blockofs;

	Console.WriteLn("blockoffset = %d", m_blockofs);
	Console.WriteLn("blocksize   = %u", m_blocksize);
	Console.WriteLn("blocks	     = %u", m_blocks);

	if (m_version == 2)
	{
		WriteBuffer("BDV2", 4);
		WriteValue(m_blocksize);
		WriteValue(m_blocks);
		WriteValue(m_blockofs);
	}
}

void OutputIsoFile::WriteSector(const u8* src, uint lsn)
{
	if (m_version == 2)
	{
		// Find and ignore blocks that have already been dumped:
		if (std::any_of(std::begin(m_dtable), std::end(m_dtable), [=](const u32 entry) { return entry == lsn; }))
			return;

		m_dtable.push_back(lsn);

		WriteValue<u32>(lsn);
	}
	else
	{
		const s64 ofs = (s64)lsn * m_blocksize + m_offset;
		FileSystem::FSeek64(m_outstream, ofs, SEEK_SET);
	}

	WriteBuffer(src + m_blockofs, m_blocksize);
}

void OutputIsoFile::Close()
{
	m_dtable.clear();

	if (m_outstream)
	{
		std::fclose(m_outstream);
		m_outstream = nullptr;
	}

	_init();
}

void OutputIsoFile::WriteBuffer(const void* src, size_t size)
{
	if (std::fwrite(src, size, 1, m_outstream) != 1)
	{
		int err = errno;
		if (!err)
			throw Exception::BadStream(m_filename).SetDiagMsg(pxsFmt(L"An error occurred while writing %u bytes to file", size));

		ScopedExcept ex(Exception::FromErrno(m_filename, err));
		ex->SetDiagMsg(pxsFmt(L"An error occurred while writing %u bytes to file: %s", size, WX_STR(ex->DiagMsg())));
		ex->Rethrow();
	}
}

bool OutputIsoFile::IsOpened() const
{
	return m_outstream != nullptr;
}

u32 OutputIsoFile::GetBlockSize() const
{
	return m_blocksize;
}
