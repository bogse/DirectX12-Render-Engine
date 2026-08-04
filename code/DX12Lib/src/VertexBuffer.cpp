#include "DX12LibPCH.h"

#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(Device& device, const std::wstring& name)
	: Buffer(device, name)
	, m_NumVertices(0)
	, m_VertexStride(0)
	, m_VertexBufferView({})
{}

void VertexBuffer::CreateViews(size_t numElements, size_t elementSize)
{
	m_NumVertices = numElements;
	m_VertexStride = elementSize;

	m_VertexBufferView.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = static_cast<UINT>(m_NumVertices * m_VertexStride);
	m_VertexBufferView.StrideInBytes = static_cast<UINT>(m_VertexStride);
}

D3D12_CPU_DESCRIPTOR_HANDLE VertexBuffer::GetShaderResourceView() const
{
	throw std::exception("VertexBuffer::GetShaderResourceView should not be called.");
}

size_t VertexBuffer::GetNumVertices() const
{
	return m_NumVertices;
}

size_t VertexBuffer::GetVertexStride() const
{
	return m_VertexStride;
}

D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetVertexBufferView() const
{
	return m_VertexBufferView;
}
