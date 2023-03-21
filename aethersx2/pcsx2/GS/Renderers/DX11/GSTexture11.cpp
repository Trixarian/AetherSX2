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
#include "GSTexture11.h"
#include "GS/GSPng.h"
#include "GS/GSPerfMon.h"

GSTexture11::GSTexture11(wil::com_ptr_nothrow<ID3D11Texture2D> texture)
	: m_texture(std::move(texture)), m_layer(0)
{
	ASSERT(m_texture);

	m_texture->GetDevice(m_dev.put());
	m_texture->GetDesc(&m_desc);

	m_dev->GetImmediateContext(m_ctx.put());

	m_size.x = (int)m_desc.Width;
	m_size.y = (int)m_desc.Height;

	if (m_desc.BindFlags & D3D11_BIND_RENDER_TARGET)
		m_type = RenderTarget;
	else if (m_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
		m_type = DepthStencil;
	else if (m_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
		m_type = Texture;
	else if (m_desc.Usage == D3D11_USAGE_STAGING)
		m_type = Offscreen;

	m_format = (int)m_desc.Format;

	m_max_layer = m_desc.MipLevels;
}

void* GSTexture11::GetNativeHandle() const
{
	return static_cast<ID3D11ShaderResourceView*>(*const_cast<GSTexture11*>(this));
}

bool GSTexture11::Update(const GSVector4i& r, const void* data, int pitch, int layer)
{
	if (layer >= m_max_layer)
		return true;

	if (m_dev && m_texture)
	{
		g_perfmon.Put(GSPerfMon::TextureUploads, 1);

		D3D11_BOX box = {(UINT)r.left, (UINT)r.top, 0U, (UINT)r.right, (UINT)r.bottom, 1U};
		UINT subresource = layer; // MipSlice + (ArraySlice * MipLevels).

		m_ctx->UpdateSubresource(m_texture.get(), subresource, &box, data, pitch, 0);

		return true;
	}

	return false;
}

bool GSTexture11::Map(GSMap& m, const GSVector4i* r, int layer)
{
	if (r != NULL)
	{
		// ASSERT(0); // not implemented
		return false;
	}

	if (layer >= m_max_layer)
		return false;

	if (m_texture && m_desc.Usage == D3D11_USAGE_STAGING)
	{
		D3D11_MAPPED_SUBRESOURCE map;
		UINT subresource = layer;

		if (SUCCEEDED(m_ctx->Map(m_texture.get(), subresource, D3D11_MAP_READ_WRITE, 0, &map)))
		{
			m.bits = (uint8*)map.pData;
			m.pitch = (int)map.RowPitch;

			m_layer = layer;

			return true;
		}
	}

	return false;
}

void GSTexture11::Unmap()
{
	if (m_texture)
	{
		UINT subresource = m_layer;
		m_ctx->Unmap(m_texture.get(), subresource);
	}
}

bool GSTexture11::Save(const std::string& fn)
{
	D3D11_TEXTURE2D_DESC desc;

	m_texture->GetDesc(&desc);

	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	wil::com_ptr_nothrow<ID3D11Texture2D> res;
	HRESULT hr = m_dev->CreateTexture2D(&desc, nullptr, res.put());
	if (FAILED(hr))
	{
		return false;
	}

	m_ctx->CopyResource(res.get(), m_texture.get());

	if (m_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;

		wil::com_ptr_nothrow<ID3D11Texture2D> dst;
		hr = m_dev->CreateTexture2D(&desc, nullptr, dst.put());
		if (FAILED(hr))
		{
			return false;
		}

		D3D11_MAPPED_SUBRESOURCE sm, dm;

		hr = m_ctx->Map(res.get(), 0, D3D11_MAP_READ, 0, &sm);
		if (FAILED(hr))
		{
			return false;
		}
		auto unmap_res = wil::scope_exit([this, res]{ // Capture by value to preserve the original pointer
			m_ctx->Unmap(res.get(), 0);
		});

		hr = m_ctx->Map(dst.get(), 0, D3D11_MAP_WRITE, 0, &dm);
		if (FAILED(hr))
		{
			return false;
		}
		auto unmap_dst = wil::scope_exit([this, dst]{ // Capture by value to preserve the original pointer
			m_ctx->Unmap(dst.get(), 0);
		});

		const uint8* s = static_cast<const uint8*>(sm.pData);
		uint8* d = static_cast<uint8*>(dm.pData);

		for (uint32 y = 0; y < desc.Height; y++, s += sm.RowPitch, d += dm.RowPitch)
		{
			for (uint32 x = 0; x < desc.Width; x++)
			{
				reinterpret_cast<uint32*>(d)[x] = static_cast<uint32>(ldexpf(reinterpret_cast<const float*>(s)[x * 2], 32));
			}
		}

		res = std::move(dst);
	}

	res->GetDesc(&desc);

#ifdef ENABLE_OGL_DEBUG
	GSPng::Format format = GSPng::RGB_A_PNG;
#else
	GSPng::Format format = GSPng::RGB_PNG;
#endif
	switch (desc.Format)
	{
		case DXGI_FORMAT_A8_UNORM:
			format = GSPng::R8I_PNG;
			break;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			break;
		default:
			fprintf(stderr, "DXGI_FORMAT %d not saved to image\n", desc.Format);
			return false;
	}

	D3D11_MAPPED_SUBRESOURCE sm;
	hr = m_ctx->Map(res.get(), 0, D3D11_MAP_READ, 0, &sm);
	if (FAILED(hr))
	{
		return false;
	}

	int compression = theApp.GetConfigI("png_compression_level");
	bool success = GSPng::Save(format, fn, static_cast<uint8*>(sm.pData), desc.Width, desc.Height, sm.RowPitch, compression);

	m_ctx->Unmap(res.get(), 0);

	return success;
}

GSTexture11::operator ID3D11Texture2D*()
{
	return m_texture.get();
}

GSTexture11::operator ID3D11ShaderResourceView*()
{
	if (!m_srv && m_dev && m_texture)
	{
		if (m_desc.Format == DXGI_FORMAT_R32G8X24_TYPELESS)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};

			srvd.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
			srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvd.Texture2D.MipLevels = 1;

			m_dev->CreateShaderResourceView(m_texture.get(), &srvd, m_srv.put());
		}
		else
		{
			m_dev->CreateShaderResourceView(m_texture.get(), nullptr, m_srv.put());
		}
	}

	return m_srv.get();
}

GSTexture11::operator ID3D11RenderTargetView*()
{
	ASSERT(m_dev);

	if (!m_rtv && m_dev && m_texture)
	{
		m_dev->CreateRenderTargetView(m_texture.get(), nullptr, m_rtv.put());
	}

	return m_rtv.get();
}

GSTexture11::operator ID3D11DepthStencilView*()
{
	if (!m_dsv && m_dev && m_texture)
	{
		if (m_desc.Format == DXGI_FORMAT_R32G8X24_TYPELESS)
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = {};

			dsvd.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

			m_dev->CreateDepthStencilView(m_texture.get(), &dsvd, m_dsv.put());
		}
		else
		{
			m_dev->CreateDepthStencilView(m_texture.get(), nullptr, m_dsv.put());
		}
	}

	return m_dsv.get();
}

bool GSTexture11::Equal(GSTexture11* tex)
{
	return tex && m_texture == tex->m_texture;
}
