/**
* CommandList class encapsulates a ID3D12GraphicsCommandList2 interface.
* The CommandList class provides additional functionality that makes working
* with DirectX 12 applications easier.
*/

#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <map>
#include <memory>
#include <mutex>
#include <vector>

class Buffer;
class ConstantBuffer;
class DynamicDescriptorHeap;
class IndexBuffer;
class Resource;
class ResourceStateTracker;
class RootSignature;
class UploadBuffer;
class VertexBuffer;

class CommandList
{
public:
	CommandList(D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandList();

	D3D12_COMMAND_LIST_TYPE GetCommandListType() const;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetGraphicsCommandList() const;

	/**
	* Transition a resource to a particular state.
	* 
	* @param resource The resource to transition.
	* #param state The state to transition the resource to.
	* @param flushBarriers Force flush any barriers. Resource barriers need to be
	* flushed before a command (draw, dispatch, or copy) that expects the resource
	* to be in a particular state to run.
	*/
	void TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, 
		UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);

	/**
	* Flush any barriers that have been pushed to the command list.
	*/
	void FlushResourceBarriers();

	/**
	* Copy the contents of a vertex buffer.
	*/
	void CopyVertexBuffer(VertexBuffer& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData);
	template<typename T>
	void CopyVertexBuffer(VertexBuffer& vertexBuffer, const std::vector<T>& vertexBufferData)
	{
		CopyVertexBuffer(vertexBuffer, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
	}

	/**
	* Copy the contents of an index buffer.
	*/
	void CopyIndexBuffer(IndexBuffer& indexBuffer, size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData);
	template<typename T>
	void CopyIndexBuffer(IndexBuffer& indexBuffer, const std::vector<T>& indexBufferData)
	{
		static_assert(sizeof(T) == 2 || sizeof(T) == 4);

		DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		SetIndexBuffer(indexBuffer, indexBufferData.size(), indexFormat, indexBufferData.data());
	}

	/**
	* Set a dynamic constant buffer data to an inline descriptor in the root signature.
	*/
	void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);
	template<typename T>
	void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, const T& data)
	{
		SetGraphicsDynamicConstantBuffer(rootParameterIndex, sizeof(T), &data);
	}

	/**
	* Set the vertex buffer to the rendering pipeline.
	*/
	void SetVertexBuffer(uint32_t slot, const VertexBuffer& vertexBuffer);

	/**
	* Set dynamic vertex buffer data to the rendering pipeline.
	*/
	void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData);
	template<typename T>
	void SetDynamicVertexBuffer(uint32_t slot, const std::vector<T>& vertexBufferData)
	{
		SetDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
	}

	/**
	* Set the index buffer to the rendering pipeline.
	*/
	void SetIndexBuffer(const IndexBuffer& indexBuffer);

	/**
	* Set dynamic index buffer data to the rendering pipeline.
	*/
	void SetDynamicIndexBuffer(size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData);
	template<typename T>
	void SetDynamicIndexBuffer(const std::vector<T>& indexBufferData)
	{
		static_assert(sizeof(T) == 2 || sizeof(T) == 4);

		DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		SetDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.data());
	}

	/**
	* Set viewports.
	*/
	void SetViewport(const D3D12_VIEWPORT& viewport);
	void SetViewports(const std::vector<D3D12_VIEWPORT>& viewports);

	/**
	* Set scrissor rects.
	*/
	void SetScissorRect(const D3D12_RECT& scissorRect);
	void SetScissorRects(const std::vector<D3D12_RECT>& scissorRect);

	/**
	* Set the pipeline state object on the command list.
	*/
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState);

	/**
	* Set the current primitive topology for the rendering pipeline.
	*/
	void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);

	/*
	* Clear render target view.
	*/
	void ClearRTV(D3D12_CPU_DESCRIPTOR_HANDLE& rtv, FLOAT* clearColor);
	
	/*
	* Clear the depth of a depth-stencil view.
	*/
	void ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE& dsv, FLOAT depth = 1.0f);

	/**
	* Set the current root signature on the command list.
	*/
	void SetGraphicsRootSignature(const RootSignature& rootSignature);

	/**
	* Bind the Render Targets to the Output Merger stage.
	*/
	void SetRenderTargets(const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs, 
						  const D3D12_CPU_DESCRIPTOR_HANDLE* dsv,
						  UINT numRTVs = 1);

	/**
	* Draw geometry.
	*/
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, 
		uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0);

	/**
	* Close the command list.
	* Used by the command queue.
	* 
	* @param pendingCommandList The command list that is used to execute pending
	* resource barriers (if any) for this command list.
	*/
	bool Close(CommandList& pendingCommandList);
	// Just close the command list. This is useful for pending command lists.
	void Close();

	/**
	* Reset the command list. This should only be called by the CommandQueue
	* before the command list is returned from CommandQueue::GetCommandList.
	*/
	void Reset();

	/**
	* Release tracked objects. Useful if the swap chain needs to be resized.
	*/
	void ReleaseTrackedObjects();

	/**
	* Set the currently bound descriptor heap.
	* Should only be called by the DynamicDescriptorHeap class.
	*/
	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);

private:
	void TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> object);
	void TrackResource(const Resource& resource);

	/**
	* Copy the contents of a buffer (possibly replacing the previous buffer contents.
	*/
	void CopyBuffer(Buffer& buffer, size_t numElements, size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	/**
	* Binds the current descriptor heaps to the command list.
	*/
	void BindDescriptorHeaps();

	using TrackedObjects = std::vector<Microsoft::WRL::ComPtr<ID3D12Object>>;

	D3D12_COMMAND_LIST_TYPE m_d3d12CommandListType;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_d3d12CommandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_d3d12CommandAllocator;

	/**
	* Keep track of the currently bound root signatures to minimize root signature changes.
	*/
	ID3D12RootSignature* m_RootSignature;

	/**
	* Resource created in an upload heap. Useful for drawing of dynamic geometry
	* or for uploading constant buffer data that changes every draw call.
	*/
	std::unique_ptr<UploadBuffer> m_UploadBuffer;

	/*
	* Resource state tracker is used by the command list to track (per command list)
	* the current state of a resource. The resource state tracker also tracks the 
	* global state of a resource in order to minimize resource state transitions.
	*/
	std::unique_ptr<ResourceStateTracker> m_ResourceStateTracker;

	/**
	* The dynamic descriptor heap allows for descriptors to be staged before
	* being committed to the command list. Dynamic descriptors need to be 
	* committed before a Draw or Dispatch.
	*/
	std::unique_ptr<DynamicDescriptorHeap> m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	/**
	* Keep track of the currently bound descriptor heaps. Only change descriptor
	* heaps if they are different than the currently bound descriptor heaps.
	*/
	ID3D12DescriptorHeap* m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	/**
	* Objects that are being referenced by a command list that is "in-flight" on
	* the command-queue cannot be deleted. To ensure objects are not deleted
	* until the command list is finished executing, a reference to the object
	* is stored. The referenced objects are released when the command list is reset.
	*/
	TrackedObjects m_TrackedObjects;
};