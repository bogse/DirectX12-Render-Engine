#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl/client.h>
#include <dxgi1_6.h>

#include "EventArgs.h"
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

	void RegisterListener(WindowListener* listener);
	void UnregisterListener(WindowListener* listener);
	void UnregisterListeners();

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

	std::vector<WindowListener*> m_Listeners;

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
