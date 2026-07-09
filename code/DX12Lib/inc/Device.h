#pragma once

#include "DescriptorAllocation.h"

#include <wrl/client.h>

#include <memory>
#include <string>

class Adapter;
class CommandQueue;
class DescriptorAllocator;

class Device
{
public:
	/**
	* Create a new DX12 device using the provided adapter.
	* If no adapter is specified, the the highest performance adapter will be chosen.
	*/
	static std::shared_ptr<Device> Create(std::shared_ptr<Adapter> adapter = nullptr);

	/**
	* Allocate a number of CPU visible descriptors.
	*/
	DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);

	/**
	* Release stale descriptors. This should only be called with a completed frame counter.
	*/
	void ReleaseStaleDescriptors(uint64_t finishedFrame);

	Microsoft::WRL::ComPtr<ID3D12Device8> GetD3D12Device() const
	{
		return m_d3d12Device;
	}

	/**
	* Flush all command queues.
	*/
	void Flush();

	void Initialize();

	/**
	* Get the adapter that was used to create this device.
	*/
	std::shared_ptr<Adapter> GetAdapter() const
	{
		return m_Adapter;
	}

	/**
	* Get a description of the adapter that was used to create the device.
	*/
	const std::wstring GetDescription() const;

	/**
	* Get a command queue. Valid types are:
	* - D3D12_COMMAND_LIST_TYPE_DIRECT : Can be used for draw, dispatch, or copy commands.
	* - D3D12_COMMAND_LIST_TYPE_COMPUTE: Can be used for dispatch or copy commands.
	* - D3D12_COMMAND_LIST_TYPE_COPY   : Can be used for copy commands.
	*/
	CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
	{
		return m_d3d12Device->GetDescriptorHandleIncrementSize(type);
	}

protected:
	explicit Device(std::shared_ptr<Adapter> adapter);
	virtual ~Device() = default;

private:
	Microsoft::WRL::ComPtr<ID3D12Device8> m_d3d12Device;

	/**
	* The adapter that was used to create the device.
	*/
	std::shared_ptr<Adapter> m_Adapter;

	/**
	* Default command queues.
	*/
	std::unique_ptr<CommandQueue> m_DirectCommandQueue;
	std::unique_ptr<CommandQueue> m_ComputeCommandQueue;
	std::unique_ptr<CommandQueue> m_CopyCommandQueue;

	/**
	* Descriptor allocators.
	*/
	std::unique_ptr<DescriptorAllocator> m_DescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};