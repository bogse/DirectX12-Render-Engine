#pragma once

#include "d3dx12.h"

#include <wrl/client.h>

class Device;

class RootSignature
{
public:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetD3D12RootSignature() const
	{
		return m_RootSignature;
	}

	const D3D12_ROOT_SIGNATURE_DESC1& GetRootSignatureDesc() const
	{
		return m_RootSignatureDesc;
	}

	uint32_t GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const;
	uint32_t GetNumDescriptors(uint32_t rootIndex) const;

protected:
	/**
	* Root signatures can only be created through the Device.
	*/
	RootSignature(Device& device, const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc);

	virtual ~RootSignature();

private:
	void Destroy();

	void SetRootSignatureDesc(const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc);

	Device&										m_Device;
	D3D12_ROOT_SIGNATURE_DESC1					m_RootSignatureDesc;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
	
	/**
	* Need to know the number of descriptors per descriptor table.
	* A maximum of 32 descriptor tables are supported (since a 32-bit
	* mask is used to represent the descriptor tables in the root signature.
	*/
	uint32_t m_NumDescriptorsPerTable[32];

	/**
	* A bit mask that represents the root paramenter indices that are
	* descriptor tables for Samplers.
	*/
	uint32_t m_SamplerTableBitMask;

	/**
	* A bit mask that represents the root parameter indices that are
	* CBV, UAV, and SRV descriptor tables.
	*/
	uint32_t m_DescriptorTableBitMask;
};