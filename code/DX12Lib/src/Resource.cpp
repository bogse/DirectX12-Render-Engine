#include "DX12LibPCH.h"

#include "Resource.h"

#include "Application.h"
#include "ResourceStateTracker.h"

Resource::Resource(const std::wstring& name)
	: m_ResourceName(name)
	, m_FormatSupport({})
{}

Resource::Resource(
	const D3D12_RESOURCE_DESC & resourceDesc,
	const D3D12_CLEAR_VALUE * clearValue,
	D3D12_RESOURCE_STATES initialState,
	const std::wstring & name)
{
	const Microsoft::WRL::ComPtr<ID3D12Device2>& device = Application::GetInstance().GetDevice();

	if (clearValue)
	{
		m_d3d12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
	}

	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		initialState,
		m_d3d12ClearValue.get(),
		IID_PPV_ARGS(&m_d3d12Resource)
	));

	ResourceStateTracker::AddGlobalResourceState(m_d3d12Resource.Get(), initialState);

	CheckFeatureSupport();
	SetName(name);
}

Resource::Resource(
	Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	const std::wstring& name
)
	: m_d3d12Resource(resource)
	, m_FormatSupport({})
{
	CheckFeatureSupport();
	SetName(name);
}

Resource::Resource(Resource&& move) noexcept
	: m_d3d12Resource(std::move(move.m_d3d12Resource))
	, m_FormatSupport({})
	, m_d3d12ClearValue(std::move(move.m_d3d12ClearValue))
	, m_ResourceName(std::move(move.m_ResourceName))
{
}

Resource& Resource::operator=(Resource&& other) noexcept
{
	if (this != &other)
	{
		// Unregister old pointer from the tracker before we lose it.
		if (m_d3d12Resource)
		{
			ResourceStateTracker::RemoveGlobalResourceState(m_d3d12Resource.Get());
		}

		m_d3d12Resource	  = std::move(other.m_d3d12Resource);
		m_FormatSupport	  = std::move(other.m_FormatSupport);
		m_d3d12ClearValue = std::move(other.m_d3d12ClearValue);
		m_ResourceName	  = std::move(other.m_ResourceName);

		// No tracker updates needed. It is already registered and raw address didn't change.
	}

	return *this;
}

Resource::~Resource()
{}

Microsoft::WRL::ComPtr<ID3D12Resource> Resource::GetD3D12Resource() const
{
	return m_d3d12Resource;
}

D3D12_RESOURCE_DESC Resource::GetD3D12ResourceDesc() const
{
	D3D12_RESOURCE_DESC resDesc = {};
	if (m_d3d12Resource)
		resDesc = m_d3d12Resource->GetDesc();

	return resDesc;
}

void Resource::SetD3D12Resource(
	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource)
{
	m_d3d12Resource = d3d12Resource;
	CheckFeatureSupport();
	SetName(m_ResourceName);
}

void Resource::SetName(const std::wstring& name)
{
	m_ResourceName = name;
	if (m_d3d12Resource && !m_ResourceName.empty())
	{
		m_d3d12Resource->SetName(m_ResourceName.c_str());
	}
}

void Resource::Reset()
{
	m_d3d12Resource.Reset();
	m_FormatSupport = {};
	m_d3d12ClearValue.reset();
	m_ResourceName.clear();
}

bool Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const
{
	return (m_FormatSupport.Support1 & formatSupport) != 0;
}

bool Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const
{
	return (m_FormatSupport.Support2 & formatSupport) != 0;
}

ULONG Resource::RefCount() const
{
	if (m_d3d12Resource)
	{
		m_d3d12Resource->AddRef();
		return m_d3d12Resource->Release();
	}

	return 0ul;
}

void Resource::CheckFeatureSupport()
{
	if (m_d3d12Resource)
	{
		const D3D12_RESOURCE_DESC& desc = m_d3d12Resource->GetDesc();
		const Microsoft::WRL::ComPtr<ID3D12Device2>& device = Application::GetInstance().GetDevice();

		m_FormatSupport.Format = desc.Format;
		ThrowIfFailed(device->CheckFeatureSupport(
			D3D12_FEATURE_FORMAT_SUPPORT,
			&m_FormatSupport,
			sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
	}
	else
	{
		m_FormatSupport = {};
	}
}
