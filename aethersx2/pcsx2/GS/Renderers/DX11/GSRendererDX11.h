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

#include "GS/Renderers/HW/GSRendererHW.h"
#include "GSTextureCache11.h"
#include "GS/Renderers/HW/GSVertexHW.h"

class GSRendererDX11 final : public GSRendererHW
{
	enum ACC_BLEND_D3D11
	{
		ACC_BLEND_NONE_D3D11   = 0,
		ACC_BLEND_BASIC_D3D11  = 1,
		ACC_BLEND_MEDIUM_D3D11 = 2,
		ACC_BLEND_HIGH_D3D11   = 3
	};

private:
	bool m_bind_rtsample;

private:
	__fi GSDevice11* GetDevice11() { return static_cast<GSDevice11*>(m_dev.get()); }

	inline void ResetStates();
	inline void SetupIA(const float& sx, const float& sy);
	inline void EmulateZbuffer();
	inline void EmulateBlending();
	inline void EmulateTextureShuffleAndFbmask();
	inline void EmulateChannelShuffle(GSTexture** rt, const GSTextureCache::Source* tex);
	inline void EmulateTextureSampler(const GSTextureCache::Source* tex);

	GSDevice11::VSSelector m_vs_sel;
	GSDevice11::GSSelector m_gs_sel;
	GSDevice11::PSSelector m_ps_sel;

	GSDevice11::PSSamplerSelector      m_ps_ssel;
	GSDevice11::OMBlendSelector        m_om_bsel;
	GSDevice11::OMDepthStencilSelector m_om_dssel;

	GSDevice11::PSConstantBuffer ps_cb;
	GSDevice11::VSConstantBuffer vs_cb;
	GSDevice11::GSConstantBuffer gs_cb;

public:
	GSRendererDX11(std::unique_ptr<GSDevice> dev);
	virtual ~GSRendererDX11() {}

	const char* GetName() const override;

	void DrawPrims(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex) final;
};
