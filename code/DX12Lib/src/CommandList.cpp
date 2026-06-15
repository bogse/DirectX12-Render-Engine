#include "DX12LibPCH.h"

#include "CommandList.h"

#include "Application.h"
#include "CommandQueue.h"
#include "DynamicDescriptorHeap.h"
#include "GenerateMipsPSO.h"
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

void CommandList::TransitionBarrier(
	const Resource& resource,
	D3D12_RESOURCE_STATES stateAfter,
	UINT subResource,
	bool flushBarriers)
{
	TransitionBarrier(resource.GetD3D12Resource(), stateAfter, subResource, flushBarriers);
}

void CommandList::TransitionBarrier(
	Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	D3D12_RESOURCE_STATES stateAfter,
	UINT subResource,
	bool flushBarriers)
{
	if (resource)
	{
		// The "before" state is not important. It will be resolved by the resource state tracker.
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			resource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, subResource);
		m_ResourceStateTracker->ResourceBarrier(barrier);
	}

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::UAVBarrier(const Resource& resource, bool flushBarriers)
{
	const Microsoft::WRL::ComPtr<ID3D12Resource>& d3d12Resource = resource.GetD3D12Resource();

	if (d3d12Resource)
	{
		const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(d3d12Resource.Get());
		m_ResourceStateTracker->ResourceBarrier(barrier);
	}

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::AliasingBarrier(const Resource& beforeResource, const Resource& afterResource, bool flushBarriers)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12BeforeResource = beforeResource.GetD3D12Resource();
	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12AfterResource = afterResource.GetD3D12Resource();
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(
		d3d12BeforeResource.Get(), d3d12AfterResource.Get());

	m_ResourceStateTracker->ResourceBarrier(barrier);

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::AliasingBarrier(
	Microsoft::WRL::ComPtr<ID3D12Resource> beforeResource,
	Microsoft::WRL::ComPtr<ID3D12Resource> afterResource,
	bool flushBarriers)
{
	const CD3DX12_RESOURCE_BARRIER barrier =
		CD3DX12_RESOURCE_BARRIER::Aliasing(beforeResource.Get(), afterResource.Get());

	m_ResourceStateTracker->ResourceBarrier(barrier);

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
	CopyResource(destinationResource.GetD3D12Resource(), sourceResource.GetD3D12Resource());
}

void CommandList::CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> destinationResource, Microsoft::WRL::ComPtr<ID3D12Resource> sourceResource)
{
	TransitionBarrier(destinationResource, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionBarrier(sourceResource, D3D12_RESOURCE_STATE_COPY_SOURCE);
	FlushResourceBarriers();

	m_d3d12CommandList->CopyResource(destinationResource.Get(),
		sourceResource.Get());

	TrackObject(destinationResource);
	TrackObject(sourceResource);
}

void CommandList::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
{
	FlushResourceBarriers();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDispatch(*this);
	}

	m_d3d12CommandList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);
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
		ThrowIfFailed(DirectX::LoadFromDDSFile(filename.c_str(), DirectX::DDS_FLAGS_FORCE_RGB, &metadata, scratchImage));
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
			static_cast<UINT16>(metadata.arraySize));
		break;
	}
	case DirectX::TEX_DIMENSION_TEXTURE2D:
	{
		textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			metadata.format,
			static_cast<UINT64>(metadata.width),
			static_cast<UINT>(metadata.height),
			static_cast<UINT16>(metadata.arraySize));
		break;
	}
	case DirectX::TEX_DIMENSION_TEXTURE3D:
	{
		textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(
			metadata.format,
			static_cast<UINT64>(metadata.width),
			static_cast<UINT>(metadata.height),
			static_cast<UINT16>(metadata.depth));
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
	ResourceStateTracker::AddGlobalResourceState(textureResource.Get(), D3D12_RESOURCE_STATE_COMMON);

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

	if (subresources.size() < textureResource->GetDesc().MipLevels && textureResource->GetDesc().MipLevels > 1)
	{
		GenerateMips(texture);
	}

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

		TrackObject(intermediateResource);
		TrackObject(destinationResource);
	}
}

void CommandList::GenerateMips(const Texture& texture)
{
	if (m_d3d12CommandListType == D3D12_COMMAND_LIST_TYPE_COPY)
	{
		if (!m_ComputeCommandList)
		{
			m_ComputeCommandList = Application::GetInstance().
				GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE)->GetCommandList();
		}
		m_ComputeCommandList->GenerateMips(texture);
		return;
	}

	const Microsoft::WRL::ComPtr<ID3D12Resource>& resource = texture.GetD3D12Resource();

	if (!resource)
		return;

	D3D12_RESOURCE_DESC resourceDesc = resource->GetDesc();

	if (resourceDesc.MipLevels == 1)
		return;

	// Currently, only non-multi-sampled 2D textures are supported.
	if (resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
		resourceDesc.DepthOrArraySize != 1 ||
		resourceDesc.SampleDesc.Count > 1)
	{
		throw std::exception("GenerateMips is only supported for non-multi-sampled 2D Textures.");
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> uavResource = resource;

	// Create an alias of the original resource.
	// This is done to perform a GPU copy of resources with different formats.
	// BGR -> RGB texture copies will fail GPU validation unless performed
	// through an alias of the BGR resource in a placed heap.
	Microsoft::WRL::ComPtr<ID3D12Resource> aliasResource;

	// If the passed-in rsource does not allow for UAV access
	// the create a staging resource that is used to generate the mipmap chain.
	if (!texture.CheckUAVSupport() ||
		(resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
	{
		const Microsoft::WRL::ComPtr<ID3D12Device2>& device = Application::GetInstance().GetDevice();

		// Describe an alias resource that is used to copy the original texture.
		D3D12_RESOURCE_DESC aliasDesc = resourceDesc;
		
		// Placed resources can't be render targets or depth-stencil views.
		aliasDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		aliasDesc.Flags &= ~(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		// Describe a UAV compatible resource that is used to perform mipmapping of the original texture.
		D3D12_RESOURCE_DESC uavDesc = aliasDesc; // The flags for the UAV description must match that of the alias description.
		uavDesc.Format = Texture::GetUAVCompatibleFormat(resourceDesc.Format);

		D3D12_RESOURCE_DESC resourceDescriptions[] = {
			aliasDesc,
			uavDesc
		};

		// Create a heap that is large enough to store a copy of the original resource.
		const UINT numDescriptions = sizeof(resourceDescriptions) / sizeof(resourceDescriptions[0]);
		D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = device->GetResourceAllocationInfo(
			0, numDescriptions, resourceDescriptions
		);

		D3D12_HEAP_DESC heapDesc = {};
		heapDesc.SizeInBytes						= allocationInfo.SizeInBytes;
		heapDesc.Alignment							= allocationInfo.Alignment;
		heapDesc.Flags								= D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
		heapDesc.Properties.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapDesc.Properties.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;
		heapDesc.Properties.Type					= D3D12_HEAP_TYPE_DEFAULT;

		Microsoft::WRL::ComPtr<ID3D12Heap> heap;
		ThrowIfFailed(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)));

		// Make sure the heap does not go out of scope until the command
		// is finished executing the command queue.
		TrackObject(heap);

		// Create a placed resource that matches the description of the original resource.
		// This resource is used to copy the original texture to the UAV compatible resource.
		ThrowIfFailed(device->CreatePlacedResource(
			heap.Get(),
			0,
			&aliasDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&aliasResource)
		));

		ResourceStateTracker::AddGlobalResourceState(aliasResource.Get(), D3D12_RESOURCE_STATE_COMMON);
		//Ensure the scope of the alias resource.
		TrackObject(aliasResource); // TODO_BOG all of those were TrackResource

		// Create a UAV compatible resource in the same heap as the alias resource.
		ThrowIfFailed(device->CreatePlacedResource(
			heap.Get(),
			0,
			&uavDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&uavResource)
		));

		ResourceStateTracker::AddGlobalResourceState(uavResource.Get(), D3D12_RESOURCE_STATE_COMMON);
		// Ensure the scope of the UAV compatible resource.
		TrackObject(uavResource);

		// Add an aliasing barrier for the alias resource.
		AliasingBarrier(nullptr, aliasResource);

		// Copy the original resource to the alias resource.
		// This ensures GPU validation.
		CopyResource(aliasResource, resource); 

		// Add an aliasing barrier for the UAV compatible resource.
		AliasingBarrier(aliasResource, uavResource);
	}

	// Generate mips for the UAV compatible resource.
	GenerateMipsUAV(Texture(uavResource), Texture::IsSRGBFormat(resourceDesc.Format));

	if (aliasResource)
	{
		AliasingBarrier(uavResource, aliasResource);
		// Copy the alias resource back to the origin resource.
		CopyResource(resource, aliasResource);
	}
}

void CommandList::GenerateMipsUAV(const Texture& texture, bool isSRGB)
{
	if (!m_GenerateMipsPSO)
	{
		m_GenerateMipsPSO = std::make_unique<GenerateMipsPSO>();
	}

	m_d3d12CommandList->SetPipelineState(m_GenerateMipsPSO->GetPipelineState().Get());
	SetComputeRootSignature(m_GenerateMipsPSO->GetRootSignature());

	GenerateMipsCB generateMipsCB;
	generateMipsCB.IsSRGB = isSRGB;

	const Microsoft::WRL::ComPtr<ID3D12Resource>& resource = texture.GetD3D12Resource();
	D3D12_RESOURCE_DESC resourceDesc = resource->GetDesc();

	//// Create an SRV that uses the format of the original texture.
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//srvDesc.Format = isSRGB ? Texture::GetSRGBFormat(resourceDesc.Format) : resourceDesc.Format;
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // Only 2D textures are supported (this was checked in the calling function).
	//srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;

	for (uint32_t srcMip = 0; srcMip < resourceDesc.MipLevels - 1u; )
	{
		uint64_t srcWidth = resourceDesc.Width >> srcMip;
		uint64_t srcHeight = resourceDesc.Height >> srcMip;
		uint32_t dstWidth = static_cast<uint32_t>(srcWidth >> 1);
		uint32_t dstHeight = static_cast<uint32_t>(srcHeight >> 1);

		// 0b00(0): Both width and height are even.
		// 0b01(1): Width is odd, height is even.
		// 0b10(2): Width is even, height is odd.
		// 0b11(3): Both width and height are odd.
		generateMipsCB.SrcDimension = (srcHeight & 1) << 1 | (srcWidth & 1);

		// How many mipmap levels to compute this pass (max 4 mips per pass).
		DWORD mipCount;

		// The number of times we can half the size of the texture and get
		// exactly a 50% reduction in size.
		// A 1 bit in the width and height indicates an odd dimension.
		// The case where either the width or the height is exactly 1 is handled
		// as a special case (as the dimension does not require reduction).
		_BitScanForward(&mipCount, (dstWidth == 1 ? dstHeight : dstWidth) |
								   (dstHeight == 1 ? dstWidth : dstHeight));

		// Maximum number of mips to generate is 4.
		mipCount = std::min<DWORD>(4, mipCount + 1);

		// Clamp to total number of mips left over.
		mipCount = (srcMip + mipCount) >= resourceDesc.MipLevels ?
			resourceDesc.MipLevels - srcMip - 1 : mipCount;

		// Dimensions shound not reduce to 0.
		// This can happen if the width and height are not the same.
		dstWidth = std::max<DWORD>(1, dstWidth);
		dstHeight = std::max<DWORD>(1, dstHeight);

		generateMipsCB.SrcMipLevel = srcMip;
		generateMipsCB.NumMipLevels = mipCount;
		generateMipsCB.TexelSize.x = 1.f / static_cast<float>(dstWidth);
		generateMipsCB.TexelSize.y = 1.f / static_cast<float>(dstHeight);

		SetCompute32BitConstants(GenerateMips::GenerateMipsCB, generateMipsCB);

		SetShaderResourceView(GenerateMips::SrcMip, 0, texture,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, srcMip, 1);

		for (uint32_t mip = 0; mip < mipCount; ++mip)
		{
			//D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			//uavDesc.Format = resourceDesc.Format;
			//uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			//uavDesc.Texture2D.MipSlice = srcMip + mip + 1;
			uint32_t targetMip = srcMip + mip + 1;

			SetUnorderedAccessView(GenerateMips::OutMip, mip, texture,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, targetMip, 1);
		}

		// Pad any unused mip levels with a default UAV. Doing this keeps the DX12 runtime happy.
		if (mipCount < 4)
		{
			m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(
				GenerateMips::OutMip, mipCount, 4 - mipCount, m_GenerateMipsPSO->GetDefaultUAV());			
		}

		Dispatch(Math::DivideByMultiple(dstWidth, 8), Math::DivideByMultiple(dstHeight, 8));

		UAVBarrier(texture);

		srcMip += mipCount;
	}

	TransitionBarrier(texture, D3D12_RESOURCE_STATE_COMMON);
}

void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
	// Constant buffers must be 256-byte aligned.
	UploadBuffer::Allocation heapAllocation = m_UploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	memcpy(heapAllocation.CPU, bufferData, sizeInBytes);

	m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllocation.GPU);
}

void CommandList::SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
	m_d3d12CommandList->SetComputeRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
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

void CommandList::SetComputeRootSignature(const RootSignature& rootSignature)
{
	ID3D12RootSignature* d3d12RootSignature = rootSignature.GetRootSignature().Get();
	if (m_RootSignature != d3d12RootSignature)
	{
		m_RootSignature = d3d12RootSignature;

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_DynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
		}

		m_d3d12CommandList->SetComputeRootSignature(d3d12RootSignature);

		TrackObject(m_RootSignature);
	}
}

void CommandList::SetShaderResourceView(
	uint32_t rootParameterIndex,
	uint32_t descriptorOffset,
	const Texture& texture,
	D3D12_RESOURCE_STATES stateAfter,
	uint32_t firstSubresource,
	uint32_t numSubresources)
{
	if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for (uint32_t i = 0; i < numSubresources; ++i)
		{
			TransitionBarrier(texture, stateAfter, firstSubresource + i);
		}
	}
	else
	{
		TransitionBarrier(texture, stateAfter);
	}

	m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(
		rootParameterIndex, descriptorOffset, 1, texture.GetShaderResourceView());

	TrackResource(texture);
}

void CommandList::SetUnorderedAccessView(
	uint32_t rootParameterIndex,
	uint32_t descriptorOffset,
	const Texture& texture,
	D3D12_RESOURCE_STATES stateAfter,
	uint32_t firstSubresource,
	uint32_t numSubresources)
{
	if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for (uint32_t i = 0; i < numSubresources; ++i)
		{
			TransitionBarrier(texture, stateAfter, firstSubresource + i);
		}
	}
	else
	{
		TransitionBarrier(texture, stateAfter);
	}

	// TODO_BOG check if we need to hardcode 1 instead of numSubresources down here.
	m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(
		rootParameterIndex, descriptorOffset, numSubresources, texture.GetUnorderedAccessView(firstSubresource));

	TrackResource(texture);
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