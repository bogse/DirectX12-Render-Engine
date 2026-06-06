#include "DX12LibPCH.h"

#include "CommandList.h"

#include "Application.h"
#include "DynamicDescriptorHeap.h"
#include "IndexBuffer.h"
#include "RenderTarget.h"
#include "Resource.h"
#include "ResourceStateTracker.h"
#include "RootSignature.h"
#include "Texture.h"
#include "UploadBuffer.h"
#include "VertexBuffer.h"

#include <filesystem>

std::map<std::wstring, ID3D12Resource*> CommandList::ms_TextureCache;
std::mutex CommandList::ms_TextureCacheMutex;

CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type)
	: m_d3d12CommandListType(type)
	, m_RootSignature(nullptr)
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

void CommandList::CopyResource(const Resource& destinationResource, const Resource& sourceResource)
{
	TransitionBarrier(destinationResource, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionBarrier(sourceResource, D3D12_RESOURCE_STATE_COPY_SOURCE);
	FlushResourceBarriers();

	m_d3d12CommandList->CopyResource(destinationResource.GetD3D12Resource().Get(),
		sourceResource.GetD3D12Resource().Get());

	m_TrackedObjects.push_back(destinationResource.GetD3D12Resource());
	m_TrackedObjects.push_back(sourceResource.GetD3D12Resource());
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

			m_ResourceStateTracker->TransitionResource(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
			FlushResourceBarriers();

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

void CommandList::LoadTextureFromFile(
	Texture& texture,
	const std::wstring& filename,
	const bool sRGB)
{
	std::filesystem::path filepath(filename);

	if (!std::filesystem::exists(filepath))
	{
		throw std::invalid_argument("Invalid filename specified.");
	}

	std::lock_guard<std::mutex> lock(ms_TextureCacheMutex);

	std::map<std::wstring, ID3D12Resource*>::iterator iter =
		ms_TextureCache.find(filename);

	if (iter != ms_TextureCache.end())
	{
		texture.SetD3D12Resource(iter->second);
		texture.CreateViews();
		texture.SetName(filename);
		return;
	}

	DirectX::TexMetadata metadata;
	DirectX::ScratchImage scratchImage;

	if (filepath.extension() == ".dds")
	{
		ThrowIfFailed(DirectX::LoadFromDDSFile(filename.c_str(),
			DirectX::DDS_FLAGS_FORCE_RGB, &metadata, scratchImage));
	}
	else if (filepath.extension() == ".hdr")
	{
		ThrowIfFailed(DirectX::LoadFromHDRFile(filename.c_str(), &metadata, scratchImage));
	}
	else if (filepath.extension() == ".tga")
	{
		ThrowIfFailed(DirectX::LoadFromTGAFile(filename.c_str(), &metadata, scratchImage));
	}
	else
	{
		ThrowIfFailed(DirectX::LoadFromWICFile(filename.c_str(),
			DirectX::WIC_FLAGS_FORCE_RGB, &metadata, scratchImage));
	}

	// Force the texture format to be sRGB to convert to linear when sampling the texture in shader.
	if (sRGB)
	{
		metadata.format = DirectX::MakeSRGB(metadata.format);
	}

	D3D12_RESOURCE_DESC textureDesc = {};
	switch (metadata.dimension)
	{
	case DirectX::TEX_DIMENSION_TEXTURE1D:
	{
		textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(
			metadata.format,
			static_cast<UINT64>(metadata.width),
			static_cast<UINT16>(metadata.arraySize),
			static_cast<UINT16>(metadata.mipLevels));
		break;
	}
	case DirectX::TEX_DIMENSION_TEXTURE2D:
	{
		textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			metadata.format,
			static_cast<UINT64>(metadata.width),
			static_cast<UINT>(metadata.height),
			static_cast<UINT16>(metadata.arraySize),
			static_cast<UINT16>(metadata.mipLevels));
		break;
	}
	case DirectX::TEX_DIMENSION_TEXTURE3D:
	{
		textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(
			metadata.format,
			static_cast<UINT64>(metadata.width),
			static_cast<UINT>(metadata.height),
			static_cast<UINT16>(metadata.depth),
			static_cast<UINT16>(metadata.mipLevels));
		break;
	}
	default:
		throw std::exception("Invalid texture dimension.");
		break;
	}

	const Microsoft::WRL::ComPtr<ID3D12Device2>& d3d12Device = Application::GetInstance().GetDevice();

	CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_DEFAULT);
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

	ThrowIfFailed(d3d12Device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&textureResource)));

	texture.SetD3D12Resource(textureResource);
	texture.SetName(filename);
	texture.CreateViews();

	// Update the global state tracker.
	ResourceStateTracker::AddGlobalResourceState(textureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
	auto x = textureResource->GetDesc().MipLevels;
	// Upload data
	const size_t imageCount = scratchImage.GetImageCount();
	std::vector<D3D12_SUBRESOURCE_DATA> subresources(imageCount);
	const DirectX::Image* pImages = scratchImage.GetImages();

	if (imageCount == 0 || pImages == nullptr)
	{
		throw std::runtime_error("Failed to retrieve image data from scratch image.");
	}

	for (size_t i = 0; i < imageCount; ++i)
	{
		D3D12_SUBRESOURCE_DATA& subresource = subresources[i];
		subresource.RowPitch				= pImages[i].rowPitch;
		subresource.SlicePitch				= pImages[i].slicePitch;
		subresource.pData					= pImages[i].pixels;
	}

	CopyTextureSubresource(texture, 0, static_cast<uint32_t>(subresources.size()),
		subresources.data());

	// Add the texture resource to the texture cache.
	ms_TextureCache[filename] = textureResource.Get();
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

void CommandList::ClearRenderTargetTexture(const Texture& texture, const float clearColor[4])
{
	TransitionBarrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);

	m_d3d12CommandList->ClearRenderTargetView(texture.GetRenderTargetView(),
		clearColor, 0, nullptr);

	TrackResource(texture);
}

void CommandList::ClearDepthStencilTexture(
	const Texture& texture,
	D3D12_CLEAR_FLAGS clearFlags,
	float depth,
	uint8_t stencil)
{
	TransitionBarrier(texture, D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

	m_d3d12CommandList->ClearDepthStencilView(texture.GetDepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);

	TrackResource(texture);
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

void CommandList::SetRenderTarget(const RenderTarget& renderTarget)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetDescriptors;
	renderTargetDescriptors.reserve(static_cast<size_t>(AttachmentPoint::NumAttachmentPoints));

	const RenderTarget::TextureArray& textures = renderTarget.GetTextures();

	constexpr int minColorSlot = static_cast<int>(AttachmentPoint::MinColorSlot);
	constexpr int maxColorSlot = static_cast<int>(AttachmentPoint::MaxColorSlot);
	for (int i = minColorSlot; i <= maxColorSlot; ++i)
	{
		const std::shared_ptr<Texture>& texture = textures[i];

		if (texture && texture->GetD3D12Resource())
		{
			TransitionBarrier(*texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
			renderTargetDescriptors.push_back(texture->GetRenderTargetView());

			TrackResource(*texture);
		}
	}

	const std::shared_ptr<Texture>& depthTexture =
		renderTarget.GetTexture(AttachmentPoint::DepthStencil);

	CD3DX12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor(D3D12_DEFAULT);
	if (depthTexture && depthTexture->GetD3D12Resource())
	{
		TransitionBarrier(*depthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		depthStencilDescriptor = depthTexture->GetDepthStencilView();

		TrackResource(*depthTexture);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = depthStencilDescriptor.ptr != 0 ? &depthStencilDescriptor : nullptr;

	m_d3d12CommandList->OMSetRenderTargets(static_cast<UINT>(renderTargetDescriptors.size()),
		renderTargetDescriptors.data(), FALSE, pDSV);
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