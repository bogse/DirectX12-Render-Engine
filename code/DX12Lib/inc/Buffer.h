#pragma once

#include "Resource.h"

class Device;

class Buffer : public Resource
{
public:
	/**
	* Create the views for the buffer resource.
	* Used by CommadList when setting the buffer contents.
	*/ 
	virtual void CreateViews(size_t numElements, size_t elementSize) = 0;

protected:
	Buffer(Device& device, const std::wstring& name = L"");
};
