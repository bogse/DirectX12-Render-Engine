/**
* CommandList class encapsulates a ID3D12GraphicsCommandList2 interface.
* The CommandList class provides additional functionality that makes working
* with DirectX 12 applications easier.
*/

#pragma once

#include <d3dx12.h>
#include <wrl.h>

#include <map>
#include <memory>
#include <mutex>
#include <vector>

class Buffer;
class ConstantBuffer;
class DynamicDescriptorHeap;
class GenerateMipsPSO;
class IndexBuffer;
class RenderTarget;
class Resource;
class ResourceStateTracker;
class RootSignature;
class Texture;
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
	void TransitionBarrier(
		const Resource& resource,
		D3D12_RESOURCE_STATES stateAfter,
		UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		bool flushBarriers = false);
	
	void TransitionBarrier(
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES stateAfter,
		UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		bool flushBarriers = false);

	/**
	* Add a UAV barrier tro ensure that any writes to a resource have completed
	* before reading from the resource.
	* 
	* @param resource The resource to add a UAV barrier for.
	* @param flushBarriers Force flush any barriers. Resource barriers need to be
	* flushed before a command (draw, dispatch, or copy) that expects the resource
	* to be in a particular state can run.
	*/
	void UAVBarrier(const Resource& resource, bool flushBarriers = false);

	/**
	* Add an aliasing barrier to indicate a transition between usages of two
	* different resources that occupy the same space in a heap.
	* 
	* @param beforeResource The resource that currently occupies the heap.
	* @param afterResource The resource that will occupy the space in the heap.
	*/
	void AliasingBarrier(
		const Resource& beforeResource,
		const Resource& afterResource,
		bool flushBarriers = false);

	void AliasingBarrier(
		Microsoft::WRL::ComPtr<ID3D12Resource> beforeResource,
		Microsoft::WRL::ComPtr<ID3D12Resource> afterResource,
		bool flushBarriers = false);

	/**
	* Flush any barriers that have been pushed to the command list.
	*/
	void FlushResourceBarriers();

	/**
	* Copy resources.
	*/
	void CopyResource(const Resource& destinationResource, const Resource& sourceResource);
	void CopyResource(
		Microsoft::WRL::ComPtr<ID3D12Resource> destinationResource,
		Microsoft::WRL::ComPtr<ID3D12Resource> sourceResource);

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
		CopyIndexBuffer(indexBuffer, indexBufferData.size(), indexFormat, indexBufferData.data());
	}

	/**
	* Load a texture by a filename.
	*/
	void LoadTextureFromFile(Texture& texture, const std::wstring& filename, const bool sRGB = false);

	/**
	* Copy a subresource data to a texture.
	*/
	void CopyTextureSubresource(Texture& texture, uint32_t firstSubresource,
		uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);

	/**
	* Generate mips for the texture.
	* The first subresource is used to generate the mip chain.
	* Mips are automatically generated for textures loaded from files.
	*/
	void GenerateMips(const Texture& texture);

	/**
	* Generate mips for UAV compatible textures.
	*/
	void GenerateMipsUAV(const Texture& texture, bool isSRGB);

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
	* Set a set of 32-bit constants on the compute pipeline.
	*/
	void SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants);
	template<typename T>
	void SetCompute32BitConstants(uint32_t rootParameterIndex, const T& constants)
	{
		static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
		SetCompute32BitConstants(rootParameterIndex, sizeof(T) / sizeof(uint32_t), &constants);
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
	* Clear the render target texture.
	*/
	void ClearRenderTargetTexture(const Texture& texture, const float clearColor[4]);
	
	/*
	* Clear the depth/stencil texture.
	*/
	void ClearDepthStencilTexture(const Texture& texture,
		D3D12_CLEAR_FLAGS clearFlags, float depth = 1.0f, uint8_t stencil = 0);

	/**
	* Set the current root signature on the command list.
	*/
	void SetGraphicsRootSignature(const RootSignature& rootSignature);
	void SetComputeRootSignature(const RootSignature& rootSignature);

	/**
	* Set the SRV on the graphics pipeline.
	*/
	void SetShaderResourceView(
		uint32_t rootParameterIndex,
		uint32_t descriptorOffset,
		const Texture& texture,
		D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
										   D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		uint32_t firstSubresource		 = 0,
		uint32_t numSubresources		 = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	/**
	* Set the UAV on the graphics pipeline.
	*/
	void SetUnorderedAccessView(
		uint32_t rootParameterIndex,
		uint32_t descriptorOffset,
		const Texture& texture,
		D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		uint32_t firstSubresource		 = 0,
		uint32_t numSubresources		 = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	/**
	* Bind the Render Target for the graphics rendering pipeline.
	*/
	void SetRenderTarget(const RenderTarget& renderTarget);

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

	std::shared_ptr<CommandList> GetComputeCommandList() const
	{
		return m_ComputeCommandList;
	}

	/**
	* Dispatch a compute shader.
	*/
	void Dispatch(uint32_t numGroupsX, uint32_t numGroupsY = 1, uint32_t numGroupsZ = 1);

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
	* Mips can't be generated of copy queues but must be generated on compute or direct queues.
	* In this case, a Compute command list is generated and executed after the copy queue
	* is finished uploading the first sub resource.
	*/
	std::shared_ptr<CommandList> m_ComputeCommandList;

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
	* Pipeline state object for Mip map generation.
	*/
	std::unique_ptr<GenerateMipsPSO> m_GenerateMipsPSO;

	/**
	* Objects that are being referenced by a command list that is "in-flight" on
	* the command-queue cannot be deleted. To ensure objects are not deleted
	* until the command list is finished executing, a reference to the object
	* is stored. The referenced objects are released when the command list is reset.
	*/
	TrackedObjects m_TrackedObjects;

	/**
	* Keep track of loaded textures to avoid loading the same texture multiple times.
	*/
	static std::map<std::wstring, ID3D12Resource*> ms_TextureCache;
	static std::mutex ms_TextureCacheMutex;
};
