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
#include "GSUtil.h"
#include <locale>
#include <codecvt>

#ifdef _WIN32
#include <VersionHelpers.h>
#include "svnrev.h"
#include <wil/com.h>
#else
#define SVN_REV 0
#define SVN_MODS 0
#endif

#if defined(_M_X86_32) || defined(_M_X86_64)
Xbyak::util::Cpu g_cpu;
#endif

static class GSUtilMaps
{
public:
	uint8 PrimClassField[8];
	uint8 VertexCountField[8];
	uint8 ClassVertexCountField[4];
	uint32 CompatibleBitsField[64][2];
	uint32 SharedBitsField[64][2];

	// Defer init to avoid AVX2 illegal instructions
	void Init()
	{
		PrimClassField[GS_POINTLIST] = GS_POINT_CLASS;
		PrimClassField[GS_LINELIST] = GS_LINE_CLASS;
		PrimClassField[GS_LINESTRIP] = GS_LINE_CLASS;
		PrimClassField[GS_TRIANGLELIST] = GS_TRIANGLE_CLASS;
		PrimClassField[GS_TRIANGLESTRIP] = GS_TRIANGLE_CLASS;
		PrimClassField[GS_TRIANGLEFAN] = GS_TRIANGLE_CLASS;
		PrimClassField[GS_SPRITE] = GS_SPRITE_CLASS;
		PrimClassField[GS_INVALID] = GS_INVALID_CLASS;

		VertexCountField[GS_POINTLIST] = 1;
		VertexCountField[GS_LINELIST] = 2;
		VertexCountField[GS_LINESTRIP] = 2;
		VertexCountField[GS_TRIANGLELIST] = 3;
		VertexCountField[GS_TRIANGLESTRIP] = 3;
		VertexCountField[GS_TRIANGLEFAN] = 3;
		VertexCountField[GS_SPRITE] = 2;
		VertexCountField[GS_INVALID] = 1;

		ClassVertexCountField[GS_POINT_CLASS] = 1;
		ClassVertexCountField[GS_LINE_CLASS] = 2;
		ClassVertexCountField[GS_TRIANGLE_CLASS] = 3;
		ClassVertexCountField[GS_SPRITE_CLASS] = 2;

		memset(CompatibleBitsField, 0, sizeof(CompatibleBitsField));

		for (int i = 0; i < 64; i++)
		{
			CompatibleBitsField[i][i >> 5] |= 1 << (i & 0x1f);
		}

		CompatibleBitsField[PSM_PSMCT32][PSM_PSMCT24 >> 5] |= 1 << (PSM_PSMCT24 & 0x1f);
		CompatibleBitsField[PSM_PSMCT24][PSM_PSMCT32 >> 5] |= 1 << (PSM_PSMCT32 & 0x1f);
		CompatibleBitsField[PSM_PSMCT16][PSM_PSMCT16S >> 5] |= 1 << (PSM_PSMCT16S & 0x1f);
		CompatibleBitsField[PSM_PSMCT16S][PSM_PSMCT16 >> 5] |= 1 << (PSM_PSMCT16 & 0x1f);
		CompatibleBitsField[PSM_PSMZ32][PSM_PSMZ24 >> 5] |= 1 << (PSM_PSMZ24 & 0x1f);
		CompatibleBitsField[PSM_PSMZ24][PSM_PSMZ32 >> 5] |= 1 << (PSM_PSMZ32 & 0x1f);
		CompatibleBitsField[PSM_PSMZ16][PSM_PSMZ16S >> 5] |= 1 << (PSM_PSMZ16S & 0x1f);
		CompatibleBitsField[PSM_PSMZ16S][PSM_PSMZ16 >> 5] |= 1 << (PSM_PSMZ16 & 0x1f);

		memset(SharedBitsField, 0, sizeof(SharedBitsField));

		SharedBitsField[PSM_PSMCT24][PSM_PSMT8H >> 5] |= 1 << (PSM_PSMT8H & 0x1f);
		SharedBitsField[PSM_PSMCT24][PSM_PSMT4HL >> 5] |= 1 << (PSM_PSMT4HL & 0x1f);
		SharedBitsField[PSM_PSMCT24][PSM_PSMT4HH >> 5] |= 1 << (PSM_PSMT4HH & 0x1f);
		SharedBitsField[PSM_PSMZ24][PSM_PSMT8H >> 5] |= 1 << (PSM_PSMT8H & 0x1f);
		SharedBitsField[PSM_PSMZ24][PSM_PSMT4HL >> 5] |= 1 << (PSM_PSMT4HL & 0x1f);
		SharedBitsField[PSM_PSMZ24][PSM_PSMT4HH >> 5] |= 1 << (PSM_PSMT4HH & 0x1f);
		SharedBitsField[PSM_PSMT8H][PSM_PSMCT24 >> 5] |= 1 << (PSM_PSMCT24 & 0x1f);
		SharedBitsField[PSM_PSMT8H][PSM_PSMZ24 >> 5] |= 1 << (PSM_PSMZ24 & 0x1f);
		SharedBitsField[PSM_PSMT4HL][PSM_PSMCT24 >> 5] |= 1 << (PSM_PSMCT24 & 0x1f);
		SharedBitsField[PSM_PSMT4HL][PSM_PSMZ24 >> 5] |= 1 << (PSM_PSMZ24 & 0x1f);
		SharedBitsField[PSM_PSMT4HL][PSM_PSMT4HH >> 5] |= 1 << (PSM_PSMT4HH & 0x1f);
		SharedBitsField[PSM_PSMT4HH][PSM_PSMCT24 >> 5] |= 1 << (PSM_PSMCT24 & 0x1f);
		SharedBitsField[PSM_PSMT4HH][PSM_PSMZ24 >> 5] |= 1 << (PSM_PSMZ24 & 0x1f);
		SharedBitsField[PSM_PSMT4HH][PSM_PSMT4HL >> 5] |= 1 << (PSM_PSMT4HL & 0x1f);
	}

} s_maps;

void GSUtil::Init()
{
	s_maps.Init();
}

GS_PRIM_CLASS GSUtil::GetPrimClass(uint32 prim)
{
	return (GS_PRIM_CLASS)s_maps.PrimClassField[prim];
}

int GSUtil::GetVertexCount(uint32 prim)
{
	return s_maps.VertexCountField[prim];
}

int GSUtil::GetClassVertexCount(uint32 primclass)
{
	return s_maps.ClassVertexCountField[primclass];
}

const uint32* GSUtil::HasSharedBitsPtr(uint32 dpsm)
{
	return s_maps.SharedBitsField[dpsm];
}

bool GSUtil::HasSharedBits(uint32 spsm, const uint32* RESTRICT ptr)
{
	return (ptr[spsm >> 5] & (1 << (spsm & 0x1f))) == 0;
}

bool GSUtil::HasSharedBits(uint32 spsm, uint32 dpsm)
{
	return (s_maps.SharedBitsField[dpsm][spsm >> 5] & (1 << (spsm & 0x1f))) == 0;
}

bool GSUtil::HasSharedBits(uint32 sbp, uint32 spsm, uint32 dbp, uint32 dpsm)
{
	return ((sbp ^ dbp) | (s_maps.SharedBitsField[dpsm][spsm >> 5] & (1 << (spsm & 0x1f)))) == 0;
}

bool GSUtil::HasCompatibleBits(uint32 spsm, uint32 dpsm)
{
	return (s_maps.CompatibleBitsField[spsm][dpsm >> 5] & (1 << (dpsm & 0x1f))) != 0;
}

bool GSUtil::CheckSSE()
{
	bool status = true;

#if defined(_M_X86_32) || defined(_M_X86_64)
	struct ISA
	{
		Xbyak::util::Cpu::Type type;
		const char* name;
	};

	ISA checks[] = {
		{Xbyak::util::Cpu::tSSE41, "SSE41"},
#if _M_SSE >= 0x500
		{Xbyak::util::Cpu::tAVX, "AVX1"},
#endif
#if _M_SSE >= 0x501
		{Xbyak::util::Cpu::tAVX2, "AVX2"},
		{Xbyak::util::Cpu::tBMI1, "BMI1"},
		{Xbyak::util::Cpu::tBMI2, "BMI2"},
#endif
	};

	for (size_t i = 0; i < countof(checks); i++)
	{
		if (!g_cpu.has(checks[i].type))
		{
			fprintf(stderr, "This CPU does not support %s\n", checks[i].name);

			status = false;
		}
	}
#endif

	return status;
}

CRCHackLevel GSUtil::GetRecommendedCRCHackLevel(GSRendererType type)
{
	return type == GSRendererType::OGL ? CRCHackLevel::Partial : CRCHackLevel::Full;
}

#ifdef _WIN32

#include "GS/Renderers/DX11/D3D.h"

GSRendererType GSGetBestRenderer()
{
	return D3D::ShouldPreferD3D() ? GSRendererType::DX11 : GSRendererType::OGL;
}

#else

GSRendererType GSGetBestRenderer()
{
	return GSRendererType::OGL;
}

#endif

#ifdef _WIN32
void GSmkdir(const wchar_t* dir)
{
	if (!CreateDirectory(dir, nullptr))
	{
		DWORD errorID = ::GetLastError();
		if (errorID != ERROR_ALREADY_EXISTS)
		{
			fprintf(stderr, "Failed to create directory: %ls error %u\n", dir, errorID);
		}
	}
#else
void GSmkdir(const char* dir)
{
	int err = mkdir(dir, 0777);
	if (!err && errno != EEXIST)
		fprintf(stderr, "Failed to create directory: %s\n", dir);
#endif
}

std::string GStempdir()
{
#ifdef _WIN32
	wchar_t path[MAX_PATH + 1];
	GetTempPath(MAX_PATH, path);
	return convert_utf16_to_utf8(path);
#else
	return "/tmp";
#endif
}

const char* psm_str(int psm)
{
	switch (psm)
	{
		// Normal color
		case PSM_PSMCT32:  return "C_32";
		case PSM_PSMCT24:  return "C_24";
		case PSM_PSMCT16:  return "C_16";
		case PSM_PSMCT16S: return "C_16S";

		// Palette color
		case PSM_PSMT8:    return "P_8";
		case PSM_PSMT4:    return "P_4";
		case PSM_PSMT8H:   return "P_8H";
		case PSM_PSMT4HL:  return "P_4HL";
		case PSM_PSMT4HH:  return "P_4HH";

		// Depth
		case PSM_PSMZ32:   return "Z_32";
		case PSM_PSMZ24:   return "Z_24";
		case PSM_PSMZ16:   return "Z_16";
		case PSM_PSMZ16S:  return "Z_16S";

		case PSM_PSGPU24:  return "PS24";

		default:break;
	}
	return "BAD_PSM";
}
