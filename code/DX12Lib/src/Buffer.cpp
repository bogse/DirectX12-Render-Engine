#include "DX12LibPCH.h"

#include "Buffer.h"

Buffer::Buffer(Device& device, const std::wstring& name)
	: Resource(device, name)
{}
