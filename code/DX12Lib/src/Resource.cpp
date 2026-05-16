#include "DX12LibPCH.h"

#include "Resource.h"

#include "Application.h"
#include "ResourceStateTracker.h"

Resource::Resource(const std::wstring& name)
	: m_ResourceName(name)
{}

Resource::~Resource()
{}

Microsoft::WRL::ComPtr<ID3D12Resource> Resource::GetD3D12Resource() const
{
	return m_d3d12Resource;
}

void Resource::SetD3D12Resource(
	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource)
{
	m_d3d12Resource = d3d12Resource;
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
	m_d3d12ClearValue.reset();
	m_ResourceName.clear();
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
