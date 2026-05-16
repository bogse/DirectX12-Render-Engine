/**
* Wrapper for DX12 Texture object.
*/

#pragma once

#include "DescriptorAllocation.h"
#include "Resource.h"

#include <d3dx12.h>

#include <map>

class Texture : public Resource
{
public:
	Texture(const std::wstring& name = L"");

	/**
	* Create SRV for the resource.
	*/
	virtual void CreateViews();

	/**
	* Get the SRV for a resource.
	*/
	D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView() const override;

private:
	DescriptorAllocation m_ShaderResourceView;
};