/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021  PCSX2 Dev Team
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
#include "Pcsx2Defs.h"
#include <cstdint>
#include <cstddef>

struct fastjmp_buf
{
#if defined(_M_X86_64) && defined(_WIN32)
	static constexpr std::size_t BUF_SIZE = 240;
#elif defined(_M_X86_64)
	static constexpr std::size_t BUF_SIZE = 64;
#elif defined(_M_X86_32)
	static constexpr std::size_t BUF_SIZE = 24;
#elif defined(_M_ARM64)
	static constexpr std::size_t BUF_SIZE = 168;
#else
#error Unknown architecture.
#endif

	alignas(16) std::uint8_t buf[BUF_SIZE];
};

extern "C" {
int __fastcall fastjmp_set(fastjmp_buf* buf);
__noreturn void __fastcall fastjmp_jmp(const fastjmp_buf* buf, int ret);
}
