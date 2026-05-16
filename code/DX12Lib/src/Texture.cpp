#include <DX12LibPCH.h>

#include "Texture.h"

#include "Application.h"

Texture::Texture(const std::wstring& name)
	: Resource(name)
{
	m_ShaderResourceView = 
		Application::GetInstance().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Texture::CreateViews()
{
	if (m_d3d12Resource)
	{
		Microsoft::WRL::ComPtr<ID3D12Device2> device = Application::GetInstance().GetDevice();
		device->CreateShaderResourceView(m_d3d12Resource.Get(), nullptr, 
			m_ShaderResourceView.GetDescriptorHandle());
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetShaderResourceView() const
{
	return m_ShaderResourceView.GetDescriptorHandle();
}
