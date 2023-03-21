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

// This file is filled with stuff that breaks clang-format
// clang-format off

#include "config.h"
#include "common/Pcsx2Types.h"
#include "common/WindowInfo.h"
#include "GS_types.h"
#include "Window/GSSetting.h"
#include "SaveState.h"
#include "pcsx2/Config.h"

#ifdef _WIN32

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <d3dcompiler.h>
#include <d3d11_1.h>
#include <dxgi1_3.h>

#else

#include <fcntl.h>

#endif

#ifdef __x86_64__
#define _M_AMD64
#endif

#include "Renderers/OpenGL/GLLoader.h"

#ifdef _WIN32

	#define DIRECTORY_SEPARATOR '\\'

#else

	#include <sys/stat.h> // mkdir

	#define DIRECTORY_SEPARATOR '/'

#endif

// put these into vc9/common7/ide/usertype.dat to have them highlighted


// stdc

#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <ctime>
#include <climits>
#include <cstring>
#include <cassert>

#if __GNUC__ > 5 || (__GNUC__ == 5 && __GNUC_MINOR__ >= 4)
#include <codecvt>
#include <locale>
#endif

#include <complex>
#include <string>
#include <array>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>
#include <bitset>

#ifdef __POSIX__
#include <zlib.h>
#else
#include <zlib/zlib.h>
#endif

#include <unordered_map>
#include <unordered_set>

// Don't un-indent our ifdefs
// clang-format off

#ifdef _MSC_VER

	#define EXPORT_C_(type) extern "C" type __stdcall
	#define EXPORT_C EXPORT_C_(void)

	#define ALIGN_STACK(n) alignas(n) int dummy__; (void)dummy__;

#else

	#ifndef __fastcall
		#define __fastcall __attribute__((fastcall))
	#endif

	#define EXPORT_C_(type) extern "C" __attribute__((stdcall, externally_visible, visibility("default"))) type
	#define EXPORT_C EXPORT_C_(void)

	#ifdef __GNUC__
		// GCC removes the variable as dead code and generates some warnings.
		// Stack is automatically realigned due to SSE/AVX operations
		#define ALIGN_STACK(n) (void)0;

	#else

		// TODO Check clang behavior
		#define ALIGN_STACK(n) alignas(n) int dummy__;

	#endif


#endif

#define countof(a) (sizeof(a) / sizeof(a[0]))

#ifndef RESTRICT

	#ifdef __INTEL_COMPILER

		#define RESTRICT restrict

	#elif defined(_MSC_VER)

		#define RESTRICT __restrict

	#elif defined(__GNUC__)

		#define RESTRICT __restrict__

	#else

		#define RESTRICT

	#endif

#endif

#if defined(_M_X86_32) || defined(_M_X86_64)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

#if defined(_M_ARM64)
#ifdef _MSC_VER
#include <arm64_neon.h>
#include <intrin.h>
#else
#include <arm_neon.h>
#endif

static __forceinline uint64_t __rdtsc()
{
#ifndef _MSC_VER
  uint64_t val;
  asm volatile("mrs %0, cntvct_el0" : "=r" (val));
	return val;
#else
	return _ReadStatusReg(ARM64_PMCCNTR_EL0);// or ARM64_CNTVCT?
#endif
}

#endif

#ifndef _MM_DENORMALS_ARE_ZERO
#define _MM_DENORMALS_ARE_ZERO 0x0040
#endif

#define MXCSR (_MM_DENORMALS_ARE_ZERO | _MM_MASK_MASK | _MM_ROUND_NEAREST | _MM_FLUSH_ZERO_ON)

#define _MM_TRANSPOSE4_SI128(row0, row1, row2, row3) \
{ \
	__m128 tmp0 = _mm_shuffle_ps(_mm_castsi128_ps(row0), _mm_castsi128_ps(row1), 0x44); \
	__m128 tmp2 = _mm_shuffle_ps(_mm_castsi128_ps(row0), _mm_castsi128_ps(row1), 0xEE); \
	__m128 tmp1 = _mm_shuffle_ps(_mm_castsi128_ps(row2), _mm_castsi128_ps(row3), 0x44); \
	__m128 tmp3 = _mm_shuffle_ps(_mm_castsi128_ps(row2), _mm_castsi128_ps(row3), 0xEE); \
	(row0) = _mm_castps_si128(_mm_shuffle_ps(tmp0, tmp1, 0x88)); \
	(row1) = _mm_castps_si128(_mm_shuffle_ps(tmp0, tmp1, 0xDD)); \
	(row2) = _mm_castps_si128(_mm_shuffle_ps(tmp2, tmp3, 0x88)); \
	(row3) = _mm_castps_si128(_mm_shuffle_ps(tmp2, tmp3, 0xDD)); \
}

#if defined(_M_X86_32) || defined(_M_X86_64)
#include <tmmintrin.h>
#include <smmintrin.h>
#endif

#if _M_SSE >= 0x500

	#include <immintrin.h>

#endif

#undef min
#undef max
#undef abs

#if !defined(_MSC_VER)
	// http://svn.reactos.org/svn/reactos/trunk/reactos/include/crt/mingw32/intrin_x86.h?view=markup

	static int _BitScanForward(unsigned long* const Index, const unsigned long Mask)
	{
#if defined(__aarch64__)
    *Index = __builtin_ctz(Mask);
    return Mask != 0;
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && 0
		// Need GCC6 to test the code validity
		int flag;
		__asm__("bsfl %k[Mask], %k[Index]" : [Index] "=r" (*Index), "=@ccz" (flag) : [Mask] "mr" (Mask));
		return flag;
#else
		__asm__("bsfl %k[Mask], %k[Index]" : [Index] "=r" (*Index) : [Mask] "mr" (Mask) : "cc");
		return Mask ? 1 : 0;
#endif
	}

#endif

extern std::string format(const char* fmt, ...);

extern void* vmalloc(size_t size, bool code);
extern void vmfree(void* ptr, size_t size);

extern void* fifo_alloc(size_t size, size_t repeat);
extern void fifo_free(void* ptr, size_t size, size_t repeat);

#ifdef ENABLE_VTUNE

	#include "jitprofiling.h"

	#ifdef _WIN32

	#pragma comment(lib, "jitprofiling.lib")

	#endif

#endif

// Note: GL messages are present in common code, so in all renderers.

#define GL_INSERT(type, code, sev, ...) \
	do \
		if (glDebugMessageInsert) glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, type, code, sev, -1, format(__VA_ARGS__).c_str()); \
	while(0);

#if defined(_DEBUG)
#  define GL_CACHE(...) GL_INSERT(GL_DEBUG_TYPE_OTHER, 0xFEAD, GL_DEBUG_SEVERITY_NOTIFICATION, __VA_ARGS__)
#else
#  define GL_CACHE(...) (void)(0);
#endif

#if defined(ENABLE_TRACE_REG) && defined(_DEBUG)
#  define GL_REG(...) GL_INSERT(GL_DEBUG_TYPE_OTHER, 0xB0B0, GL_DEBUG_SEVERITY_NOTIFICATION, __VA_ARGS__)
#else
#  define GL_REG(...) (void)(0);
#endif

#if defined(ENABLE_EXTRA_LOG) && defined(_DEBUG)
#  define GL_DBG(...) GL_INSERT(GL_DEBUG_TYPE_OTHER, 0xD0D0, GL_DEBUG_SEVERITY_NOTIFICATION, __VA_ARGS__)
#else
#  define GL_DBG(...) (void)(0);
#endif

#if defined(ENABLE_OGL_DEBUG)
	struct GLAutoPop
	{
		~GLAutoPop()
		{
			if (glPopDebugGroup)
				glPopDebugGroup();
		}
	};

	#define GL_PUSH_(...) do if (glPushDebugGroup) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0xBAD, -1, format(__VA_ARGS__).c_str()); while(0);
	#define GL_PUSH(...)  do if (glPushDebugGroup) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0xBAD, -1, format(__VA_ARGS__).c_str()); while(0); GLAutoPop gl_auto_pop;
	#define GL_POP()      do if (glPopDebugGroup) glPopDebugGroup(); while(0);
	#define GL_INS(...)   GL_INSERT(GL_DEBUG_TYPE_ERROR, 0xDEAD, GL_DEBUG_SEVERITY_MEDIUM, __VA_ARGS__)
	#define GL_PERF(...)  GL_INSERT(GL_DEBUG_TYPE_PERFORMANCE, 0xFEE1, GL_DEBUG_SEVERITY_NOTIFICATION, __VA_ARGS__)
#else
	#define GL_PUSH_(...) (void)(0);
	#define GL_PUSH(...) (void)(0);
	#define GL_POP()     (void)(0);
	#define GL_INS(...)  (void)(0);
	#define GL_PERF(...) (void)(0);
#endif

// Helper path to dump texture
extern const std::string root_sw;
extern const std::string root_hw;

// MacOS headers define PAGE_SIZE to the size of an x86 page
#ifdef PAGE_SIZE
#  undef PAGE_SIZE
#endif

#define VM_SIZE 4194304u
#define HALF_VM_SIZE (VM_SIZE / 2u)
#define PAGE_SIZE 8192u
#define BLOCK_SIZE 256u
#define COLUMN_SIZE 64u

#define MAX_PAGES (VM_SIZE / PAGE_SIZE)
#define MAX_BLOCKS (VM_SIZE / BLOCK_SIZE)
#define MAX_COLUMNS (VM_SIZE / COLUMN_SIZE)

//if defined, will send much info in reply to the API title info queri from PCSX2
//default should be undefined
//#define GSTITLEINFO_API_FORCE_VERBOSE

#include "GSVector.h"

#pragma pack(push, 1)

enum GS_PRIM
{
	GS_POINTLIST     = 0,
	GS_LINELIST      = 1,
	GS_LINESTRIP     = 2,
	GS_TRIANGLELIST  = 3,
	GS_TRIANGLESTRIP = 4,
	GS_TRIANGLEFAN   = 5,
	GS_SPRITE        = 6,
	GS_INVALID       = 7,
};

enum GS_PRIM_CLASS
{
	GS_POINT_CLASS    = 0,
	GS_LINE_CLASS     = 1,
	GS_TRIANGLE_CLASS = 2,
	GS_SPRITE_CLASS   = 3,
	GS_INVALID_CLASS  = 7,
};

enum GIF_REG
{
	GIF_REG_PRIM    = 0x00,
	GIF_REG_RGBA    = 0x01,
	GIF_REG_STQ     = 0x02,
	GIF_REG_UV      = 0x03,
	GIF_REG_XYZF2   = 0x04,
	GIF_REG_XYZ2    = 0x05,
	GIF_REG_TEX0_1  = 0x06,
	GIF_REG_TEX0_2  = 0x07,
	GIF_REG_CLAMP_1 = 0x08,
	GIF_REG_CLAMP_2 = 0x09,
	GIF_REG_FOG     = 0x0a,
	GIF_REG_INVALID = 0x0b,
	GIF_REG_XYZF3   = 0x0c,
	GIF_REG_XYZ3    = 0x0d,
	GIF_REG_A_D     = 0x0e,
	GIF_REG_NOP     = 0x0f,
};

enum GIF_REG_COMPLEX
{
	GIF_REG_STQRGBAXYZF2 = 0x00,
	GIF_REG_STQRGBAXYZ2 = 0x01,
};

enum GIF_A_D_REG
{
	GIF_A_D_REG_PRIM       = 0x00,
	GIF_A_D_REG_RGBAQ      = 0x01,
	GIF_A_D_REG_ST         = 0x02,
	GIF_A_D_REG_UV         = 0x03,
	GIF_A_D_REG_XYZF2      = 0x04,
	GIF_A_D_REG_XYZ2       = 0x05,
	GIF_A_D_REG_TEX0_1     = 0x06,
	GIF_A_D_REG_TEX0_2     = 0x07,
	GIF_A_D_REG_CLAMP_1    = 0x08,
	GIF_A_D_REG_CLAMP_2    = 0x09,
	GIF_A_D_REG_FOG        = 0x0a,
	GIF_A_D_REG_XYZF3      = 0x0c,
	GIF_A_D_REG_XYZ3       = 0x0d,
	GIF_A_D_REG_NOP        = 0x0f,
	GIF_A_D_REG_TEX1_1     = 0x14,
	GIF_A_D_REG_TEX1_2     = 0x15,
	GIF_A_D_REG_TEX2_1     = 0x16,
	GIF_A_D_REG_TEX2_2     = 0x17,
	GIF_A_D_REG_XYOFFSET_1 = 0x18,
	GIF_A_D_REG_XYOFFSET_2 = 0x19,
	GIF_A_D_REG_PRMODECONT = 0x1a,
	GIF_A_D_REG_PRMODE     = 0x1b,
	GIF_A_D_REG_TEXCLUT    = 0x1c,
	GIF_A_D_REG_SCANMSK    = 0x22,
	GIF_A_D_REG_MIPTBP1_1  = 0x34,
	GIF_A_D_REG_MIPTBP1_2  = 0x35,
	GIF_A_D_REG_MIPTBP2_1  = 0x36,
	GIF_A_D_REG_MIPTBP2_2  = 0x37,
	GIF_A_D_REG_TEXA       = 0x3b,
	GIF_A_D_REG_FOGCOL     = 0x3d,
	GIF_A_D_REG_TEXFLUSH   = 0x3f,
	GIF_A_D_REG_SCISSOR_1  = 0x40,
	GIF_A_D_REG_SCISSOR_2  = 0x41,
	GIF_A_D_REG_ALPHA_1    = 0x42,
	GIF_A_D_REG_ALPHA_2    = 0x43,
	GIF_A_D_REG_DIMX       = 0x44,
	GIF_A_D_REG_DTHE       = 0x45,
	GIF_A_D_REG_COLCLAMP   = 0x46,
	GIF_A_D_REG_TEST_1     = 0x47,
	GIF_A_D_REG_TEST_2     = 0x48,
	GIF_A_D_REG_PABE       = 0x49,
	GIF_A_D_REG_FBA_1      = 0x4a,
	GIF_A_D_REG_FBA_2      = 0x4b,
	GIF_A_D_REG_FRAME_1    = 0x4c,
	GIF_A_D_REG_FRAME_2    = 0x4d,
	GIF_A_D_REG_ZBUF_1     = 0x4e,
	GIF_A_D_REG_ZBUF_2     = 0x4f,
	GIF_A_D_REG_BITBLTBUF  = 0x50,
	GIF_A_D_REG_TRXPOS     = 0x51,
	GIF_A_D_REG_TRXREG     = 0x52,
	GIF_A_D_REG_TRXDIR     = 0x53,
	GIF_A_D_REG_HWREG      = 0x54,
	GIF_A_D_REG_SIGNAL     = 0x60,
	GIF_A_D_REG_FINISH     = 0x61,
	GIF_A_D_REG_LABEL      = 0x62,
};

enum GIF_FLG
{
	GIF_FLG_PACKED  = 0,
	GIF_FLG_REGLIST = 1,
	GIF_FLG_IMAGE   = 2,
	GIF_FLG_IMAGE2  = 3,
};

enum GS_PSM
{
	PSM_PSMCT32  =  0, // 0000-0000
	PSM_PSMCT24  =  1, // 0000-0001
	PSM_PSMCT16  =  2, // 0000-0010
	PSM_PSMCT16S = 10, // 0000-1010
	PSM_PSGPU24  = 18, // 0001-0010
	PSM_PSMT8    = 19, // 0001-0011
	PSM_PSMT4    = 20, // 0001-0100
	PSM_PSMT8H   = 27, // 0001-1011
	PSM_PSMT4HL  = 36, // 0010-0100
	PSM_PSMT4HH  = 44, // 0010-1100
	PSM_PSMZ32   = 48, // 0011-0000
	PSM_PSMZ24   = 49, // 0011-0001
	PSM_PSMZ16   = 50, // 0011-0010
	PSM_PSMZ16S  = 58, // 0011-1010
};

enum GS_TFX
{
	TFX_MODULATE   = 0,
	TFX_DECAL      = 1,
	TFX_HIGHLIGHT  = 2,
	TFX_HIGHLIGHT2 = 3,
	TFX_NONE       = 4,
};

enum GS_CLAMP
{
	CLAMP_REPEAT        = 0,
	CLAMP_CLAMP         = 1,
	CLAMP_REGION_CLAMP  = 2,
	CLAMP_REGION_REPEAT = 3,
};

enum GS_ZTST
{
	ZTST_NEVER   = 0,
	ZTST_ALWAYS  = 1,
	ZTST_GEQUAL  = 2,
	ZTST_GREATER = 3,
};

enum GS_ATST
{
	ATST_NEVER    = 0,
	ATST_ALWAYS   = 1,
	ATST_LESS     = 2,
	ATST_LEQUAL   = 3,
	ATST_EQUAL    = 4,
	ATST_GEQUAL   = 5,
	ATST_GREATER  = 6,
	ATST_NOTEQUAL = 7,
};

enum GS_AFAIL
{
	AFAIL_KEEP     = 0,
	AFAIL_FB_ONLY  = 1,
	AFAIL_ZB_ONLY  = 2,
	AFAIL_RGB_ONLY = 3,
};

enum class GS_MIN_FILTER : uint8_t
{
	Nearest                = 0,
	Linear                 = 1,
	Nearest_Mipmap_Nearest = 2,
	Nearest_Mipmap_Linear  = 3,
	Linear_Mipmap_Nearest  = 4,
	Linear_Mipmap_Linear   = 5,
};

#define REG32(name) \
	union name      \
	{               \
		uint32 u32; \
		struct {    \

#define REG64(name)    \
	union name         \
	{                  \
		uint64 u64;    \
		uint32 u32[2]; \
		void operator = (const GSVector4i& v) {GSVector4i::storel(this, v);} \
		bool operator == (const union name& r) const {return ((GSVector4i)r).eq(*this);} \
		bool operator != (const union name& r) const {return !((GSVector4i)r).eq(*this);} \
		operator GSVector4i() const {return GSVector4i::loadl(this);} \
		struct {

#define REG128(name)   \
	union name         \
	{                  \
		uint64 u64[2]; \
		uint32 u32[4]; \
		struct {

#define REG32_(prefix, name) REG32(prefix##name)
#define REG64_(prefix, name) REG64(prefix##name)
#define REG128_(prefix, name) REG128(prefix##name)

#define REG_END }; };
#define REG_END2 };

#define REG32_SET(name) \
union name          \
{                   \
	uint32 u32;

#define REG64_SET(name) \
	union name          \
	{                   \
		uint64 u64;     \
		uint32 u32[2];

#define REG128_SET(name) \
	union name           \
	{                    \
		uint64 u64[2];   \
		uint32 u32[4];

#define REG_SET_END };

REG64_(GSReg, BGCOLOR)
	uint8 R;
	uint8 G;
	uint8 B;
	uint8 _PAD1[5];
REG_END

REG64_(GSReg, BUSDIR)
	uint32 DIR   :  1;
	uint32 _PAD1 : 31;
	uint32 _PAD2 : 32;
REG_END

REG64_(GSReg, CSR)
	uint32 rSIGNAL : 1;
	uint32 rFINISH : 1;
	uint32 rHSINT  : 1;
	uint32 rVSINT  : 1;
	uint32 rEDWINT : 1;
	uint32 rZERO1  : 1;
	uint32 rZERO2  : 1;
	uint32 r_PAD1  : 1;
	uint32 rFLUSH  : 1;
	uint32 rRESET  : 1;
	uint32 r_PAD2  : 2;
	uint32 rNFIELD : 1;
	uint32 rFIELD  : 1;
	uint32 rFIFO   : 2;
	uint32 rREV    : 8;
	uint32 rID     : 8;
	uint32 wSIGNAL : 1;
	uint32 wFINISH : 1;
	uint32 wHSINT  : 1;
	uint32 wVSINT  : 1;
	uint32 wEDWINT : 1;
	uint32 wZERO1  : 1;
	uint32 wZERO2  : 1;
	uint32 w_PAD1  : 1;
	uint32 wFLUSH  : 1;
	uint32 wRESET  : 1;
	uint32 w_PAD2  : 2;
	uint32 wNFIELD : 1;
	uint32 wFIELD  : 1;
	uint32 wFIFO   : 2;
	uint32 wREV    : 8;
	uint32 wID     : 8;
REG_END

REG64_(GSReg, DISPFB) // (-1/2)
	uint32 FBP   :  9;
	uint32 FBW   :  6;
	uint32 PSM   :  5;
	uint32 _PAD  : 12;
	uint32 DBX   : 11;
	uint32 DBY   : 11;
	uint32 _PAD2 : 10;
REG_END2
	uint32 Block() const { return FBP << 5; }
REG_END2

REG64_(GSReg, DISPLAY) // (-1/2)
	uint32 DX    : 12;
	uint32 DY    : 11;
	uint32 MAGH  :  4;
	uint32 MAGV  :  2;
	uint32 _PAD  :  3;
	uint32 DW    : 12;
	uint32 DH    : 11;
	uint32 _PAD2 :  9;
REG_END

REG64_(GSReg, EXTBUF)
	uint32 EXBP  : 14;
	uint32 EXBW  :  6;
	uint32 FBIN  :  2;
	uint32 WFFMD :  1;
	uint32 EMODA :  2;
	uint32 EMODC :  2;
	uint32 _PAD1 :  5;
	uint32 WDX   : 11;
	uint32 WDY   : 11;
	uint32 _PAD2 : 10;
REG_END

REG64_(GSReg, EXTDATA)
	uint32 SX    : 12;
	uint32 SY    : 11;
	uint32 SMPH  :  4;
	uint32 SMPV  :  2;
	uint32 _PAD1 :  3;
	uint32 WW    : 12;
	uint32 WH    : 11;
	uint32 _PAD2 :  9;
REG_END

REG64_(GSReg, EXTWRITE)
	uint32 WRITE :  1;
	uint32 _PAD1 : 31;
	uint32 _PAD2 : 32;
REG_END

REG64_(GSReg, IMR)
	uint32 _PAD1     :  8;
	uint32 SIGMSK    :  1;
	uint32 FINISHMSK :  1;
	uint32 HSMSK     :  1;
	uint32 VSMSK     :  1;
	uint32 EDWMSK    :  1;
	uint32 _PAD2     : 19;
	uint32 _PAD3     : 32;
REG_END

REG64_(GSReg, PMODE)
union
{
	struct
	{
		uint32 EN1   :  1;
		uint32 EN2   :  1;
		uint32 CRTMD :  3;
		uint32 MMOD  :  1;
		uint32 AMOD  :  1;
		uint32 SLBG  :  1;
		uint32 ALP   :  8;
		uint32 _PAD  : 16;
		uint32 _PAD1 : 32;
	};

	struct
	{
		uint32 EN    :  2;
		uint32 _PAD2 : 30;
		uint32 _PAD3 : 32;
	};
};
REG_END

REG64_(GSReg, SIGLBLID)
	uint32 SIGID;
	uint32 LBLID;
REG_END

REG64_(GSReg, SMODE1)
	uint32 RC     :  3;
	uint32 LC     :  7;
	uint32 T1248  :  2;
	uint32 SLCK   :  1;
	uint32 CMOD   :  2;
	uint32 EX     :  1;
	uint32 PRST   :  1;
	uint32 SINT   :  1;
	uint32 XPCK   :  1;
	uint32 PCK2   :  2;
	uint32 SPML   :  4;
	uint32 GCONT  :  1; // YCrCb
	uint32 PHS    :  1;
	uint32 PVS    :  1;
	uint32 PEHS   :  1;
	uint32 PEVS   :  1;
	uint32 CLKSEL :  2;
	uint32 NVCK   :  1;
	uint32 SLCK2  :  1;
	uint32 VCKSEL :  2;
	uint32 VHP    :  1;
	uint32 _PAD1  : 27;
REG_END

/*

// pal

CLKSEL=1 CMOD=3 EX=0 GCONT=0 LC=32 NVCK=1 PCK2=0 PEHS=0 PEVS=0 PHS=0 PRST=1 PVS=0 RC=4 SINT=0 SLCK=0 SLCK2=1 SPML=4 T1248=1 VCKSEL=1 VHP=0 XPCK=0

// ntsc

CLKSEL=1 CMOD=2 EX=0 GCONT=0 LC=32 NVCK=1 PCK2=0 PEHS=0 PEVS=0 PHS=0 PRST=1 PVS=0 RC=4 SINT=0 SLCK=0 SLCK2=1 SPML=4 T1248=1 VCKSEL=1 VHP=0 XPCK=0

// ntsc progressive (SoTC)

CLKSEL=1 CMOD=0 EX=0 GCONT=0 LC=32 NVCK=1 PCK2=0 PEHS=0 PEVS=0 PHS=0 PRST=1 PVS=0 RC=4 SINT=0 SLCK=0 SLCK2=1 SPML=2 T1248=1 VCKSEL=1 VHP=1 XPCK=0

*/

REG64_(GSReg, SMODE2)
	uint32 INT   :  1;
	uint32 FFMD  :  1;
	uint32 DPMS  :  2;
	uint32 _PAD2 : 28;
	uint32 _PAD3 : 32;
REG_END

REG64_(GSReg, SRFSH)
	uint32 _DUMMY;
	// TODO
REG_END

REG64_(GSReg, SYNCH1)
	uint32 _DUMMY;
	// TODO
REG_END

REG64_(GSReg, SYNCH2)
	uint32 _DUMMY;
	// TODO
REG_END

REG64_(GSReg, SYNCV)
	uint32 VFP  : 10; // Vertical Front Porchinterval (?s)
	uint32 VFPE : 10; // Vertical Front Porchinterval End (?s)
	uint32 VBP  : 12; // Vertical Back Porchinterval (?s)
	uint32 VBPE : 10; // Vertical Back Porchinterval End (?s)
	uint32 VDP  : 11; // Vertical Differential Phase
	uint32 VS   : 11; // Vertical Synchronization Timing
REG_END

REG64_SET(GSReg)
	GSRegBGCOLOR  BGCOLOR;
	GSRegBUSDIR   BUSDIR;
	GSRegCSR      CSR;
	GSRegDISPFB   DISPFB;
	GSRegDISPLAY  DISPLAY;
	GSRegEXTBUF   EXTBUF;
	GSRegEXTDATA  EXTDATA;
	GSRegEXTWRITE EXTWRITE;
	GSRegIMR      IMR;
	GSRegPMODE    PMODE;
	GSRegSIGLBLID SIGLBLID;
	GSRegSMODE1   SMODE1;
	GSRegSMODE2   SMODE2;
REG_SET_END

//
// GIFTag

REG128(GIFTag)
	uint32 NLOOP : 15;
	uint32 EOP   :  1;
	uint32 _PAD1 : 16;
	uint32 _PAD2 : 14;
	uint32 PRE   :  1;
	uint32 PRIM  : 11;
	uint32 FLG   :  2; // enum GIF_FLG
	uint32 NREG  :  4;
	uint64 REGS;
REG_END

// GIFReg

REG64_(GIFReg, ALPHA)
	uint32 A     :  2;
	uint32 B     :  2;
	uint32 C     :  2;
	uint32 D     :  2;
	uint32 _PAD1 : 24;
	uint8 FIX;
	uint8 _PAD2[3];
REG_END2
	// opaque => output will be Cs/As
	__forceinline bool IsOpaque() const { return ((A == B || (C == 2 && FIX == 0)) && D == 0) || (A == 0 && B == D && C == 2 && FIX == 0x80); }
	__forceinline bool IsOpaque(int amin, int amax) const { return ((A == B || amax == 0) && D == 0) || (A == 0 && B == D && amin == 0x80 && amax == 0x80); }
	__forceinline bool IsCd() { return (A == B) && (D == 1); }
REG_END2

REG64_(GIFReg, BITBLTBUF)
	uint32 SBP   : 14;
	uint32 _PAD1 :  2;
	uint32 SBW   :  6;
	uint32 _PAD2 :  2;
	uint32 SPSM  :  6;
	uint32 _PAD3 :  2;
	uint32 DBP   : 14;
	uint32 _PAD4 :  2;
	uint32 DBW   :  6;
	uint32 _PAD5 :  2;
	uint32 DPSM  :  6;
	uint32 _PAD6 :  2;
REG_END

REG64_(GIFReg, CLAMP)
union
{
	struct
	{
		uint32 WMS   :  2;
		uint32 WMT   :  2;
		uint32 MINU  : 10;
		uint32 MAXU  : 10;
		uint32 _PAD1 :  8;
		uint32 _PAD2 :  2;
		uint32 MAXV  : 10;
		uint32 _PAD3 : 20;
	};

	struct
	{
		uint64 _PAD4 : 24;
		uint64 MINV  : 10;
		uint64 _PAD5 : 30;
	};
};
REG_END

REG64_(GIFReg, COLCLAMP)
	uint32 CLAMP :  1;
	uint32 _PAD1 : 31;
	uint32 _PAD2 : 32;
REG_END

REG64_(GIFReg, DIMX)
	int32   DM00 : 3;
	int32 _PAD00 : 1;
	int32   DM01 : 3;
	int32 _PAD01 : 1;
	int32   DM02 : 3;
	int32 _PAD02 : 1;
	int32   DM03 : 3;
	int32 _PAD03 : 1;
	int32   DM10 : 3;
	int32 _PAD10 : 1;
	int32   DM11 : 3;
	int32 _PAD11 : 1;
	int32   DM12 : 3;
	int32 _PAD12 : 1;
	int32   DM13 : 3;
	int32 _PAD13 : 1;
	int32   DM20 : 3;
	int32 _PAD20 : 1;
	int32   DM21 : 3;
	int32 _PAD21 : 1;
	int32   DM22 : 3;
	int32 _PAD22 : 1;
	int32   DM23 : 3;
	int32 _PAD23 : 1;
	int32   DM30 : 3;
	int32 _PAD30 : 1;
	int32   DM31 : 3;
	int32 _PAD31 : 1;
	int32   DM32 : 3;
	int32 _PAD32 : 1;
	int32   DM33 : 3;
	int32 _PAD33 : 1;
REG_END

REG64_(GIFReg, DTHE)
	uint32 DTHE  :  1;
	uint32 _PAD1 : 31;
	uint32 _PAD2 : 32;
REG_END

REG64_(GIFReg, FBA)
	uint32 FBA   :  1;
	uint32 _PAD1 : 31;
	uint32 _PAD2 : 32;
REG_END

REG64_(GIFReg, FINISH)
	uint32 _PAD1[2];
REG_END

REG64_(GIFReg, FOG)
	uint8 _PAD1[7];
	uint8 F;
REG_END

REG64_(GIFReg, FOGCOL)
	uint8 FCR;
	uint8 FCG;
	uint8 FCB;
	uint8 _PAD1[5];
REG_END

REG64_(GIFReg, FRAME)
	uint32 FBP   : 9;
	uint32 _PAD1 : 7;
	uint32 FBW   : 6;
	uint32 _PAD2 : 2;
	uint32 PSM   : 6;
	uint32 _PAD3 : 2;
	uint32 FBMSK;
REG_END2
	uint32 Block() const { return FBP << 5; }
REG_END2

REG64_(GIFReg, HWREG)
	uint32 DATA_LOWER;
	uint32 DATA_UPPER;
REG_END

REG64_(GIFReg, LABEL)
	uint32 ID;
	uint32 IDMSK;
REG_END

REG64_(GIFReg, MIPTBP1)
	uint64 TBP1 : 14;
	uint64 TBW1 :  6;
	uint64 TBP2 : 14;
	uint64 TBW2 :  6;
	uint64 TBP3 : 14;
	uint64 TBW3 :  6;
	uint64 _PAD :  4;
REG_END

REG64_(GIFReg, MIPTBP2)
	uint64 TBP4 : 14;
	uint64 TBW4 :  6;
	uint64 TBP5 : 14;
	uint64 TBW5 :  6;
	uint64 TBP6 : 14;
	uint64 TBW6 :  6;
	uint64 _PAD :  4;
REG_END

REG64_(GIFReg, NOP)
	uint32 _PAD[2];
REG_END

REG64_(GIFReg, PABE)
	uint32 PABE  :  1;
	uint32 _PAD1 : 31;
	uint32 _PAD2 : 32;
REG_END

REG64_(GIFReg, PRIM)
	uint32 PRIM  :  3;
	uint32 IIP   :  1;
	uint32 TME   :  1;
	uint32 FGE   :  1;
	uint32 ABE   :  1;
	uint32 AA1   :  1;
	uint32 FST   :  1;
	uint32 CTXT  :  1;
	uint32 FIX   :  1;
	uint32 _PAD1 : 21;
	uint32 _PAD2 : 32;
REG_END

REG64_(GIFReg, PRMODE)
	uint32 _PRIM :  3;
	uint32 IIP   :  1;
	uint32 TME   :  1;
	uint32 FGE   :  1;
	uint32 ABE   :  1;
	uint32 AA1   :  1;
	uint32 FST   :  1;
	uint32 CTXT  :  1;
	uint32 FIX   :  1;
	uint32 _PAD2 : 21;
	uint32 _PAD3 : 32;
REG_END

REG64_(GIFReg, PRMODECONT)
	uint32 AC    :  1;
	uint32 _PAD1 : 31;
	uint32 _PAD2 : 32;
REG_END

REG64_(GIFReg, RGBAQ)
	uint8 R;
	uint8 G;
	uint8 B;
	uint8 A;
	float Q;
REG_END

REG64_(GIFReg, SCANMSK)
	uint32 MSK   :  2;
	uint32 _PAD1 : 30;
	uint32 _PAD2 : 32;
REG_END

REG64_(GIFReg, SCISSOR)
	uint32 SCAX0 : 11;
	uint32 _PAD1 :  5;
	uint32 SCAX1 : 11;
	uint32 _PAD2 :  5;
	uint32 SCAY0 : 11;
	uint32 _PAD3 :  5;
	uint32 SCAY1 : 11;
	uint32 _PAD4 :  5;
REG_END

REG64_(GIFReg, SIGNAL)
	uint32 ID;
	uint32 IDMSK;
REG_END

REG64_(GIFReg, ST)
	float S;
	float T;
REG_END

REG64_(GIFReg, TEST)
	uint32 ATE   :  1;
	uint32 ATST  :  3;
	uint32 AREF  :  8;
	uint32 AFAIL :  2;
	uint32 DATE  :  1;
	uint32 DATM  :  1;
	uint32 ZTE   :  1;
	uint32 ZTST  :  2;
	uint32 _PAD1 : 13;
	uint32 _PAD2 : 32;
REG_END2
	__forceinline bool DoFirstPass() const { return !ATE || ATST != ATST_NEVER; } // not all pixels fail automatically
	__forceinline bool DoSecondPass() const { return ATE && ATST != ATST_ALWAYS && AFAIL != AFAIL_KEEP; } // pixels may fail, write fb/z
	__forceinline bool NoSecondPass() const { return ATE && ATST != ATST_ALWAYS && AFAIL == AFAIL_KEEP; } // pixels may fail, no output
REG_END2

REG64_(GIFReg, TEX0)
union
{
	struct
	{
		uint32 TBP0  : 14;
		uint32 TBW   :  6;
		uint32 PSM   :  6;
		uint32 TW    :  4;
		uint32 _PAD1 :  2;
		uint32 _PAD2 :  2;
		uint32 TCC   :  1;
		uint32 TFX   :  2;
		uint32 CBP   : 14;
		uint32 CPSM  :  4;
		uint32 CSM   :  1;
		uint32 CSA   :  5;
		uint32 CLD   :  3;
	};

	struct
	{
		uint64 _PAD3 : 30;
		uint64 TH    :  4;
		uint64 _PAD4 : 30;
	};
};
REG_END2
	__forceinline bool IsRepeating() const
	{
		if (TBW < 2)
		{
			if (PSM == PSM_PSMT8)
				return TW > 7 || TH > 6;
			if (PSM == PSM_PSMT4)
				return TW > 7 || TH > 7;
		}

		// The recast of TBW seems useless but it avoid tons of warning from GCC...
		return ((uint32)TBW << 6u) < (1u << TW);
	}
REG_END2

REG64_(GIFReg, TEX1)
	uint32 LCM   :  1;
	uint32 _PAD1 :  1;
	uint32 MXL   :  3;
	uint32 MMAG  :  1;
	uint32 MMIN  :  3;
	uint32 MTBA  :  1;
	uint32 _PAD2 :  9;
	uint32 L     :  2;
	uint32 _PAD3 : 11;
	int32  K     : 12; // 1:7:4
	uint32 _PAD4 : 20;
REG_END2
	bool IsMinLinear() const { return (MMIN == 1) || (MMIN & 4); }
	bool IsMagLinear() const { return MMAG; }
REG_END2

REG64_(GIFReg, TEX2)
	uint32 _PAD1 : 20;
	uint32 PSM   :  6;
	uint32 _PAD2 :  6;
	uint32 _PAD3 :  5;
	uint32 CBP   : 14;
	uint32 CPSM  :  4;
	uint32 CSM   :  1;
	uint32 CSA   :  5;
	uint32 CLD   :  3;
REG_END

REG64_(GIFReg, TEXA)
	uint8 TA0;
	uint8 _PAD1 : 7;
	uint8 AEM   : 1;
	uint16 _PAD2;
	uint8 TA1   : 8;
	uint8 _PAD3[3];
REG_END

REG64_(GIFReg, TEXCLUT)
	uint32 CBW   :  6;
	uint32 COU   :  6;
	uint32 COV   : 10;
	uint32 _PAD1 : 10;
	uint32 _PAD2 : 32;
REG_END

REG64_(GIFReg, TEXFLUSH)
	uint32 _PAD1 : 32;
	uint32 _PAD2 : 32;
REG_END

REG64_(GIFReg, TRXDIR)
	uint32 XDIR  :  2;
	uint32 _PAD1 : 30;
	uint32 _PAD2 : 32;
REG_END

REG64_(GIFReg, TRXPOS)
	uint32 SSAX  : 11;
	uint32 _PAD1 :  5;
	uint32 SSAY  : 11;
	uint32 _PAD2 :  5;
	uint32 DSAX  : 11;
	uint32 _PAD3 :  5;
	uint32 DSAY  : 11;
	uint32 DIRY  :  1;
	uint32 DIRX  :  1;
	uint32 _PAD4 :  3;
REG_END

REG64_(GIFReg, TRXREG)
	uint32 RRW   : 12;
	uint32 _PAD1 : 20;
	uint32 RRH   : 12;
	uint32 _PAD2 : 20;
REG_END

// GSState::GIFPackedRegHandlerUV and GSState::GIFRegHandlerUV will make sure that the _PAD1/2 bits are set to zero

REG64_(GIFReg, UV)
	uint16 U;
	// uint32 _PAD1 : 2;
	uint16 V;
	// uint32 _PAD2 : 2;
	uint32 _PAD3;
REG_END

// GSState::GIFRegHandlerXYOFFSET will make sure that the _PAD1/2 bits are set to zero

REG64_(GIFReg, XYOFFSET)
	uint32 OFX; // : 16; uint32 _PAD1 : 16;
	uint32 OFY; // : 16; uint32 _PAD2 : 16;
REG_END

REG64_(GIFReg, XYZ)
	uint16 X;
	uint16 Y;
	uint32 Z;
REG_END

REG64_(GIFReg, XYZF)
	uint16 X;
	uint16 Y;
	uint32 Z : 24;
	uint32 F :  8;
REG_END

REG64_(GIFReg, ZBUF)
	uint32 ZBP   :  9;
	uint32 _PAD1 : 15;
	// uint32 PSM   :  4;
	// uint32 _PAD2 :  4;
	uint32 PSM   :  6;
	uint32 _PAD2 :  2;
	uint32 ZMSK  :  1;
	uint32 _PAD3 : 31;
REG_END2
	uint32 Block() const { return ZBP << 5; }
REG_END2

REG64_SET(GIFReg)
	GIFRegALPHA      ALPHA;
	GIFRegBITBLTBUF  BITBLTBUF;
	GIFRegCLAMP      CLAMP;
	GIFRegCOLCLAMP   COLCLAMP;
	GIFRegDIMX       DIMX;
	GIFRegDTHE       DTHE;
	GIFRegFBA        FBA;
	GIFRegFINISH     FINISH;
	GIFRegFOG        FOG;
	GIFRegFOGCOL     FOGCOL;
	GIFRegFRAME      FRAME;
	GIFRegHWREG      HWREG;
	GIFRegLABEL      LABEL;
	GIFRegMIPTBP1    MIPTBP1;
	GIFRegMIPTBP2    MIPTBP2;
	GIFRegNOP        NOP;
	GIFRegPABE       PABE;
	GIFRegPRIM       PRIM;
	GIFRegPRMODE     PRMODE;
	GIFRegPRMODECONT PRMODECONT;
	GIFRegRGBAQ      RGBAQ;
	GIFRegSCANMSK    SCANMSK;
	GIFRegSCISSOR    SCISSOR;
	GIFRegSIGNAL     SIGNAL;
	GIFRegST         ST;
	GIFRegTEST       TEST;
	GIFRegTEX0       TEX0;
	GIFRegTEX1       TEX1;
	GIFRegTEX2       TEX2;
	GIFRegTEXA       TEXA;
	GIFRegTEXCLUT    TEXCLUT;
	GIFRegTEXFLUSH   TEXFLUSH;
	GIFRegTRXDIR     TRXDIR;
	GIFRegTRXPOS     TRXPOS;
	GIFRegTRXREG     TRXREG;
	GIFRegUV         UV;
	GIFRegXYOFFSET   XYOFFSET;
	GIFRegXYZ        XYZ;
	GIFRegXYZF       XYZF;
	GIFRegZBUF       ZBUF;
REG_SET_END

// GIFPacked

REG128_(GIFPacked, PRIM)
	uint32 PRIM  : 11;
	uint32 _PAD1 : 21;
	uint32 _PAD2[3];
REG_END

REG128_(GIFPacked, RGBA)
	uint8 R;
	uint8 _PAD1[3];
	uint8 G;
	uint8 _PAD2[3];
	uint8 B;
	uint8 _PAD3[3];
	uint8 A;
	uint8 _PAD4[3];
REG_END

REG128_(GIFPacked, STQ)
	float S;
	float T;
	float Q;
	uint32 _PAD1 : 32;
REG_END

REG128_(GIFPacked, UV)
	uint32 U     : 14;
	uint32 _PAD1 : 18;
	uint32 V     : 14;
	uint32 _PAD2 : 18;
	uint32 _PAD3 : 32;
	uint32 _PAD4 : 32;
REG_END

REG128_(GIFPacked, XYZF2)
	uint16 X;
	uint16 _PAD1;
	uint16 Y;
	uint16 _PAD2;
	uint32 _PAD3 :  4;
	uint32 Z     : 24;
	uint32 _PAD4 :  4;
	uint32 _PAD5 :  4;
	uint32 F     :  8;
	uint32 _PAD6 :  3;
	uint32 ADC   :  1;
	uint32 _PAD7 : 16;
REG_END2
uint32 Skip() const { return u32[3] & 0x8000; }
REG_END2

REG128_(GIFPacked, XYZ2)
	uint16 X;
	uint16 _PAD1;
	uint16 Y;
	uint16 _PAD2;
	uint32 Z;
	uint32 _PAD3 : 15;
	uint32 ADC   :  1;
	uint32 _PAD4 : 16;
REG_END2
	uint32 Skip() const { return u32[3] & 0x8000; }
REG_END2

REG128_(GIFPacked, FOG)
	uint32 _PAD1;
	uint32 _PAD2;
	uint32 _PAD3;
	uint32 _PAD4 :  4;
	uint32 F     :  8;
	uint32 _PAD5 : 20;
REG_END

REG128_(GIFPacked, A_D)
	uint64 DATA;
	uint8 ADDR : 8; // enum GIF_A_D_REG
	uint8 _PAD1[3 + 4];
REG_END

REG128_(GIFPacked, NOP)
	uint32 _PAD1;
	uint32 _PAD2;
	uint32 _PAD3;
	uint32 _PAD4;
REG_END

REG128_SET(GIFPackedReg)
	GIFReg          r;
	GIFPackedPRIM   PRIM;
	GIFPackedRGBA   RGBA;
	GIFPackedSTQ    STQ;
	GIFPackedUV     UV;
	GIFPackedXYZF2  XYZF2;
	GIFPackedXYZ2   XYZ2;
	GIFPackedFOG    FOG;
	GIFPackedA_D    A_D;
	GIFPackedNOP    NOP;
REG_SET_END

struct alignas(32) GIFPath
{
	GIFTag tag;
	uint32 nloop;
	uint32 nreg;
	uint32 reg;
	uint32 type;
	GSVector4i regs;

	enum
	{
		TYPE_UNKNOWN,
		TYPE_ADONLY,
		TYPE_STQRGBAXYZF2,
		TYPE_STQRGBAXYZ2
	};

	__forceinline void SetTag(const void* mem)
	{
		const GIFTag* RESTRICT src = (const GIFTag*)mem;

		// the compiler has a hard time not reloading every time a field of src is accessed

		uint32 a = src->u32[0];
		uint32 b = src->u32[1];

		tag.u32[0] = a;
		tag.u32[1] = b;

		nloop = a & 0x7fff;

		if (nloop == 0)
			return;

		GSVector4i v = GSVector4i::loadl(&src->REGS); // REGS not stored to tag.REGS, only into this->regs, restored before saving the state though

		nreg = (b & 0xf0000000) ? (b >> 28) : 16; // src->NREG
		regs = v.upl8(v >> 4) & GSVector4i::x0f(nreg);
		reg = 0;

		type = TYPE_UNKNOWN;

		if (tag.FLG == GIF_FLG_PACKED)
		{
			if (regs.eq8(GSVector4i(0x0e0e0e0e)).mask() == (1 << nreg) - 1)
			{
				type = TYPE_ADONLY;
			}
			else
			{
				switch (nreg)
				{
					case 1:
						break;
					case 2:
						break;
					case 3:
						// many games, TODO: formats mixed with NOPs (xeno2: 040f010f02, 04010f020f, mgs3: 04010f0f02, 0401020f0f, 04010f020f)
						if (regs.u32[0] == 0x00040102)
							type = TYPE_STQRGBAXYZF2;
						// GoW (has other crazy formats, like ...030503050103)
						if (regs.u32[0] == 0x00050102)
							type = TYPE_STQRGBAXYZ2;
						// TODO: common types with UV instead
						break;
					case 4:
						break;
					case 5:
						break;
					case 6:
						break;
					case 7:
						break;
					case 8:
						break;
					case 9:
						// ffx
						if (regs.u32[0] == 0x02040102 && regs.u32[1] == 0x01020401 && regs.u32[2] == 0x00000004)
						{
							type = TYPE_STQRGBAXYZF2;
							nreg = 3;
							nloop *= 3;
						}
						break;
					case 10:
						break;
					case 11:
						break;
					case 12:
						// dq8 (not many, mostly 040102)
						if (regs.u32[0] == 0x02040102 && regs.u32[1] == 0x01020401 && regs.u32[2] == 0x04010204)
						{
							type = TYPE_STQRGBAXYZF2;
							nreg = 3;
							nloop *= 4;
						}
						break;
					case 13:
						break;
					case 14:
						break;
					case 15:
						break;
					case 16:
						break;
					default:
						__assume(0);
				}
			}
		}
	}

	__forceinline uint8 GetReg() const
	{
		return regs.u8[reg];
	}

	__forceinline uint8 GetReg(uint32 index) const
	{
		return regs.u8[index];
	}

	__forceinline bool StepReg()
	{
		if (++reg == nreg)
		{
			reg = 0;

			if (--nloop == 0)
			{
				return false;
			}
		}

		return true;
	}
};

struct GSPrivRegSet
{
	union
	{
		struct
		{
			GSRegPMODE  PMODE;
			uint64      _pad1;
			GSRegSMODE1 SMODE1;
			uint64      _pad2;
			GSRegSMODE2 SMODE2;
			uint64      _pad3;
			GSRegSRFSH  SRFSH;
			uint64      _pad4;
			GSRegSYNCH1 SYNCH1;
			uint64      _pad5;
			GSRegSYNCH2 SYNCH2;
			uint64      _pad6;
			GSRegSYNCV  SYNCV;
			uint64      _pad7;
			struct
			{
				GSRegDISPFB  DISPFB;
				uint64       _pad1;
				GSRegDISPLAY DISPLAY;
				uint64       _pad2;
			} DISP[2];
			GSRegEXTBUF   EXTBUF;
			uint64        _pad8;
			GSRegEXTDATA  EXTDATA;
			uint64        _pad9;
			GSRegEXTWRITE EXTWRITE;
			uint64        _pad10;
			GSRegBGCOLOR  BGCOLOR;
			uint64        _pad11;
		};

		uint8 _pad12[0x1000];
	};

	union
	{
		struct
		{
			GSRegCSR      CSR;
			uint64        _pad13;
			GSRegIMR      IMR;
			uint64        _pad14;
			uint64        _unk1[4];
			GSRegBUSDIR   BUSDIR;
			uint64        _pad15;
			uint64        _unk2[6];
			GSRegSIGLBLID SIGLBLID;
			uint64        _pad16;
		};

		uint8 _pad17[0x1000];
	};

	void Dump(FILE* fp)
	{
		for (int i = 0; i < 2; i++)
		{
			if (!fp)
				return;

			if (i == 0 && !PMODE.EN1)
				continue;
			if (i == 1 && !PMODE.EN2)
				continue;

			fprintf(fp, "DISPFB[%d] BP=%05x BW=%u PSM=%u DBX=%u DBY=%u\n",
				i,
				DISP[i].DISPFB.Block(),
				DISP[i].DISPFB.FBW,
				DISP[i].DISPFB.PSM,
				DISP[i].DISPFB.DBX,
				DISP[i].DISPFB.DBY);

			fprintf(fp, "DISPLAY[%d] DX=%u DY=%u DW=%u DH=%u MAGH=%u MAGV=%u\n",
				i,
				DISP[i].DISPLAY.DX,
				DISP[i].DISPLAY.DY,
				DISP[i].DISPLAY.DW,
				DISP[i].DISPLAY.DH,
				DISP[i].DISPLAY.MAGH,
				DISP[i].DISPLAY.MAGV);
		}

		fprintf(fp, "PMODE EN1=%u EN2=%u CRTMD=%u MMOD=%u AMOD=%u SLBG=%u ALP=%u\n",
			PMODE.EN1,
			PMODE.EN2,
			PMODE.CRTMD,
			PMODE.MMOD,
			PMODE.AMOD,
			PMODE.SLBG,
			PMODE.ALP);

		fprintf(fp, "SMODE1 CLKSEL=%u CMOD=%u EX=%u GCONT=%u LC=%u NVCK=%u PCK2=%u PEHS=%u PEVS=%u PHS=%u PRST=%u PVS=%u RC=%u SINT=%u SLCK=%u SLCK2=%u SPML=%u T1248=%u VCKSEL=%u VHP=%u XPCK=%u\n",
			SMODE1.CLKSEL,
			SMODE1.CMOD,
			SMODE1.EX,
			SMODE1.GCONT,
			SMODE1.LC,
			SMODE1.NVCK,
			SMODE1.PCK2,
			SMODE1.PEHS,
			SMODE1.PEVS,
			SMODE1.PHS,
			SMODE1.PRST,
			SMODE1.PVS,
			SMODE1.RC,
			SMODE1.SINT,
			SMODE1.SLCK,
			SMODE1.SLCK2,
			SMODE1.SPML,
			SMODE1.T1248,
			SMODE1.VCKSEL,
			SMODE1.VHP,
			SMODE1.XPCK);

		fprintf(fp, "SMODE2 INT=%u FFMD=%u DPMS=%u\n",
			SMODE2.INT,
			SMODE2.FFMD,
			SMODE2.DPMS);

		fprintf(fp, "SRFSH %08x_%08x\n",
			SRFSH.u32[0],
			SRFSH.u32[1]);

		fprintf(fp, "SYNCH1 %08x_%08x\n",
			SYNCH1.u32[0],
			SYNCH1.u32[1]);

		fprintf(fp, "SYNCH2 %08x_%08x\n",
			SYNCH2.u32[0],
			SYNCH2.u32[1]);

		fprintf(fp, "SYNCV VBP=%u VBPE=%u VDP=%u VFP=%u VFPE=%u VS=%u\n",
			SYNCV.VBP,
			SYNCV.VBPE,
			SYNCV.VDP,
			SYNCV.VFP,
			SYNCV.VFPE,
			SYNCV.VS);

		fprintf(fp, "CSR %08x_%08x\n",
			CSR.u32[0],
			CSR.u32[1]);

		fprintf(fp, "BGCOLOR B=%u G=%u R=%u\n",
			BGCOLOR.B,
			BGCOLOR.G,
			BGCOLOR.R);

		fprintf(fp, "EXTBUF BP=0x%x BW=%u FBIN=%u WFFMD=%u EMODA=%u EMODC=%u WDX=%u WDY=%u\n",
			EXTBUF.EXBP, EXTBUF.EXBW, EXTBUF.FBIN, EXTBUF.WFFMD,
			EXTBUF.EMODA, EXTBUF.EMODC, EXTBUF.WDX, EXTBUF.WDY);

		fprintf(fp, "EXTDATA SX=%u SY=%u SMPH=%u SMPV=%u WW=%u WH=%u\n",
			EXTDATA.SX, EXTDATA.SY, EXTDATA.SMPH, EXTDATA.SMPV, EXTDATA.WW, EXTDATA.WH);

		fprintf(fp, "EXTWRITE EN=%u\n", EXTWRITE.WRITE);
	}

	void Dump(const std::string& filename)
	{
		FILE* fp = fopen(filename.c_str(), "wt");
		if (fp)
		{
			Dump(fp);
			fclose(fp);
		}
	}
};

#pragma pack(pop)

// ST_WRITE is defined in libc, avoid this
enum stateType
{
	SAVE_WRITE,
	SAVE_TRANSFER,
	SAVE_VSYNC
};

enum class GSVideoMode : uint8
{
	Unknown,
	NTSC,
	PAL,
	VESA,
	SDTV_480P,
	HDTV_720P,
	HDTV_1080I
};

extern Pcsx2Config::GSOptions GSConfig;

struct HostKeyEvent;
class HostDisplay;

#ifdef ENABLE_ACCURATE_BUFFER_EMULATION
const GSVector2i default_rt_size(2048, 2048);
#else
const GSVector2i default_rt_size(1280, 1024);
#endif

int GSinit();
void GSshutdown();
bool GSopen(const Pcsx2Config::GSOptions& config, GSRendererType renderer, u8* basemem);
void GSreset();
void GSclose();
void GSgifSoftReset(uint32 mask);
void GSwriteCSR(uint32 csr);
void GSinitReadFIFO(uint8* mem);
void GSreadFIFO(uint8* mem);
void GSinitReadFIFO2(uint8* mem, uint32 size);
void GSreadFIFO2(uint8* mem, uint32 size);
void GSgifTransfer(const uint8* mem, uint32 size);
void GSgifTransfer1(uint8* mem, uint32 addr);
void GSgifTransfer2(uint8* mem, uint32 size);
void GSgifTransfer3(uint8* mem, uint32 size);
void GSvsync(int field);
uint32 GSmakeSnapshot(char* path);
void GSkeyEvent(const HostKeyEvent& e);
int GSfreeze(FreezeAction mode, freezeData* data);
void GSconfigure();
int GStest();
bool GSsetupRecording(std::string& filename);
void GSendRecording();
void GSsetGameCRC(uint32 crc, int options);
void GSsetFrameSkip(int frameskip);

void GSgetInternalResolution(int* width, int* height);
void GSgetStats(std::string& info);

GSRendererType GSGetBestRenderer();
void GSUpdateConfig(const Pcsx2Config::GSOptions& new_config);
void GSSwitchRenderer(GSRendererType new_renderer);
void GSResetAPIState();
void GSRestoreAPIState();
bool GSSaveSnapshotToMemory(uint32 width, uint32 height, std::vector<uint32>* pixels);

#ifndef PCSX2_CORE
std::string GSGetConfigString(const char* key);
void GSLoadConfigFromApp(Pcsx2Config::GSOptions* config);

// Needed for window resizing in wx. Can be safely called from the UI thread.
void GSResizeWindow(int width, int height);
bool GSCheckForWindowResize(int* new_width, int* new_height);
#endif

class GSApp
{
	std::string m_section;
	std::map<std::string, std::string> m_default_configuration;
	std::map<std::string, std::string> m_configuration_map;

public:
	std::string m_ini;
	GSApp();

	void Init();

#ifndef PCSX2_CORE
	void BuildConfigurationMap(const char* lpFileName);
	void ReloadConfig();
	int GetIniInt(const char* lpAppName, const char* lpKeyName, int nDefault, const char* lpFileName);
#endif

	size_t GetIniString(const char* lpAppName, const char* lpKeyName, const char* lpDefault, char* lpReturnedString, size_t nSize, const char* lpFileName);
	bool WriteIniString(const char* lpAppName, const char* lpKeyName, const char* pString, const char* lpFileName);

	void SetConfig(const char* entry, const char* value);
	void SetConfig(const char* entry, int value);
	// Avoid issue with overloading
	template <typename T>
	T GetConfigT(const char* entry)
	{
		return static_cast<T>(GetConfigI(entry));
	}
	int GetConfigI(const char* entry);
	bool GetConfigB(const char* entry);
	std::string GetConfigS(const char* entry);

	void SetConfigDir();

	std::vector<GSSetting> m_gs_renderers;
	std::vector<GSSetting> m_gs_interlace;
	std::vector<GSSetting> m_gs_upscale_multiplier;
	std::vector<GSSetting> m_gs_max_anisotropy;
	std::vector<GSSetting> m_gs_dithering;
	std::vector<GSSetting> m_gs_bifilter;
	std::vector<GSSetting> m_gs_trifilter;
	std::vector<GSSetting> m_gs_hack;
	std::vector<GSSetting> m_gs_generic_list;
	std::vector<GSSetting> m_gs_offset_hack;
	std::vector<GSSetting> m_gs_hw_mipmapping;
	std::vector<GSSetting> m_gs_crc_level;
	std::vector<GSSetting> m_gs_acc_blend_level;
	std::vector<GSSetting> m_gs_acc_blend_level_d3d11;
	std::vector<GSSetting> m_gs_tv_shaders;
};

struct GSError
{
};
struct GSRecoverableError : GSError
{
};
struct GSErrorGlVertexArrayTooSmall : GSError
{
};

extern GSApp theApp;
