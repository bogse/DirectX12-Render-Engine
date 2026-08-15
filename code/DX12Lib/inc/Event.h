#pragma once

#include <functional>

#include "EventArgs.h"

template <typename T>
class Event
{
	static_assert(std::is_base_of_v<EventArgs, T>);

public:
	using CallbackType = std::function<void(T&)>;

	~Event()
	{
		Clear();
	}

	void AddListener(CallbackType callback)
	{
		m_Callbacks.push_back(callback);
	}

	void Broadcast(T& eventArgs)
	{
		for (const CallbackType& callback : m_Callbacks)
		{
			if (callback)
				callback(eventArgs);
		}
	}

	void Clear()
	{
		m_Callbacks.clear();
	}

private:
	std::vector<CallbackType> m_Callbacks;
};
