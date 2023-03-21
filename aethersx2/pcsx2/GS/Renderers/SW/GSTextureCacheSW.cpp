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
#include "GSTextureCacheSW.h"

GSTextureCacheSW::GSTextureCacheSW(GSState* state)
	: m_state(state)
{
}

GSTextureCacheSW::~GSTextureCacheSW()
{
	RemoveAll();
}

GSTextureCacheSW::Texture* GSTextureCacheSW::Lookup(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, uint32 tw0)
{
	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];

	auto& m = m_map[TEX0.TBP0 >> 5];

	for (auto i = m.begin(); i != m.end(); ++i)
	{
		Texture* t = *i;

		if (((TEX0.u32[0] ^ t->m_TEX0.u32[0]) | ((TEX0.u32[1] ^ t->m_TEX0.u32[1]) & 3)) != 0) // TBP0 TBW PSM TW TH
		{
			continue;
		}

		if ((psm.trbpp == 16 || psm.trbpp == 24) && TEX0.TCC && TEXA != t->m_TEXA)
		{
			continue;
		}

		if (tw0 != 0 && t->m_tw != tw0)
		{
			continue;
		}

		// Lookup hit
		m.MoveFront(i.Index());
		t->m_age = 0;
		return t;
	}

	// Lookup miss
	Texture* t = new Texture(m_state, tw0, TEX0, TEXA);

	m_textures.insert(t);

	t->m_pages.loopPages([&](uint32 page)
	{
		t->m_erase_it[page] = m_map[page].InsertFront(t);
	});

	return t;
}

void GSTextureCacheSW::InvalidatePages(const GSOffset::PageLooper& pages, uint32 psm)
{
	pages.loopPages([&](uint32 page)
	{
		for (Texture* t : m_map[page])
		{
			if (GSUtil::HasSharedBits(psm, t->m_sharedbits))
			{
				uint32* RESTRICT valid = t->m_valid;

				if (t->m_repeating)
				{
					for (const GSVector2i& j : t->m_p2t[page])
					{
						valid[j.x] &= j.y;
					}
				}
				else
				{
					valid[page] = 0;
				}

				t->m_complete = false;
			}
		}
	});
}

void GSTextureCacheSW::RemoveAll()
{
	for (auto i : m_textures)
		delete i;

	m_textures.clear();

	for (auto& l : m_map)
	{
		l.clear();
	}
}

void GSTextureCacheSW::IncAge()
{
	for (auto i = m_textures.begin(); i != m_textures.end();)
	{
		Texture* t = *i;

		if (++t->m_age > 10)
		{
			i = m_textures.erase(i);

			t->m_pages.loopPages([&](uint32 page)
			{
				m_map[page].EraseIndex(t->m_erase_it[page]);
			});

			delete t;
		}
		else
		{
			++i;
		}
	}
}

//

GSTextureCacheSW::Texture::Texture(GSState* state, uint32 tw0, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA)
	: m_state(state)
	, m_buff(NULL)
	, m_tw(tw0)
	, m_age(0)
	, m_complete(false)
	, m_p2t(NULL)
{
	m_TEX0 = TEX0;
	m_TEXA = TEXA;

	if (m_tw == 0)
	{
		m_tw = std::max<int>(m_TEX0.TW, GSLocalMemory::m_psm[m_TEX0.PSM].pal == 0 ? 3 : 5); // makes one row 32 bytes at least, matches the smallest block size that is allocated for m_buff
	}

	memset(m_valid, 0, sizeof(m_valid));

	m_sharedbits = GSUtil::HasSharedBitsPtr(m_TEX0.PSM);

	m_offset = m_state->m_mem.GetOffset(TEX0.TBP0, TEX0.TBW, TEX0.PSM);
	m_pages = m_offset.pageLooperForRect(GSVector4i(0, 0, 1 << TEX0.TW, 1 << TEX0.TH));

	m_repeating = m_TEX0.IsRepeating(); // repeating mode always works, it is just slightly slower

	if (m_repeating)
	{
		m_p2t = m_state->m_mem.GetPage2TileMap(m_TEX0);
	}
}

GSTextureCacheSW::Texture::~Texture()
{
	if (m_buff)
	{
		_aligned_free(m_buff);
	}
}

bool GSTextureCacheSW::Texture::Update(const GSVector4i& rect)
{
	if (m_complete)
	{
		return true;
	}

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[m_TEX0.PSM];

	GSVector2i bs = psm.bs;

	int shift = psm.pal == 0 ? 2 : 0;

	int tw = std::max<int>(1 << m_TEX0.TW, bs.x);
	int th = std::max<int>(1 << m_TEX0.TH, bs.y);

	GSVector4i r = rect;

	r = r.ralign<Align_Outside>(bs);

	if (r.eq(GSVector4i(0, 0, tw, th)))
	{
		m_complete = true; // lame, but better than nothing
	}

	if (m_buff == NULL)
	{
		uint32 pitch = (1 << m_tw) << shift;

		m_buff = _aligned_malloc(pitch * th * 4, 32);

		if (m_buff == NULL)
		{
			return false;
		}
	}

	GSLocalMemory& mem = m_state->m_mem;

	GSOffset off = m_offset;

	uint32 blocks = 0;

	GSLocalMemory::readTextureBlock rtxbP = psm.rtxbP;

	uint32 pitch = (1 << m_tw) << shift;

	uint8* dst = (uint8*)m_buff + pitch * r.top;

	int block_pitch = pitch * bs.y;

	shift += off.blockShiftX();
	int bottom = r.bottom >> off.blockShiftY();
	int right = r.right >> off.blockShiftX();

	GSOffset::BNHelper bn = off.bnMulti(r.left, r.top);

	if (m_repeating)
	{
		for (; bn.blkY() < bottom; bn.nextBlockY(), dst += block_pitch)
		{
			for (; bn.blkX() < right; bn.nextBlockX())
			{
				int i = (bn.blkY() << 7) + bn.blkX();
				uint32 block = bn.value();

				uint32 row = i >> 5;
				uint32 col = 1 << (i & 31);

				if ((m_valid[row] & col) == 0)
				{
					m_valid[row] |= col;

					(mem.*rtxbP)(block, &dst[bn.blkX() << shift], pitch, m_TEXA);

					blocks++;
				}
			}
		}
	}
	else
	{
		for (; bn.blkY() < bottom; bn.nextBlockY(), dst += block_pitch)
		{
			for (; bn.blkX() < right; bn.nextBlockX())
			{
				uint32 block = bn.value();

				uint32 row = block >> 5;
				uint32 col = 1 << (block & 31);

				if ((m_valid[row] & col) == 0)
				{
					m_valid[row] |= col;

					(mem.*rtxbP)(block, &dst[bn.blkX() << shift], pitch, m_TEXA);

					blocks++;
				}
			}
		}
	}

	if (blocks > 0)
	{
		g_perfmon.Put(GSPerfMon::Unswizzle, bs.x * bs.y * blocks << shift);
	}

	return true;
}

#include "GSTextureSW.h"

bool GSTextureCacheSW::Texture::Save(const std::string& fn, bool dds) const
{
	const uint32* RESTRICT clut = m_state->m_mem.m_clut;

	int w = 1 << m_TEX0.TW;
	int h = 1 << m_TEX0.TH;

	GSTextureSW t(0, w, h);

	GSTexture::GSMap m;

	if (t.Map(m, NULL))
	{
		const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[m_TEX0.PSM];

		const uint8* RESTRICT src = (uint8*)m_buff;
		int pitch = 1 << (m_tw + (psm.pal == 0 ? 2 : 0));

		for (int j = 0; j < h; j++, src += pitch, m.bits += m.pitch)
		{
			if (psm.pal == 0)
			{
				memcpy(m.bits, src, sizeof(uint32) * w);
			}
			else
			{
				for (int i = 0; i < w; i++)
				{
					((uint32*)m.bits)[i] = clut[src[i]];
				}
			}
		}

		t.Unmap();

		return t.Save(fn);
	}

	return false;
}
