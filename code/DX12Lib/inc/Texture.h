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
	Texture(Microsoft::WRL::ComPtr<ID3D12Resource> resource,
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
	* Get the UAV for a (sub)resource.
	*/
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(uint32_t mip = 0) const;

	/**
	* Get the DSV for a resource.
	*/
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

	/**
	* Get the RTV for a texture.
	*/
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;

	bool CheckUAVSupport() const
	{
		return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
			CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) &&
			CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
	}
	// TODO_BOG add all other support check?

	static bool IsSRGBFormat(DXGI_FORMAT format);

	static DXGI_FORMAT GetSRGBFormat(DXGI_FORMAT format);
	static DXGI_FORMAT GetUAVCompatibleFormat(DXGI_FORMAT format);

private:
	DescriptorAllocation m_ShaderResourceView;
	DescriptorAllocation m_UnorderedAccessView;
	DescriptorAllocation m_DepthStencilView;
	DescriptorAllocation m_RenderTargetView;

	D3D12_FEATURE_DATA_FORMAT_SUPPORT m_FormatSupport;
};
