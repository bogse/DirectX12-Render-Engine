#pragma once

#include "Buffer.h"

class IndexBuffer : public Buffer
{
public:
	IndexBuffer(const std::wstring& name = L"");
	virtual ~IndexBuffer();

	void CreateViews(size_t numElements, size_t elementSize) override;

	size_t GetNumIndicies() const;
	DXGI_FORMAT GetIndexFormat() const;

	/**
	* Get the index buffer view for binding to the Input Assembler stage.
	*/
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;

private:
	size_t m_NumIndicies;
	DXGI_FORMAT m_IndexFormat;

	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
};