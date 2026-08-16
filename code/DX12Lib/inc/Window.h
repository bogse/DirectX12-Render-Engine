#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl/client.h>
#include <dxgi1_6.h>

#include "Event.h"
#include "HighResolutionClock.h"

class WindowListener;

class Window
{
public:
	void Destroy();

	int GetClientWidth() const
	{
		return m_ClientWidth;
	}

	int GetClientHeight() const
	{
		return m_ClientHeight;
	}

	HWND GetWindowHandle() const
	{
		return m_hWnd;
	}

	const std::wstring& GetWindowName() const
	{
		return m_WindowName;
	}

	bool IsFullscreen() const
	{
		return m_Fullscreen;
	}

	void SetFullscreen(const bool fullscreen);
	void ToggleFullscreen();

	void Show();
	void Hide();

protected:
	/*
	* The Window procedure needsto call protected methods of this class.
	*/
	friend LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	/*
	* Only the application can create a window.
	*/
	friend class Application;

	Window() = delete;
	Window(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight);
	virtual ~Window();

	void RegisterEvents(WindowListener* listener);

	void OnUpdate(UpdateEventArgs& eventArgs);
	void OnRender(RenderEventArgs& eventArgs);

	void OnKeyPressed(KeyEventArgs& eventArgs);
	void OnKeyReleased(KeyEventArgs& eventArgs);

	void OnMouseMoved(MouseMotionEventArgs& eventArgs);
	void OnMouseButtonPressed(MouseButtonEventArgs& eventArgs);
	void OnMouseButtonReleased(MouseButtonEventArgs& eventArgs);
	void OnMouseWheel(MouseWheelEventArgs& eventArgs);

	void OnResize(ResizeEventArgs& eventArgs);

private:
	Window(const Window& otherWindow) = delete;
	Window& operator= (const Window& otherWindow) = delete;

	HighResolutionClock m_UpdateClock;
	HighResolutionClock m_RenderClock;

	std::wstring m_WindowName;

	int m_ClientWidth;
	int m_ClientHeight;

	RECT m_WindowRect;
	HWND m_hWnd;

	int m_PreviousMouseX;
	int m_PreviousMouseY;

	bool m_Fullscreen;

	Event<UpdateEventArgs>		m_Update;
	Event<RenderEventArgs>		m_Render;
	Event<KeyEventArgs>			m_KeyPressed;
	Event<KeyEventArgs>			m_KeyReleased;
	Event<MouseMotionEventArgs> m_MouseMoved;
	Event<MouseButtonEventArgs> m_MouseButtonPressed;
	Event<MouseButtonEventArgs> m_MouseButtonReleased;
	Event<MouseWheelEventArgs>	m_MouseWheel;
	Event<ResizeEventArgs>		m_Resize;
};
