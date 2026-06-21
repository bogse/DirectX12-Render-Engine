#include <DX12LibPCH.h>

#include "Texture.h"

#include "Application.h"
#include "ResourceStateTracker.h"

namespace Texture_Private
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC GetUAVDesc(
		const D3D12_RESOURCE_DESC& resDesc,
		UINT mipSlice,
		UINT arraySlice = 0,
		UINT planeSlice = 0)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = resDesc.Format;

		switch (resDesc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		{
			if (resDesc.DepthOrArraySize > 1)
			{
				uavDesc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
				uavDesc.Texture1DArray.ArraySize		= resDesc.DepthOrArraySize - arraySlice;
				uavDesc.Texture1DArray.FirstArraySlice	= arraySlice;
				uavDesc.Texture1DArray.MipSlice			= mipSlice;
			}
			else
			{
				uavDesc.ViewDimension		= D3D12_UAV_DIMENSION_TEXTURE1D;
				uavDesc.Texture1D.MipSlice	= mipSlice;
			}
			break;
		}
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (resDesc.DepthOrArraySize > 1)
			{
				uavDesc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2DArray.ArraySize		= resDesc.DepthOrArraySize - arraySlice;
				uavDesc.Texture2DArray.FirstArraySlice	= arraySlice;
				uavDesc.Texture2DArray.PlaneSlice		= planeSlice;
				uavDesc.Texture2DArray.MipSlice			= mipSlice;
			}
			else
			{
				uavDesc.ViewDimension		 = D3D12_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.PlaneSlice = planeSlice;
				uavDesc.Texture2D.MipSlice   = mipSlice;
			}
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		{
			uavDesc.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.WSize			= resDesc.DepthOrArraySize - arraySlice;
			uavDesc.Texture3D.FirstWSlice	= arraySlice;
			uavDesc.Texture3D.MipSlice		= mipSlice;
			break;
		}
		default:
			throw std::exception("Invalid resource dimension.");
		}

		return uavDesc;
	}
}

Texture::Texture(const std::wstring& name)
	: Resource(name)
{
}

Texture::Texture(
	const D3D12_RESOURCE_DESC& resourceDesc,
	const D3D12_CLEAR_VALUE* clearValue,
	D3D12_RESOURCE_STATES initialState,
	const std::wstring& name
)
	: Resource(resourceDesc, clearValue, initialState, name)
{
	CreateViews();
}

Texture::Texture(Microsoft::WRL::ComPtr<ID3D12Resource> resource, const std::wstring& name)
	: Resource(resource, name)
{
	CreateViews();
}

Texture::Texture(Texture&& move) noexcept
	: Resource(std::move(move))
	, m_ShaderResourceView(std::move(move.m_ShaderResourceView))
	, m_UnorderedAccessView(std::move(move.m_UnorderedAccessView))
	, m_DepthStencilView(std::move(move.m_DepthStencilView))
	, m_RenderTargetView(std::move(move.m_RenderTargetView))
{
}

Texture& Texture::operator=(Texture&& other) noexcept
{
	if (this != &other)
	{
		Resource::operator=(std::move(other));

		m_ShaderResourceView = std::move(other.m_ShaderResourceView);
		m_UnorderedAccessView = std::move(other.m_UnorderedAccessView);
		m_DepthStencilView   = std::move(other.m_DepthStencilView);
		m_RenderTargetView	 = std::move(other.m_RenderTargetView);
	}

	return *this;
}

void Texture::CreateViews()
{
	if (!m_d3d12Resource)
	{
		assert(false && "Attempted to create views on an uninitialized Texture.");
		return;
	}

	const Microsoft::WRL::ComPtr<ID3D12Device2>& device = Application::GetInstance().GetDevice();
	CD3DX12_RESOURCE_DESC desc(m_d3d12Resource->GetDesc());

	// Create SRV.
	if ((desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0 &&
		desc.Format != DXGI_FORMAT_D32_FLOAT)
	{
		m_ShaderResourceView = Application::GetInstance().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		if (m_ShaderResourceView.IsNull())
		{
			throw std::runtime_error("Descriptor Heap out of memory during SRV creation.");
		}

		device->CreateShaderResourceView(m_d3d12Resource.Get(), nullptr, m_ShaderResourceView.GetDescriptorHandle());
	}

	// Create UAV for each mip (only supported for 1D and 2D textures).
	if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0 &&
		desc.DepthOrArraySize == 1)
	{
		m_UnorderedAccessView = Application::GetInstance().AllocateDescriptors(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, desc.MipLevels);

		for (int i = 0; i < desc.MipLevels; ++i)
		{
			const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc = Texture_Private::GetUAVDesc(desc, i);
			device->CreateUnorderedAccessView(
				m_d3d12Resource.Get(), nullptr, &uavDesc, m_UnorderedAccessView.GetDescriptorHandle(i));
		}
	}

	// Create DSV.
	if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
	{
		m_DepthStencilView = Application::GetInstance().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		if (m_DepthStencilView.IsNull())
		{
			throw std::runtime_error("Descriptor Heap out of memory during DSV creation.");
		}

		device->CreateDepthStencilView(m_d3d12Resource.Get(), nullptr, m_DepthStencilView.GetDescriptorHandle());
	}

	if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
	{
		m_RenderTargetView = Application::GetInstance().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		if (m_RenderTargetView.IsNull())
		{
			throw::std::runtime_error("Descriptor Heap out of memory during RTV creation.");
		}

		device->CreateRenderTargetView(m_d3d12Resource.Get(), nullptr, m_RenderTargetView.GetDescriptorHandle());
	}
}

void Texture::Resize(const uint32_t width, const uint32_t height, const uint32_t depthOrArraySize)
{
	// Resource can't be resized if it was never created in the first place.
	if (!m_d3d12Resource)
		return;

	ResourceStateTracker::RemoveGlobalResourceState(m_d3d12Resource.Get());

	CD3DX12_RESOURCE_DESC resourceDescription(m_d3d12Resource->GetDesc());

	resourceDescription.Width = std::max(1, static_cast<int>(width));
	resourceDescription.Height = std::max(1, static_cast<int>(height));
	resourceDescription.DepthOrArraySize = depthOrArraySize;

	const Microsoft::WRL::ComPtr<ID3D12Device2>& device = Application::GetInstance().GetDevice();

	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDescription,
		D3D12_RESOURCE_STATE_COMMON,
		m_d3d12ClearValue.get(),
		IID_PPV_ARGS(&m_d3d12Resource)
	));

	ResourceStateTracker::AddGlobalResourceState(m_d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

	CreateViews();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetShaderResourceView() const
{
	return m_ShaderResourceView.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetUnorderedAccessView(uint32_t mip) const
{
	return m_UnorderedAccessView.GetDescriptorHandle(mip);
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetDepthStencilView() const
{
	return m_DepthStencilView.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetRenderTargetView() const
{
	return m_RenderTargetView.GetDescriptorHandle();
}

bool Texture::IsSRGBFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return true;
	default:
		return false;
	}
}

DXGI_FORMAT Texture::GetSRGBFormat(DXGI_FORMAT format)
{
	DXGI_FORMAT srgbFormat = format;
	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		srgbFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC1_UNORM:
		srgbFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC2_UNORM:
		srgbFormat = DXGI_FORMAT_BC2_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC3_UNORM:
		srgbFormat = DXGI_FORMAT_BC3_UNORM_SRGB;
		break;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		srgbFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		break;
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		srgbFormat = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC7_UNORM:
		srgbFormat = DXGI_FORMAT_BC7_UNORM_SRGB;
		break;
	}

	return srgbFormat;
}

DXGI_FORMAT Texture::GetUAVCompatibleFormat(DXGI_FORMAT format)
{
	DXGI_FORMAT uavFormat = format;

	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		uavFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		uavFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
		uavFormat = DXGI_FORMAT_R32_FLOAT;
		break;
	}

	return uavFormat;
}

//void Texture::CheckFeatureSupport()
//{
//	if (m_d3d12Resource)
//	{
//		const D3D12_RESOURCE_DESC& desc = m_d3d12Resource->GetDesc();
//		const Microsoft::WRL::ComPtr<ID3D12Device2>& device = Application::GetInstance().GetDevice();
//
//		m_FormatSupport.Format = desc.Format;
//		ThrowIfFailed(device->CheckFeatureSupport(
//			D3D12_FEATURE_FORMAT_SUPPORT,
//			&m_FormatSupport,
//			sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
//	}
//	else
//	{
//		m_FormatSupport = {};
//	}
//}
//
//bool Texture::CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const
//{
//	return (m_FormatSupport.Support1 & formatSupport) != 0;
//}
//
//bool Texture::CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const
//{
//	return (m_FormatSupport.Support2 & formatSupport) != 0;
//}
