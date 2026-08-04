#pragma once

#include "Buffer.h"

class VertexBuffer : public Buffer
{
public:
	void CreateViews(size_t numElements, size_t elementSize) override;

	/**
	* Get the SRV for a resource.
	*/
	D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView() const override;

	size_t GetNumVertices() const;
	size_t GetVertexStride() const;

	/*
	* Get the vertex buffer view for binding to the Input Assembler stage.
	*/
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;

protected:
	/**
	* Vertex buffers can only be created through the Device.
	*/
	VertexBuffer(Device& device, const std::wstring& name = L"VertexBuffer");
	virtual ~VertexBuffer() = default;

private:
	size_t m_NumVertices;
	size_t m_VertexStride;

	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
};