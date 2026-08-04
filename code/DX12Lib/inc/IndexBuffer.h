#pragma once

#include "Buffer.h"

class IndexBuffer : public Buffer
{
public:
	void CreateViews(size_t numElements, size_t elementSize) override;

	/**
	* Get the SRV for a resource.
	*/
	D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView() const override;

	size_t GetNumIndicies() const;
	DXGI_FORMAT GetIndexFormat() const;

	/**
	* Get the index buffer view for binding to the Input Assembler stage.
	*/
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;

protected:
	/**
	* Index buffers can only be created through the Device.
	*/
	IndexBuffer(Device& device, const std::wstring& name = L"IndexBuffer");

	virtual ~IndexBuffer() = default;

private:
	size_t m_NumIndicies;
	DXGI_FORMAT m_IndexFormat;

	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
};