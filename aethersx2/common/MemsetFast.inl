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

#if defined(_M_X86_32) || defined(_M_X86_64)

#include <xmmintrin.h>

template <u8 data>
__noinline void memset_sse_a(void* dest, const size_t size)
{
	const uint MZFqwc = size / 16;

	pxAssert((size & 0xf) == 0);

	__m128 srcreg;

	if (data != 0)
	{
		static __aligned16 const u8 loadval[8] = {data, data, data, data, data, data, data, data};
		srcreg = _mm_loadh_pi(_mm_load_ps((float*)loadval), (__m64*)loadval);
	}
	else
		srcreg = _mm_setzero_ps();

	float(*destxmm)[4] = (float(*)[4])dest;

	switch (MZFqwc & 0x07)
	{
		case 0x07:
			_mm_store_ps(&destxmm[0x07 - 1][0], srcreg);
			// Fall through
		case 0x06:
			_mm_store_ps(&destxmm[0x06 - 1][0], srcreg);
			// Fall through
		case 0x05:
			_mm_store_ps(&destxmm[0x05 - 1][0], srcreg);
			// Fall through
		case 0x04:
			_mm_store_ps(&destxmm[0x04 - 1][0], srcreg);
			// Fall through
		case 0x03:
			_mm_store_ps(&destxmm[0x03 - 1][0], srcreg);
			// Fall through
		case 0x02:
			_mm_store_ps(&destxmm[0x02 - 1][0], srcreg);
			// Fall through
		case 0x01:
			_mm_store_ps(&destxmm[0x01 - 1][0], srcreg);
			// Fall through
	}

	destxmm += (MZFqwc & 0x07);
	for (uint i = 0; i < MZFqwc / 8; ++i, destxmm += 8)
	{
		_mm_store_ps(&destxmm[0][0], srcreg);
		_mm_store_ps(&destxmm[1][0], srcreg);
		_mm_store_ps(&destxmm[2][0], srcreg);
		_mm_store_ps(&destxmm[3][0], srcreg);
		_mm_store_ps(&destxmm[4][0], srcreg);
		_mm_store_ps(&destxmm[5][0], srcreg);
		_mm_store_ps(&destxmm[6][0], srcreg);
		_mm_store_ps(&destxmm[7][0], srcreg);
	}
};

static __fi void memzero_sse_a(void* dest, const size_t size)
{
	memset_sse_a<0>(dest, size);
}

template <u8 data, typename T>
__noinline void memset_sse_a(T& dest)
{
	static_assert((sizeof(dest) & 0xf) == 0, "Bad size for SSE memset");
	memset_sse_a<data>(&dest, sizeof(dest));
}

template <typename T>
void memzero_sse_a(T& dest)
{
	static_assert((sizeof(dest) & 0xf) == 0, "Bad size for SSE memset");
	memset_sse_a<0>(&dest, sizeof(dest));
}

#elif defined(_M_ARM64)

#include <cstring>

static __fi void memzero_sse_a(void* dest, const size_t size)
{
  std::memset(dest, 0, size);
}

template <u8 data, typename T>
__noinline void memset_sse_a(T& dest)
{
  static_assert((sizeof(dest) & 0xf) == 0, "Bad size for SSE memset");
  std::memset(dest, data, sizeof(dest));
}

template <typename T>
void memzero_sse_a(T& dest)
{
  static_assert((sizeof(dest) & 0xf) == 0, "Bad size for SSE memset");
  std::memset(&dest, 0, sizeof(dest));
}

#endif
