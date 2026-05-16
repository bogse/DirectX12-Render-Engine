/**
* A wrapper for a DX12 resource.
* This provides a base class for all other resource types (Buffers & Textures).
*/
#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <string>

class Resource
{
public:
	Resource(const std::wstring& name = L"");
	virtual ~Resource();

	/**
	* Get access to the underlying D3D12 resource.
	*/
	Microsoft::WRL::ComPtr<ID3D12Resource> GetD3D12Resource() const;

	/**
	* Replace the D3D12 resource.
	* Should only be called by the CommandList.
	*/
	virtual void SetD3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource);

	/**
	* Get the SRV for a resource.
	*/
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView() const = 0;

	/**
	* Set the name of the resource. Useful for debugging purposes.
	* The name of the resource will persist if the underlying D3D12 resource is
	* replaced with SetD3D12Resource.
	*/
	void SetName(const std::wstring& name);

	/**
	* Release the underlying resource.
	* This is useful for swap chain resizing.
	*/
	virtual void Reset();

	ULONG RefCount() const;

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_d3d12Resource;
	std::unique_ptr<D3D12_CLEAR_VALUE> m_d3d12ClearValue;

private:
	std::wstring m_ResourceName;
};