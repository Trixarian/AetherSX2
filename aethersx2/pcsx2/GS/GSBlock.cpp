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
#include "GSBlock.h"

#if _M_SSE >= 0x501
CONSTINIT const GSVector8i GSBlock::m_r16mask(0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15, 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15);
#else
CONSTINIT const GSVector4i GSBlock::m_r16mask(0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15);
#endif
CONSTINIT const GSVector4i GSBlock::m_r8mask(0, 4, 2, 6, 8, 12, 10, 14, 1, 5, 3, 7, 9, 13, 11, 15);
CONSTINIT const GSVector4i GSBlock::m_r4mask(0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15);

#if _M_SSE >= 0x501
CONSTINIT const GSVector8i GSBlock::m_xxxa = GSVector8i::cxpr(0x00008000);
CONSTINIT const GSVector8i GSBlock::m_xxbx = GSVector8i::cxpr(0x00007c00);
CONSTINIT const GSVector8i GSBlock::m_xgxx = GSVector8i::cxpr(0x000003e0);
CONSTINIT const GSVector8i GSBlock::m_rxxx = GSVector8i::cxpr(0x0000001f);
#else
CONSTINIT const GSVector4i GSBlock::m_xxxa = GSVector4i::cxpr(0x00008000);
CONSTINIT const GSVector4i GSBlock::m_xxbx = GSVector4i::cxpr(0x00007c00);
CONSTINIT const GSVector4i GSBlock::m_xgxx = GSVector4i::cxpr(0x000003e0);
CONSTINIT const GSVector4i GSBlock::m_rxxx = GSVector4i::cxpr(0x0000001f);
#endif

CONSTINIT const GSVector4i GSBlock::m_uw8hmask0(0, 0, 0, 0, 1, 1, 1, 1, 8, 8, 8, 8, 9, 9, 9, 9);
CONSTINIT const GSVector4i GSBlock::m_uw8hmask1(2, 2, 2, 2, 3, 3, 3, 3, 10, 10, 10, 10, 11, 11, 11, 11);
CONSTINIT const GSVector4i GSBlock::m_uw8hmask2(4, 4, 4, 4, 5, 5, 5, 5, 12, 12, 12, 12, 13, 13, 13, 13);
CONSTINIT const GSVector4i GSBlock::m_uw8hmask3(6, 6, 6, 6, 7, 7, 7, 7, 14, 14, 14, 14, 15, 15, 15, 15);
