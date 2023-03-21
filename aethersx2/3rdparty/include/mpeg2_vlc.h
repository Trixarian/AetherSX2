/*
 * vlc.h
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 * Modified by Florin for PCSX2 emu
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

// NOTE: While part of this header is originally from libmpeg2, which is GPL-licensed,
// it's not substantial and does not contain any functions, therefore can be argued
// not to be a derived work. See http://lkml.iu.edu/hypermail/linux/kernel/0301.1/0362.html
// The constants themselves can also be argued to be part of the MPEG-2 standard, whose
// patents expired worldwide in Feb 2020.

#pragma once
#include <cstdint>

#ifdef _MSC_VER
#define VLC_ALIGNED16 __declspec(align(16))
#else
#define VLC_ALIGNED16 __attribute__((aligned(16)))
#endif

enum macroblock_modes
{
	MACROBLOCK_INTRA = 1,
	MACROBLOCK_PATTERN = 2,
	MACROBLOCK_MOTION_BACKWARD = 4,
	MACROBLOCK_MOTION_FORWARD = 8,
	MACROBLOCK_QUANT = 16,
	DCT_TYPE_INTERLACED = 32
};

enum motion_type
{
	MOTION_TYPE_SHIFT = 6,
	MOTION_TYPE_MASK = (3 * 64),
	MOTION_TYPE_BASE = 64,
	MC_FIELD = (1 * 64),
	MC_FRAME = (2 * 64),
	MC_16X8 = (2 * 64),
	MC_DMV = (3 * 64)
};

/* picture structure */
enum picture_structure
{
	TOP_FIELD = 1,
	BOTTOM_FIELD = 2,
	FRAME_PICTURE = 3
};

/* picture coding type */
enum picture_coding_type
{
	I_TYPE = 1,
	P_TYPE = 2,
	B_TYPE = 3,
	D_TYPE = 4
};

struct MBtab
{
	std::uint8_t modes;
	std::uint8_t len;
};

struct MVtab
{
	std::uint8_t delta;
	std::uint8_t len;
};

struct DMVtab
{
	std::int8_t dmv;
	std::uint8_t len;
};

struct CBPtab
{
	std::uint8_t cbp;
	std::uint8_t len;
};

struct DCtab
{
	std::uint8_t size;
	std::uint8_t len;
};

struct DCTtab
{
	std::uint8_t run;
	std::uint8_t level;
	std::uint8_t len;
};

struct MBAtab
{
	std::uint8_t mba;
	std::uint8_t len;
};


#define INTRA MACROBLOCK_INTRA
#define QUANT MACROBLOCK_QUANT

static constexpr MBtab MB_I[] = {
	{INTRA | QUANT, 2}, {INTRA, 1}};

#define MC MACROBLOCK_MOTION_FORWARD
#define CODED MACROBLOCK_PATTERN

static constexpr VLC_ALIGNED16 MBtab MB_P[] = {
	{INTRA | QUANT, 6}, {CODED | QUANT, 5}, {MC | CODED | QUANT, 5}, {INTRA, 5},
	{MC, 3}, {MC, 3}, {MC, 3}, {MC, 3},
	{CODED, 2}, {CODED, 2}, {CODED, 2}, {CODED, 2},
	{CODED, 2}, {CODED, 2}, {CODED, 2}, {CODED, 2},
	{MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1},
	{MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1},
	{MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1},
	{MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1}, {MC | CODED, 1}};

#define FWD MACROBLOCK_MOTION_FORWARD
#define BWD MACROBLOCK_MOTION_BACKWARD
#define INTER MACROBLOCK_MOTION_FORWARD | MACROBLOCK_MOTION_BACKWARD

static constexpr VLC_ALIGNED16 MBtab MB_B[] = {
	{0, 0}, {INTRA | QUANT, 6},
	{BWD | CODED | QUANT, 6}, {FWD | CODED | QUANT, 6},
	{INTER | CODED | QUANT, 5}, {INTER | CODED | QUANT, 5},
	{INTRA, 5}, {INTRA, 5},
	{FWD, 4}, {FWD, 4}, {FWD, 4}, {FWD, 4},
	{FWD | CODED, 4}, {FWD | CODED, 4}, {FWD | CODED, 4}, {FWD | CODED, 4},
	{BWD, 3}, {BWD, 3}, {BWD, 3}, {BWD, 3},
	{BWD, 3}, {BWD, 3}, {BWD, 3}, {BWD, 3},
	{BWD | CODED, 3}, {BWD | CODED, 3}, {BWD | CODED, 3}, {BWD | CODED, 3},
	{BWD | CODED, 3}, {BWD | CODED, 3}, {BWD | CODED, 3}, {BWD | CODED, 3},
	{INTER, 2}, {INTER, 2}, {INTER, 2}, {INTER, 2},
	{INTER, 2}, {INTER, 2}, {INTER, 2}, {INTER, 2},
	{INTER, 2}, {INTER, 2}, {INTER, 2}, {INTER, 2},
	{INTER, 2}, {INTER, 2}, {INTER, 2}, {INTER, 2},
	{INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2},
	{INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2},
	{INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2},
	{INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2}, {INTER | CODED, 2}};

#undef INTRA
#undef QUANT
#undef MC
#undef CODED
#undef FWD
#undef BWD
#undef INTER


static constexpr MVtab MV_4[] = {
	{3, 6}, {2, 4}, {1, 3}, {1, 3}, {0, 2}, {0, 2}, {0, 2}, {0, 2}};

static constexpr VLC_ALIGNED16 MVtab MV_10[] = {
	{0, 10}, {0, 10}, {0, 10}, {0, 10}, {0, 10}, {0, 10}, {0, 10}, {0, 10},
	{0, 10}, {0, 10}, {0, 10}, {0, 10}, {15, 10}, {14, 10}, {13, 10}, {12, 10},
	{11, 10}, {10, 10}, {9, 9}, {9, 9}, {8, 9}, {8, 9}, {7, 9}, {7, 9},
	{6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7}, {6, 7},
	{5, 7}, {5, 7}, {5, 7}, {5, 7}, {5, 7}, {5, 7}, {5, 7}, {5, 7},
	{4, 7}, {4, 7}, {4, 7}, {4, 7}, {4, 7}, {4, 7}, {4, 7}, {4, 7}};


static constexpr DMVtab DMV_2[] = {
	{0, 1}, {0, 1}, {1, 2}, {-1, 2}};


static constexpr VLC_ALIGNED16 CBPtab CBP_7[] = {
	{0x22, 7}, {0x12, 7}, {0x0a, 7}, {0x06, 7},
	{0x21, 7}, {0x11, 7}, {0x09, 7}, {0x05, 7},
	{0x3f, 6}, {0x3f, 6}, {0x03, 6}, {0x03, 6},
	{0x24, 6}, {0x24, 6}, {0x18, 6}, {0x18, 6},
	{0x3e, 5}, {0x3e, 5}, {0x3e, 5}, {0x3e, 5},
	{0x02, 5}, {0x02, 5}, {0x02, 5}, {0x02, 5},
	{0x3d, 5}, {0x3d, 5}, {0x3d, 5}, {0x3d, 5},
	{0x01, 5}, {0x01, 5}, {0x01, 5}, {0x01, 5},
	{0x38, 5}, {0x38, 5}, {0x38, 5}, {0x38, 5},
	{0x34, 5}, {0x34, 5}, {0x34, 5}, {0x34, 5},
	{0x2c, 5}, {0x2c, 5}, {0x2c, 5}, {0x2c, 5},
	{0x1c, 5}, {0x1c, 5}, {0x1c, 5}, {0x1c, 5},
	{0x28, 5}, {0x28, 5}, {0x28, 5}, {0x28, 5},
	{0x14, 5}, {0x14, 5}, {0x14, 5}, {0x14, 5},
	{0x30, 5}, {0x30, 5}, {0x30, 5}, {0x30, 5},
	{0x0c, 5}, {0x0c, 5}, {0x0c, 5}, {0x0c, 5},
	{0x20, 4}, {0x20, 4}, {0x20, 4}, {0x20, 4},
	{0x20, 4}, {0x20, 4}, {0x20, 4}, {0x20, 4},
	{0x10, 4}, {0x10, 4}, {0x10, 4}, {0x10, 4},
	{0x10, 4}, {0x10, 4}, {0x10, 4}, {0x10, 4},
	{0x08, 4}, {0x08, 4}, {0x08, 4}, {0x08, 4},
	{0x08, 4}, {0x08, 4}, {0x08, 4}, {0x08, 4},
	{0x04, 4}, {0x04, 4}, {0x04, 4}, {0x04, 4},
	{0x04, 4}, {0x04, 4}, {0x04, 4}, {0x04, 4},
	{0x3c, 3}, {0x3c, 3}, {0x3c, 3}, {0x3c, 3},
	{0x3c, 3}, {0x3c, 3}, {0x3c, 3}, {0x3c, 3},
	{0x3c, 3}, {0x3c, 3}, {0x3c, 3}, {0x3c, 3},
	{0x3c, 3}, {0x3c, 3}, {0x3c, 3}, {0x3c, 3}};

static constexpr VLC_ALIGNED16 CBPtab CBP_9[] = {
	{0, 0}, {0x00, 9}, {0x27, 9}, {0x1b, 9},
	{0x3b, 9}, {0x37, 9}, {0x2f, 9}, {0x1f, 9},
	{0x3a, 8}, {0x3a, 8}, {0x36, 8}, {0x36, 8},
	{0x2e, 8}, {0x2e, 8}, {0x1e, 8}, {0x1e, 8},
	{0x39, 8}, {0x39, 8}, {0x35, 8}, {0x35, 8},
	{0x2d, 8}, {0x2d, 8}, {0x1d, 8}, {0x1d, 8},
	{0x26, 8}, {0x26, 8}, {0x1a, 8}, {0x1a, 8},
	{0x25, 8}, {0x25, 8}, {0x19, 8}, {0x19, 8},
	{0x2b, 8}, {0x2b, 8}, {0x17, 8}, {0x17, 8},
	{0x33, 8}, {0x33, 8}, {0x0f, 8}, {0x0f, 8},
	{0x2a, 8}, {0x2a, 8}, {0x16, 8}, {0x16, 8},
	{0x32, 8}, {0x32, 8}, {0x0e, 8}, {0x0e, 8},
	{0x29, 8}, {0x29, 8}, {0x15, 8}, {0x15, 8},
	{0x31, 8}, {0x31, 8}, {0x0d, 8}, {0x0d, 8},
	{0x23, 8}, {0x23, 8}, {0x13, 8}, {0x13, 8},
	{0x0b, 8}, {0x0b, 8}, {0x07, 8}, {0x07, 8}};

struct MBAtabSet
{
	MBAtab mba5[30];
	MBAtab mba11[26 * 4];
};
static constexpr VLC_ALIGNED16 MBAtabSet MBA = {
	{// mba5
		{6, 5}, {5, 5}, {4, 4}, {4, 4}, {3, 4}, {3, 4},
		{2, 3}, {2, 3}, {2, 3}, {2, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
		{0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
		{0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}},

	{// mba11
		{32, 11}, {31, 11}, {30, 11}, {29, 11},
		{28, 11}, {27, 11}, {26, 11}, {25, 11},
		{24, 11}, {23, 11}, {22, 11}, {21, 11},
		{20, 10}, {20, 10}, {19, 10}, {19, 10},
		{18, 10}, {18, 10}, {17, 10}, {17, 10},
		{16, 10}, {16, 10}, {15, 10}, {15, 10},
		{14, 8}, {14, 8}, {14, 8}, {14, 8},
		{14, 8}, {14, 8}, {14, 8}, {14, 8},
		{13, 8}, {13, 8}, {13, 8}, {13, 8},
		{13, 8}, {13, 8}, {13, 8}, {13, 8},
		{12, 8}, {12, 8}, {12, 8}, {12, 8},
		{12, 8}, {12, 8}, {12, 8}, {12, 8},
		{11, 8}, {11, 8}, {11, 8}, {11, 8},
		{11, 8}, {11, 8}, {11, 8}, {11, 8},
		{10, 8}, {10, 8}, {10, 8}, {10, 8},
		{10, 8}, {10, 8}, {10, 8}, {10, 8},
		{9, 8}, {9, 8}, {9, 8}, {9, 8},
		{9, 8}, {9, 8}, {9, 8}, {9, 8},
		{8, 7}, {8, 7}, {8, 7}, {8, 7},
		{8, 7}, {8, 7}, {8, 7}, {8, 7},
		{8, 7}, {8, 7}, {8, 7}, {8, 7},
		{8, 7}, {8, 7}, {8, 7}, {8, 7},
		{7, 7}, {7, 7}, {7, 7}, {7, 7},
		{7, 7}, {7, 7}, {7, 7}, {7, 7},
		{7, 7}, {7, 7}, {7, 7}, {7, 7},
		{7, 7}, {7, 7}, {7, 7}, {7, 7}}};

struct DCtabSet
{
	DCtab lum0[32]; // Table B-12, dct_dc_size_luminance, codes 00xxx ... 11110
	DCtab lum1[16]; // Table B-12, dct_dc_size_luminance, codes 111110xxx ... 111111111
	DCtab chrom0[32]; // Table B-13, dct_dc_size_chrominance, codes 00xxx ... 11110
	DCtab chrom1[32]; // Table B-13, dct_dc_size_chrominance, codes 111110xxxx ... 1111111111
};

static constexpr VLC_ALIGNED16 DCtabSet DCtable =
	{
		// lum0: Table B-12, dct_dc_size_luminance, codes 00xxx ... 11110 */
		{{1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
			{2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
			{0, 3}, {0, 3}, {0, 3}, {0, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3},
			{4, 3}, {4, 3}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {6, 5}, {0, 0}},

		/* lum1: Table B-12, dct_dc_size_luminance, codes 111110xxx ... 111111111 */
		{{7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6},
			{8, 7}, {8, 7}, {8, 7}, {8, 7}, {9, 8}, {9, 8}, {10, 9}, {11, 9}},

		/* chrom0: Table B-13, dct_dc_size_chrominance, codes 00xxx ... 11110 */
		{{0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
			{1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
			{2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
			{3, 3}, {3, 3}, {3, 3}, {3, 3}, {4, 4}, {4, 4}, {5, 5}, {0, 0}},

		/* chrom1: Table B-13, dct_dc_size_chrominance, codes 111110xxxx ... 1111111111 */
		{{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6},
			{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6},
			{7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7},
			{8, 8}, {8, 8}, {8, 8}, {8, 8}, {9, 9}, {9, 9}, {10, 10}, {11, 10}},
};

struct DCTtabSet
{
	DCTtab first[12];
	DCTtab next[12];

	DCTtab tab0[60];
	DCTtab tab0a[252];
	DCTtab tab1[8];
	DCTtab tab1a[8];

	DCTtab tab2[16];
	DCTtab tab3[16];
	DCTtab tab4[16];
	DCTtab tab5[16];
	DCTtab tab6[16];
};

static constexpr VLC_ALIGNED16 DCTtabSet DCT =
	{
		/* first[12]: Table B-14, DCT coefficients table zero,
	 * codes 0100 ... 1xxx (used for first (DC) coefficient)
	 */
		{{0, 2, 4}, {2, 1, 4}, {1, 1, 3}, {1, 1, 3},
			{0, 1, 1}, {0, 1, 1}, {0, 1, 1}, {0, 1, 1},
			{0, 1, 1}, {0, 1, 1}, {0, 1, 1}, {0, 1, 1}},

		/* next[12]: Table B-14, DCT coefficients table zero,
	 * codes 0100 ... 1xxx (used for all other coefficients)
	 */
		{{0, 2, 4}, {2, 1, 4}, {1, 1, 3}, {1, 1, 3},
			{64, 0, 2}, {64, 0, 2}, {64, 0, 2}, {64, 0, 2}, /* EOB */
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2}},

		/* tab0[60]: Table B-14, DCT coefficients table zero,
	 * codes 000001xx ... 00111xxx
	 */
		{{65, 0, 6}, {65, 0, 6}, {65, 0, 6}, {65, 0, 6}, /* Escape */
			{2, 2, 7}, {2, 2, 7}, {9, 1, 7}, {9, 1, 7},
			{0, 4, 7}, {0, 4, 7}, {8, 1, 7}, {8, 1, 7},
			{7, 1, 6}, {7, 1, 6}, {7, 1, 6}, {7, 1, 6},
			{6, 1, 6}, {6, 1, 6}, {6, 1, 6}, {6, 1, 6},
			{1, 2, 6}, {1, 2, 6}, {1, 2, 6}, {1, 2, 6},
			{5, 1, 6}, {5, 1, 6}, {5, 1, 6}, {5, 1, 6},
			{13, 1, 8}, {0, 6, 8}, {12, 1, 8}, {11, 1, 8},
			{3, 2, 8}, {1, 3, 8}, {0, 5, 8}, {10, 1, 8},
			{0, 3, 5}, {0, 3, 5}, {0, 3, 5}, {0, 3, 5},
			{0, 3, 5}, {0, 3, 5}, {0, 3, 5}, {0, 3, 5},
			{4, 1, 5}, {4, 1, 5}, {4, 1, 5}, {4, 1, 5},
			{4, 1, 5}, {4, 1, 5}, {4, 1, 5}, {4, 1, 5},
			{3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5},
			{3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5}},

		/* tab0a[252]: Table B-15, DCT coefficients table one,
	 * codes 000001xx ... 11111111
	 */
		{{65, 0, 6}, {65, 0, 6}, {65, 0, 6}, {65, 0, 6}, /* Escape */
			{7, 1, 7}, {7, 1, 7}, {8, 1, 7}, {8, 1, 7},
			{6, 1, 7}, {6, 1, 7}, {2, 2, 7}, {2, 2, 7},
			{0, 7, 6}, {0, 7, 6}, {0, 7, 6}, {0, 7, 6},
			{0, 6, 6}, {0, 6, 6}, {0, 6, 6}, {0, 6, 6},
			{4, 1, 6}, {4, 1, 6}, {4, 1, 6}, {4, 1, 6},
			{5, 1, 6}, {5, 1, 6}, {5, 1, 6}, {5, 1, 6},
			{1, 5, 8}, {11, 1, 8}, {0, 11, 8}, {0, 10, 8},
			{13, 1, 8}, {12, 1, 8}, {3, 2, 8}, {1, 4, 8},
			{2, 1, 5}, {2, 1, 5}, {2, 1, 5}, {2, 1, 5},
			{2, 1, 5}, {2, 1, 5}, {2, 1, 5}, {2, 1, 5},
			{1, 2, 5}, {1, 2, 5}, {1, 2, 5}, {1, 2, 5},
			{1, 2, 5}, {1, 2, 5}, {1, 2, 5}, {1, 2, 5},
			{3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5},
			{3, 1, 5}, {3, 1, 5}, {3, 1, 5}, {3, 1, 5},
			{1, 1, 3}, {1, 1, 3}, {1, 1, 3}, {1, 1, 3},
			{1, 1, 3}, {1, 1, 3}, {1, 1, 3}, {1, 1, 3},
			{1, 1, 3}, {1, 1, 3}, {1, 1, 3}, {1, 1, 3},
			{1, 1, 3}, {1, 1, 3}, {1, 1, 3}, {1, 1, 3},
			{1, 1, 3}, {1, 1, 3}, {1, 1, 3}, {1, 1, 3},
			{1, 1, 3}, {1, 1, 3}, {1, 1, 3}, {1, 1, 3},
			{1, 1, 3}, {1, 1, 3}, {1, 1, 3}, {1, 1, 3},
			{1, 1, 3}, {1, 1, 3}, {1, 1, 3}, {1, 1, 3},
			{64, 0, 4}, {64, 0, 4}, {64, 0, 4}, {64, 0, 4}, /* EOB */
			{64, 0, 4}, {64, 0, 4}, {64, 0, 4}, {64, 0, 4},
			{64, 0, 4}, {64, 0, 4}, {64, 0, 4}, {64, 0, 4},
			{64, 0, 4}, {64, 0, 4}, {64, 0, 4}, {64, 0, 4},
			{0, 3, 4}, {0, 3, 4}, {0, 3, 4}, {0, 3, 4},
			{0, 3, 4}, {0, 3, 4}, {0, 3, 4}, {0, 3, 4},
			{0, 3, 4}, {0, 3, 4}, {0, 3, 4}, {0, 3, 4},
			{0, 3, 4}, {0, 3, 4}, {0, 3, 4}, {0, 3, 4},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 1, 2}, {0, 1, 2}, {0, 1, 2}, {0, 1, 2},
			{0, 2, 3}, {0, 2, 3}, {0, 2, 3}, {0, 2, 3},
			{0, 2, 3}, {0, 2, 3}, {0, 2, 3}, {0, 2, 3},
			{0, 2, 3}, {0, 2, 3}, {0, 2, 3}, {0, 2, 3},
			{0, 2, 3}, {0, 2, 3}, {0, 2, 3}, {0, 2, 3},
			{0, 2, 3}, {0, 2, 3}, {0, 2, 3}, {0, 2, 3},
			{0, 2, 3}, {0, 2, 3}, {0, 2, 3}, {0, 2, 3},
			{0, 2, 3}, {0, 2, 3}, {0, 2, 3}, {0, 2, 3},
			{0, 2, 3}, {0, 2, 3}, {0, 2, 3}, {0, 2, 3},
			{0, 4, 5}, {0, 4, 5}, {0, 4, 5}, {0, 4, 5},
			{0, 4, 5}, {0, 4, 5}, {0, 4, 5}, {0, 4, 5},
			{0, 5, 5}, {0, 5, 5}, {0, 5, 5}, {0, 5, 5},
			{0, 5, 5}, {0, 5, 5}, {0, 5, 5}, {0, 5, 5},
			{9, 1, 7}, {9, 1, 7}, {1, 3, 7}, {1, 3, 7},
			{10, 1, 7}, {10, 1, 7}, {0, 8, 7}, {0, 8, 7},
			{0, 9, 7}, {0, 9, 7}, {0, 12, 8}, {0, 13, 8},
			{2, 3, 8}, {4, 2, 8}, {0, 14, 8}, {0, 15, 8}},

		/* Table B-14, DCT coefficients table zero,
	 * codes 0000001000 ... 0000001111
	 */
		{{16, 1, 10}, {5, 2, 10}, {0, 7, 10}, {2, 3, 10},
			{1, 4, 10}, {15, 1, 10}, {14, 1, 10}, {4, 2, 10}},

		/* Table B-15, DCT coefficients table one,
	 * codes 000000100x ... 000000111x
	 */
		{{5, 2, 9}, {5, 2, 9}, {14, 1, 9}, {14, 1, 9},
			{2, 4, 10}, {16, 1, 10}, {15, 1, 9}, {15, 1, 9}},

		/* Table B-14/15, DCT coefficients table zero / one,
	 * codes 000000010000 ... 000000011111
	 */
		{{0, 11, 12}, {8, 2, 12}, {4, 3, 12}, {0, 10, 12},
			{2, 4, 12}, {7, 2, 12}, {21, 1, 12}, {20, 1, 12},
			{0, 9, 12}, {19, 1, 12}, {18, 1, 12}, {1, 5, 12},
			{3, 3, 12}, {0, 8, 12}, {6, 2, 12}, {17, 1, 12}},

		/* Table B-14/15, DCT coefficients table zero / one,
	 * codes 0000000010000 ... 0000000011111
	 */
		{{10, 2, 13}, {9, 2, 13}, {5, 3, 13}, {3, 4, 13},
			{2, 5, 13}, {1, 7, 13}, {1, 6, 13}, {0, 15, 13},
			{0, 14, 13}, {0, 13, 13}, {0, 12, 13}, {26, 1, 13},
			{25, 1, 13}, {24, 1, 13}, {23, 1, 13}, {22, 1, 13}},

		/* Table B-14/15, DCT coefficients table zero / one,
	 * codes 00000000010000 ... 00000000011111
	 */
		{{0, 31, 14}, {0, 30, 14}, {0, 29, 14}, {0, 28, 14},
			{0, 27, 14}, {0, 26, 14}, {0, 25, 14}, {0, 24, 14},
			{0, 23, 14}, {0, 22, 14}, {0, 21, 14}, {0, 20, 14},
			{0, 19, 14}, {0, 18, 14}, {0, 17, 14}, {0, 16, 14}},

		/* Table B-14/15, DCT coefficients table zero / one,
	 * codes 000000000010000 ... 000000000011111
	 */
		{{0, 40, 15}, {0, 39, 15}, {0, 38, 15}, {0, 37, 15},
			{0, 36, 15}, {0, 35, 15}, {0, 34, 15}, {0, 33, 15},
			{0, 32, 15}, {1, 14, 15}, {1, 13, 15}, {1, 12, 15},
			{1, 11, 15}, {1, 10, 15}, {1, 9, 15}, {1, 8, 15}},

		/* Table B-14/15, DCT coefficients table zero / one,
	 * codes 0000000000010000 ... 0000000000011111
	 */
		{{1, 18, 16}, {1, 17, 16}, {1, 16, 16}, {1, 15, 16},
			{6, 3, 16}, {16, 2, 16}, {15, 2, 16}, {14, 2, 16},
			{13, 2, 16}, {12, 2, 16}, {11, 2, 16}, {31, 1, 16},
			{30, 1, 16}, {29, 1, 16}, {28, 1, 16}, {27, 1, 16}}

};

#undef VLC_ALIGNED16