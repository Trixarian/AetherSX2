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
#include "GS/Renderers/DX11/D3D.h"
#include "GS/GS.h"

#include <d3d11.h>

namespace D3D
{
	wil::com_ptr_nothrow<IDXGIFactory2> CreateFactory(bool debug)
	{
		UINT flags = 0;
		if (debug)
			flags |= DXGI_CREATE_FACTORY_DEBUG;

		// we use CreateDXGIFactory2 because we assume at least windows 8.1 anyway
		wil::com_ptr_nothrow<IDXGIFactory2> factory;
		HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(factory.put()));

		// if we failed to create a factory with debug support
		// try one without
		if (FAILED(hr) && debug)
		{
			fprintf(stderr, "D3D: failed to create debug dxgi factory, trying without debugging\n");
			hr = CreateDXGIFactory2(0, IID_PPV_ARGS(factory.put()));;
		}

		if (FAILED(hr))
		{
			fprintf(stderr, "D3D: failed to create dxgi factory\n"
				"check that your system meets our minimum requirements:\n"
				"https://github.com/PCSX2/pcsx2#system-requirements\n");
		}

		return factory;
	}

	std::vector<std::string> GetAdapterList(IDXGIFactory2* factory)
	{
		ASSERT(factory);

		UINT index = 0;
		wil::com_ptr_nothrow<IDXGIAdapter1> adapter;

		std::vector<std::string> adapter_list;
		while (factory->EnumAdapters1(index, adapter.put()) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC1 desc;

			std::string name;
			if (SUCCEEDED(adapter->GetDesc1(&desc)))
				name = convert_utf16_to_utf8(desc.Description);

			adapter_list.push_back(std::move(name));

			index++;
		}

		return adapter_list;
	}

	wil::com_ptr_nothrow<IDXGIAdapter1> GetAdapterFromIndex(IDXGIFactory2* factory, int index)
	{
		ASSERT(factory);

		wil::com_ptr_nothrow<IDXGIAdapter1> adapter;
		if (factory->EnumAdapters1(index, adapter.put()) == DXGI_ERROR_NOT_FOUND)
		{
			// try index 0 (default adapter)
			fprintf(stderr, "D3D: adapter not found, falling back to the default\n");
			if (FAILED(factory->EnumAdapters1(0, adapter.put())))
			{
				// either there are no adapters connected or something major is wrong with the system
				fprintf(stderr, "D3D: failed to EnumAdapters\n");
			}
		}

		return adapter;
	}

	bool IsNvidia(IDXGIAdapter1* adapter)
	{
		ASSERT(adapter);

		DXGI_ADAPTER_DESC1 desc = {};
		if (FAILED(adapter->GetDesc1(&desc)))
		{
			fprintf(stderr, "D3D: failed to get the adapter description\n");
			return false;
		}

		// NV magic number
		return desc.VendorId == 0x10DE;
	}

	bool SupportsFeatureLevel11(IDXGIAdapter1* adapter)
	{
		ASSERT(adapter);

		D3D_FEATURE_LEVEL feature_level;

		static const D3D_FEATURE_LEVEL check[] = { D3D_FEATURE_LEVEL_11_0 };
		const HRESULT hr = D3D11CreateDevice(
			adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
			check, std::size(check), D3D11_SDK_VERSION, nullptr, &feature_level, nullptr
		);

		if (FAILED(hr))
			return false;

		return feature_level == D3D_FEATURE_LEVEL_11_0;
	}

	bool ShouldPreferD3D()
	{
		auto factory = CreateFactory(false);
		auto adapter = GetAdapterFromIndex(factory.get(), 0);

		return !(IsNvidia(adapter.get()) && SupportsFeatureLevel11(adapter.get()));
	}
}
