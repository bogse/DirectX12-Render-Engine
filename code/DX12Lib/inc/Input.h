#pragma once

#include "KeyCodes.h"

#include <bitset>

class Input
{
public:
	static void SetKeyState(KeyCode::Key key, bool isPressed)
	{
		size_t index = static_cast<size_t>(key);
		if (index < 256)
		{
			m_KeyStates[index] = isPressed;
		}
	}

	static bool IsKeyPressed(KeyCode::Key key)
	{
		size_t index = static_cast<size_t>(key);
		return (index < 256) ? m_KeyStates[index] : false;
	}

	static void ClearStates()
	{
		m_KeyStates.reset();
	}

private:
	inline static std::bitset<256> m_KeyStates{};
};