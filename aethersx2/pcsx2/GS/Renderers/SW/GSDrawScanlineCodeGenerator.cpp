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

#include "PrecompiledHeader.h"
#include "GSDrawScanlineCodeGenerator.h"

#if defined(_M_X86_32) || defined(_M_X86_64)
#include "GSDrawScanlineCodeGenerator.all.h"
#elif defined(_M_ARM64)
#include "GSDrawScanlineCodeGenerator.arm64.h"
#else
#error Unknown target.
#endif

GSDrawScanlineCodeGenerator::GSDrawScanlineCodeGenerator(void* param, uint64 key, void* code, size_t maxsize)
	: GSCodeGenerator(code, maxsize)
	, m_local(*(GSScanlineLocalData*)param)
	, m_rip(false)
{
	m_sel.key = key;

#if defined(_M_X86_32) || defined(_M_X86_64)
	if (m_sel.breakpoint)
		db(0xCC);

	GSDrawScanlineCodeGenerator2(this, CPUInfo(m_cpu), (void*)&m_local, m_sel.key).Generate();
#elif defined(_M_ARM64)
	GSDrawScanlineCodeGenerator2(armAsm, param, key).Generate();
#endif
}
