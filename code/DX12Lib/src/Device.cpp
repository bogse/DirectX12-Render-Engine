#include "DX12LibPCH.h"

#include "Device.h"

#include "Adapter.h"
#include "CommandQueue.h"
#include "DescriptorAllocator.h"
#include "SwapChain.h"

namespace
{
	// Local pass-through helper class to allow std::make_shared
	// to instantiate objects with protected constructors.

	class MakeDevice : public Device
	{
	public:
		MakeDevice(std::shared_ptr<Adapter> adapter)
			: Device(adapter)
		{
		}
	};

	class MakeSwapChain : public SwapChain
	{
	public:
		MakeSwapChain(Device& device, HWND hWnd)
			: SwapChain(device, hWnd)
		{}
	};
}

std::shared_ptr<Device> Device::Create(std::shared_ptr<Adapter> adapter)
{
	return std::make_shared<MakeDevice>(adapter);
}

std::shared_ptr<SwapChain> Device::CreateSwapChain(HWND hWnd)
{
	return std::make_shared<MakeSwapChain>(*this, hWnd);
}

Device::Device(std::shared_ptr<Adapter> adapter)
	: m_Adapter(adapter)
{
	if (!m_Adapter)
	{
		m_Adapter = Adapter::Create();
		assert(m_Adapter);
	}

	Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter = m_Adapter->GetDXGIAdapter();

	ThrowIfFailed(D3D12CreateDevice(
		dxgiAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_d3d12Device)));

	// Enable debug messages (only works if the debug layer has already been enabled).
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(m_d3d12Device.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress whole categories of messages
		// D3D12_MESSAGE_CATEGORY catergories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID denyIds[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,	// I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,							// This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,						// This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER newFilter = {};
		// newFilter.DenyList.NumCategories = _countof(categories);
		// newFilter.DenyList.pCatergoyList = categories;
		newFilter.DenyList.NumSeverities = _countof(severities);
		newFilter.DenyList.pSeverityList = severities;
		newFilter.DenyList.NumIDs = _countof(denyIds);
		newFilter.DenyList.pIDList = denyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&newFilter));
	}
}

DescriptorAllocation Device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
	return m_DescriptorAllocators[type]->Allocate(numDescriptors);
}

void Device::ReleaseStaleDescriptors(uint64_t finishedFrame)
{
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DescriptorAllocators[i]->ReleaseStaleDescriptors(finishedFrame);
	}
}

void Device::Flush()
{
	m_DirectCommandQueue->Flush();
	m_ComputeCommandQueue->Flush();
	m_CopyCommandQueue->Flush();
}

void Device::Initialize()
{
	m_DirectCommandQueue = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_ComputeCommandQueue = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_CopyCommandQueue = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY);

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DescriptorAllocators[i] =
			std::make_unique<DescriptorAllocator>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
	}
}

const std::wstring Device::GetDescription() const
{
	return m_Adapter->GetDescription();
}

CommandQueue& Device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
	CommandQueue* commandQueue;

	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		commandQueue = m_DirectCommandQueue.get();
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		commandQueue = m_ComputeCommandQueue.get();
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		commandQueue = m_CopyCommandQueue.get();
		break;
	default:
		assert(false && "Invalid command queue type.");
	}

	return *commandQueue;
}
