#pragma once

#include "Buffer.h"

class VertexBuffer : public Buffer
{
public:
	VertexBuffer(const std::wstring& name = L"");
	virtual ~VertexBuffer();

	void CreateViews(size_t numElements, size_t elementSize) override;

	size_t GetNumVertices() const;
	size_t GetVertexStride() const;

	/*
	* Get the vertex buffer view for binding to the Input Assembler stage.
	*/
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;

private:
	size_t m_NumVertices;
	size_t m_VertexStride;

	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
};