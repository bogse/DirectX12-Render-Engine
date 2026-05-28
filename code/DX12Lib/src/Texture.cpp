#include <DX12LibPCH.h>

#include "Texture.h"

#include "Application.h"
#include "ResourceStateTracker.h"

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

Texture::Texture(Texture&& move) noexcept
	: Resource(std::move(move))
	, m_ShaderResourceView(std::move(move.m_ShaderResourceView))
	, m_DepthStencilView(std::move(move.m_DepthStencilView))
{
}

Texture& Texture::operator=(Texture&& other) noexcept
{
	if (this != &other)
	{
		Resource::operator=(std::move(other));

		m_ShaderResourceView = std::move(other.m_ShaderResourceView);
		m_DepthStencilView   = std::move(other.m_DepthStencilView);
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
}

void Texture::Resize(const uint32_t width, const uint32_t height, const uint32_t depthOrArraySize)
{
	// Resource can't be resized if it was never created in the first place.
	if (!m_d3d12Resource)
		return;

	ResourceStateTracker::RemoveGlobalResourceState(m_d3d12Resource.Get());

	CD3DX12_RESOURCE_DESC resourceDescription(m_d3d12Resource->GetDesc());

	resourceDescription.Width = width;
	resourceDescription.Height = height;
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

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetDepthStencilView() const
{
	return m_DepthStencilView.GetDescriptorHandle();
}
