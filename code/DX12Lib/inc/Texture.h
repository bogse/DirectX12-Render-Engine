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
	Texture(const D3D12_RESOURCE_DESC& resourceDesc,
		const D3D12_CLEAR_VALUE* clearValue = nullptr,
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON,
		const std::wstring& name = L"");

	/**
	* Copy not allowed for performance reasons.
	*/
	Texture(const Texture& copy) = delete;
	Texture& operator=(const Texture& other) = delete;

	/**
	* Move semantics allowed.
	*/
	Texture(Texture&& move) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	/**
	* Create SRV for the resource.
	*/
	virtual void CreateViews();

	/**
	* Resize the texture.
	*/
	void Resize(const uint32_t width, const uint32_t height, const uint32_t depthOrArraySize = 1u);

	/**
	* Get the SRV for a resource.
	*/
	D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView() const override;

	/**
	* Get the DSV for a resource.
	*/
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

	/**
	* Get the RTV for a texture.
	*/
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;

private:
	DescriptorAllocation m_ShaderResourceView;
	DescriptorAllocation m_DepthStencilView;
	DescriptorAllocation m_RenderTargetView;
};
