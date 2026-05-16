#include "DX12LibPCH.h"

#include "CommandList.h"

#include "Application.h"
#include "DynamicDescriptorHeap.h"
#include "IndexBuffer.h"
#include "Resource.h"
#include "ResourceStateTracker.h"
#include "RootSignature.h"
#include "Texture.h"
#include "UploadBuffer.h"
#include "VertexBuffer.h"

#include <DDSTextureLoader12.h>
#include <WICTextureLoader12.h>

#include <d3dx12.h>

#include <filesystem>

std::map<std::wstring, ID3D12Resource*> CommandList::ms_TextureCache;
std::mutex CommandList::ms_TextureCacheMutex;

CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type)
	: m_d3d12CommandListType(type)
{
	Microsoft::WRL::ComPtr<ID3D12Device2> device = Application::GetInstance().GetDevice();

	ThrowIfFailed(device->CreateCommandAllocator(m_d3d12CommandListType, IID_PPV_ARGS(&m_d3d12CommandAllocator)));

	ThrowIfFailed(device->CreateCommandList(0, m_d3d12CommandListType, m_d3d12CommandAllocator.Get(),
		nullptr, IID_PPV_ARGS(&m_d3d12CommandList)));

	m_UploadBuffer = std::make_unique<UploadBuffer>();

	m_ResourceStateTracker = std::make_unique<ResourceStateTracker>();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DynamicDescriptorHeap[i] = std::make_unique<DynamicDescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
		m_DescriptorHeaps[i] = nullptr;
	}
}

CommandList::~CommandList()
{}

D3D12_COMMAND_LIST_TYPE CommandList::GetCommandListType() const
{
	return m_d3d12CommandListType;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandList::GetGraphicsCommandList() const
{
	return m_d3d12CommandList;
}

void CommandList::TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource, bool flushBarriers)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource = resource.GetD3D12Resource();
	if (d3d12Resource)
	{
		// The "before" state is not important. It will be resolved by the resource state tracker.
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, subResource);
		m_ResourceStateTracker->ResourceBarrier(barrier);
	}

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::FlushResourceBarriers()
{
	m_ResourceStateTracker->FlushResourceBarriers(*this);
}

void CommandList::TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> object)
{
	m_TrackedObjects.push_back(object);
}

void CommandList::TrackResource(const Resource& resource)
{
	TrackObject(resource.GetD3D12Resource());
}

void CommandList::CopyBuffer(Buffer& buffer, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
	Microsoft::WRL::ComPtr<ID3D12Device2> device = Application::GetInstance().GetDevice();

	size_t bufferSize = numElements * elementSize;

	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource;
	if (bufferSize == 0)
	{
		// This will result in a NULL resource (which may be desired to define a default resource.
	}
	else
	{
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC defaultResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);

		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&defaultResourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&d3d12Resource)));

		// Add the resource to the global resource state tracker.
		ResourceStateTracker::AddGlobalResourceState(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);

		if (bufferData != nullptr)
		{
			// Create an upload resource to use as an intermediate buffer to copy the buffer resource
			Microsoft::WRL::ComPtr<ID3D12Resource> uploadResource;
			CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			CD3DX12_RESOURCE_DESC uploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

			ThrowIfFailed(device->CreateCommittedResource(
				&uploadHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&uploadResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadResource)));

			D3D12_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pData = bufferData;
			subresourceData.RowPitch = bufferSize;
			subresourceData.SlicePitch = subresourceData.RowPitch;

			UpdateSubresources(m_d3d12CommandList.Get(), d3d12Resource.Get(), uploadResource.Get(), 0, 0, 1, &subresourceData);

			// Add references to resources so they stay in scope until the command list is reset.
			TrackObject(uploadResource);
		}
		TrackObject(d3d12Resource);
	}

	buffer.SetD3D12Resource(d3d12Resource);
	buffer.CreateViews(numElements, elementSize);
}

void CommandList::CopyVertexBuffer(VertexBuffer& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData)
{
	CopyBuffer(vertexBuffer, numVertices, vertexStride, vertexBufferData);
}

void CommandList::CopyIndexBuffer(IndexBuffer& indexBuffer, size_t numIndices, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
	size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
	CopyBuffer(indexBuffer, numIndices, indexSizeInBytes, indexBufferData);
}

void CommandList::LoadTextureFromFile(Texture& texture, const std::wstring& filename)
{
	Microsoft::WRL::ComPtr<ID3D12Device2> device = Application::GetInstance().GetDevice();

	std::filesystem::path filepath(filename);

	if (!std::filesystem::exists(filepath))
	{
		throw std::invalid_argument("Invalid filename specified.");
	}

	std::map<std::wstring, ID3D12Resource*>::iterator iter =
		ms_TextureCache.find(filename);

	if (iter != ms_TextureCache.end())
	{
		texture.SetD3D12Resource(iter->second);
	}
	else
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
		std::unique_ptr<uint8_t[]> textureData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresourceData;

		if (filepath.extension() == ".dds")
		{
			// Use DDS texture loaded.
			ThrowIfFailed(DirectX::LoadDDSTextureFromFile(device.Get(),
				filename.c_str(), &textureResource, textureData, subresourceData));
		}
		else
		{
			D3D12_SUBRESOURCE_DATA resourceData;
			// Use WIC texture loader.
			ThrowIfFailed(DirectX::LoadWICTextureFromFile(device.Get(),
				filename.c_str(), &textureResource, textureData, resourceData));

			subresourceData.push_back(resourceData);
		}

		// Update the global state tracker.
		ResourceStateTracker::AddGlobalResourceState(textureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);

		texture.SetD3D12Resource(textureResource);
		texture.CreateViews();

		CopyTextureSubresource(texture, 0, static_cast<uint32_t>(subresourceData.size()),
			subresourceData.data());

		// Add the texture resource to the texture cache.
		std::lock_guard<std::mutex> lock(ms_TextureCacheMutex);
		ms_TextureCache[filename] = textureResource.Get();
	}
}

void CommandList::CopyTextureSubresource(
	Texture& texture,
	uint32_t firstSubresource,
	uint32_t numSubresources,
	D3D12_SUBRESOURCE_DATA* subresourceData)
{
	Microsoft::WRL::ComPtr<ID3D12Device2> device = Application::GetInstance().GetDevice();
	Microsoft::WRL::ComPtr<ID3D12Resource> destinationResource = texture.GetD3D12Resource();

	if (destinationResource)
	{
		// Resource must be in the copy-destination state.
		TransitionBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST, true);

		UINT64 requiredSize = GetRequiredIntermediateSize(destinationResource.Get(),
			firstSubresource, numSubresources);

		// Create a temporary (intermediate) resource for uploading the subresources.
		CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);

		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&intermediateResource)
		));

		UpdateSubresources(m_d3d12CommandList.Get(), destinationResource.Get(),
			intermediateResource.Get(), 0, firstSubresource, numSubresources, subresourceData);

		m_TrackedObjects.push_back(intermediateResource);
		m_TrackedObjects.push_back(destinationResource);
	}
}

void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
	// Constant buffers must be 256-byte aligned.
	UploadBuffer::Allocation heapAllocation = m_UploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	memcpy(heapAllocation.CPU, bufferData, sizeInBytes);

	m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllocation.GPU);
}

void CommandList::SetVertexBuffer(uint32_t slot, const VertexBuffer& vertexBuffer)
{
	TransitionBarrier(vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = vertexBuffer.GetVertexBufferView();

	m_d3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);

	TrackResource(vertexBuffer);
}

void CommandList::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
{
	size_t bufferSize = numVertices * vertexSize;

	UploadBuffer::Allocation heapAllocation = m_UploadBuffer->Allocate(bufferSize, vertexSize);
	memcpy(heapAllocation.CPU, vertexBufferData, bufferSize);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = heapAllocation.GPU;
	vertexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
	vertexBufferView.StrideInBytes = static_cast<UINT>(vertexSize);

	m_d3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void CommandList::SetIndexBuffer(const IndexBuffer& indexBuffer)
{
	TransitionBarrier(indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	D3D12_INDEX_BUFFER_VIEW indexBufferView = indexBuffer.GetIndexBufferView();

	m_d3d12CommandList->IASetIndexBuffer(&indexBufferView);

	TrackResource(indexBuffer);
}

void CommandList::SetDynamicIndexBuffer(size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
	size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
	size_t bufferSize = numIndicies * indexSizeInBytes;

	UploadBuffer::Allocation heapAllocation = m_UploadBuffer->Allocate(bufferSize, indexSizeInBytes);
	memcpy(heapAllocation.CPU, indexBufferData, bufferSize);

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = heapAllocation.GPU;
	indexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
	indexBufferView.Format = indexFormat;

	m_d3d12CommandList->IASetIndexBuffer(&indexBufferView);
}

void CommandList::SetViewport(const D3D12_VIEWPORT& viewport)
{
	SetViewports({ viewport });
}

void CommandList::SetViewports(const std::vector<D3D12_VIEWPORT>& viewports)
{
	assert(viewports.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
	m_d3d12CommandList->RSSetViewports(static_cast<UINT>(viewports.size()), viewports.data());
}

void CommandList::SetScissorRect(const D3D12_RECT& scissorRect)
{
	SetScissorRects({ scissorRect });
}

void CommandList::SetScissorRects(const std::vector<D3D12_RECT>& scissorRects)
{
	assert(scissorRects.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
	m_d3d12CommandList->RSSetScissorRects(static_cast<UINT>(scissorRects.size()), scissorRects.data());
}

void CommandList::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState)
{
	m_d3d12CommandList->SetPipelineState(pipelineState.Get());

	TrackObject(pipelineState);
}

void CommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	m_d3d12CommandList->IASetPrimitiveTopology(primitiveTopology);
}

void CommandList::ClearRTV(D3D12_CPU_DESCRIPTOR_HANDLE& rtv, FLOAT* clearColor)
{
	m_d3d12CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void CommandList::ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE& dsv, FLOAT depth)
{
	m_d3d12CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void CommandList::SetGraphicsRootSignature(const RootSignature& rootSignature)
{
	ID3D12RootSignature* d3d12RootSignature = rootSignature.GetRootSignature().Get();
	if (m_RootSignature != d3d12RootSignature)
	{
		m_RootSignature = d3d12RootSignature;

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_DynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
		}

		m_d3d12CommandList->SetGraphicsRootSignature(m_RootSignature);

		TrackObject(m_RootSignature);
	}
}

void CommandList::SetShaderResourceView(
	uint32_t rootParameterIndex,
	uint32_t descriptorOffset,
	const Resource& resource,
	D3D12_RESOURCE_STATES stateAfter)
{
	TransitionBarrier(resource, stateAfter);

	D3D12_CPU_DESCRIPTOR_HANDLE srv =
	{
		resource.GetShaderResourceView()
	};

	m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->
		StageDescriptors(rootParameterIndex, descriptorOffset, 1, srv);
}

void CommandList::SetRenderTargets(const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs,
								   const D3D12_CPU_DESCRIPTOR_HANDLE* dsv,
								   UINT numRTVs)
{
	m_d3d12CommandList->OMSetRenderTargets(numRTVs, rtvs, FALSE, dsv);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
	uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
	FlushResourceBarriers();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
	}

	m_d3d12CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

bool CommandList::Close(CommandList& pendingCommandList)
{
	// Flush any remaining barriers.
	FlushResourceBarriers();

	m_d3d12CommandList->Close();

	// Flush pending resource barriers.
	uint32_t numPendingBarriers = m_ResourceStateTracker->FlushPendingResourceBarriers(pendingCommandList);
	// Commit the final resource state to the global state.
	m_ResourceStateTracker->CommitFinalResourceStates();

	return numPendingBarriers > 0;
}

void CommandList::Close()
{
	FlushResourceBarriers();
	m_d3d12CommandList->Close();
}

void CommandList::Reset()
{
	ThrowIfFailed(m_d3d12CommandAllocator->Reset());
	ThrowIfFailed(m_d3d12CommandList->Reset(m_d3d12CommandAllocator.Get(), nullptr));

	m_ResourceStateTracker->Reset();
	m_UploadBuffer->Reset();
	ReleaseTrackedObjects();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DynamicDescriptorHeap[i]->Reset();
		m_DescriptorHeaps[i] = nullptr;
	}

	m_RootSignature = nullptr;
}

void CommandList::ReleaseTrackedObjects()
{
	m_TrackedObjects.clear();
}

void CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap)
{
	if (m_DescriptorHeaps[heapType] != heap)
	{
		m_DescriptorHeaps[heapType] = heap;
		BindDescriptorHeaps();
	}
}

void CommandList::BindDescriptorHeaps()
{
	UINT numDescriptorHeaps = 0;
	ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		ID3D12DescriptorHeap* descriptorHeap = m_DescriptorHeaps[i];
		if (descriptorHeap)
		{
			descriptorHeaps[numDescriptorHeaps++] = descriptorHeap;
		}
	}

	m_d3d12CommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
}