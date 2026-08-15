#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl/client.h>
#include <dxgi1_6.h>

#include "Event.h"
#include "HighResolutionClock.h"

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

	Event<UpdateEventArgs>		Update;
	Event<RenderEventArgs>		Render;
	Event<KeyEventArgs>			KeyPressed;
	Event<KeyEventArgs>			KeyReleased;
	Event<MouseMotionEventArgs> MouseMoved;
	Event<MouseButtonEventArgs> MouseButtonPressed;
	Event<MouseButtonEventArgs> MouseButtonReleased;
	Event<MouseWheelEventArgs>	MouseWheel;
	Event<ResizeEventArgs>		Resize;
	Event<EventArgs>			WindowDestroy;

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
};
