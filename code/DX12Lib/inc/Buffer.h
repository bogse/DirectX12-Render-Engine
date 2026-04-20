#pragma once

#include "Resource.h"

class Buffer : public Resource
{
public:
	Buffer(const std::wstring& name = L"");

	/**
	* Create the views for the buffer resource.
	* Used by CommadList when setting the buffer contents.
	*/ 
	virtual void CreateViews(size_t numElements, size_t elementSize) = 0;
};