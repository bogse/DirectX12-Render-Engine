#pragma once
#include "KeyCodes.h"

#include <cstdint>

/**
* Base class for events args
*/
class EventArgs
{
public:
	EventArgs() {}
};

class KeyEventArgs : public EventArgs
{
public:
	enum KeyState
	{
		Released,
		Pressed
	};

	KeyEventArgs(KeyCode::Key key, unsigned int c, KeyState state, bool control, bool shift, bool alt)
		: m_Key(key)
		, m_Char(c)
		, m_State(state)
		, m_Control(control)
		, m_Shift(shift)
		, m_Alt(alt)
	{
	}

	KeyCode::Key	m_Key;	// The Key Code that was pressed or release.
	unsigned int		m_Char;	// The 32-bit character code that was pressed. This value will be 0 if it is a non-printable character.
	KeyState		m_State;	// Key pressed or released.
	bool			m_Control;// Control modifier pressed.
	bool			m_Shift;	// Shift modifier pressed.
	bool			m_Alt;	// Alt modifier pressed.
};

class MouseMotionEventArgs : public EventArgs
{
public:
	MouseMotionEventArgs(bool leftButton, bool middleButton, bool rightButton, bool control, bool shift, int x, int y)
		: m_LeftButton(leftButton)
		, m_MiddleButton(middleButton)
		, m_RightButton(rightButton)
		, m_Control(control)
		, m_Shift(shift)
		, m_X(x)
		, m_Y(y)
	{
	}

	bool m_LeftButton;		// Left mouse button down.
	bool m_MiddleButton;	// Middle mouse button down.
	bool m_RightButton;		// Right mouse button down.
	bool m_Control;			// Ctrl key down.
	bool m_Shift;			// Shift key down.

	int m_X;				// The X-position of the cursor relative to the upper-left corner of the client area.
	int m_Y;				// The Y-position of the cursor relative to the upper-left corner of the client area.
	int m_RelX;				// How far the mouse moved since the last event.
	int m_RelY;				// How far the mouse moved since the last event.
};

class MouseButtonEventArgs : public EventArgs
{
public:
	enum MouseButton
	{
		None,
		Left,
		Right,
		Middle
	};

	enum ButtonState
	{
		Released,
		Pressed
	};

	MouseButtonEventArgs(MouseButton buttonId, ButtonState state, 
		bool leftButton, bool middleButton, bool rightButton, bool control, bool shift, int x, int y
	)
		: m_Button(buttonId)
		, m_State(state)
		, m_LeftButton(leftButton)
		, m_MiddleButton(middleButton)
		, m_RightButton(rightButton)
		, m_Control(control)
		, m_Shift(shift)
		, m_X(x)
		, m_Y(y)
	{
	}

	MouseButton m_Button;		// The mouse button that was pressed or released.
	ButtonState m_State;		// Button was pressed or released.
	bool m_LeftButton;			// Left mouse down.
	bool m_MiddleButton;		// Middle mouse down.
	bool m_RightButton;			// Right mouse down.
	bool m_Control;				// Ctrl key down.
	bool m_Shift;				// Shift key down.

	int m_X;					// The X-position of the cursor relative to the upper-left corner of the client area.
	int m_Y;					// The Y-position of the cursor relative to the upper-left corner of the client area.
};

class MouseWheelEventArgs : public EventArgs
{
public:
	MouseWheelEventArgs(float wheelDelta, bool leftButton, bool middleButton, bool rightButton, bool control, bool shift, int x, int y)
		: m_WheelDelta(wheelDelta)
		, m_LeftButton(leftButton)
		, m_MiddleButton(middleButton)
		, m_RightButton(rightButton)
		, m_Control(control)
		, m_Shift(shift)
		, m_X(x)
		, m_Y(y)
	{
	}

	float m_WheelDelta;		// How much the mouse wheel has moved. A positive value indicates that the wheel was moved to the right.
	bool m_LeftButton;		// Left mouse button down.
	bool m_MiddleButton;	// Middle mouse button down.
	bool m_RightButton;		// Right mouse button down.
	bool m_Control;			// Ctrl key down.
	bool m_Shift;			// Shift key down.

	int m_X;				// The X-position of the cursor relative to the upper-left corner of the client area.
	int m_Y;				// The Y-position of the cursor relative to the upper-left corner of the client area.
};

class ResizeEventArgs : public EventArgs
{
public:
	ResizeEventArgs(int width, int height)
		: m_Width(width)
		, m_Height(height)
	{
	}

	int m_Width;	// The new width of the window.
	int m_Height;	// The new height of the window.
};

class UpdateEventArgs : public EventArgs
{
public:
	UpdateEventArgs(double deltaTime, double totalTime, uint64_t frameNumber)
		: m_ElapsedTime(deltaTime)
		, m_TotalTime(totalTime)
		, m_FrameNumber(frameNumber)
	{
	}

	double m_ElapsedTime;
	double m_TotalTime;
	uint64_t m_FrameNumber;
};

class RenderEventArgs : public EventArgs
{
public:
	RenderEventArgs(double deltaTime, double totalTime, uint64_t frameNumber)
		: m_ElapsedTime(deltaTime)
		, m_TotalTime(totalTime)
		, m_FrameNumber(frameNumber)
	{
	}

	double m_ElapsedTime;
	double m_TotalTime;
	uint64_t m_FrameNumber;
};

class UserEventArgs : public EventArgs
{
public:
	UserEventArgs(int code, void* data1, void* data2)
		: m_Code(code)
		, m_Data1(data1)
		, m_Data2(data2)
	{
	}

	int		m_Code;
	void*	m_Data1;
	void*	m_Data2;
};