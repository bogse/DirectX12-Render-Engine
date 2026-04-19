/**
* Wrapper class for ID3D12CommandQueue
*/

#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <cstdint>
#include <queue>

class CommandList;

class CommandQueue
{
public:
	CommandQueue(D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandQueue();

	std::shared_ptr<CommandList> GetCommandList();

	/** 
	* Returns the fence to wait for this command list
	*/
	uint64_t ExecuteCommandList(std::shared_ptr<CommandList> commandList);
	uint64_t ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& commandLists);

	uint64_t Signal();
	bool IsFenceComplete(uint64_t fenceValue);
	void WaitForFenceValue(uint64_t fenceValue);
	void Flush();

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

private:
	/**
	* Keep track of the command allocators that are "in-flight"
	*/
	struct CommandListEntry
	{
		uint64_t fenceValue;
		std::shared_ptr<CommandList> commandList;
	};

	using CommandListQueue = std::queue<CommandListEntry>;

	D3D12_COMMAND_LIST_TYPE						m_CommandListType;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>	m_d3d12CommandQueue;
	Microsoft::WRL::ComPtr<ID3D12Fence>			m_d3d12Fence;
	HANDLE										m_FenceEvent;
	uint64_t									m_FenceValue;

	CommandListQueue							m_CommandListQueue;
};