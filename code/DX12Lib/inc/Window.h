#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl/client.h>
#include <dxgi1_6.h>

#include "Event.h"
#include "HighResolutionClock.h"

class RenderApp;

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
	/*
	* The DirectXTemplate class needs to register itself with a window.
	*/
	friend class RenderApp;

	Window() = delete;
	Window(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight);
	virtual ~Window();

	/*
	* Register a RenderApp with this window. This allows the window to callback functions in the RenderApp class.
	*/
	void RegisterCallbacks(std::shared_ptr<RenderApp> pRenderApp);

	virtual void OnUpdate(UpdateEventArgs& eventArgs);
	virtual void OnRender(RenderEventArgs& eventArgs);

	virtual void OnKeyPressed(KeyEventArgs& eventArgs);
	virtual void OnKeyReleased(KeyEventArgs& eventArgs);

	virtual void OnMouseMoved(MouseMotionEventArgs& eventArgs);
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& eventArgs);
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& eventArgs);
	virtual void OnMouseWheel(MouseWheelEventArgs& eventArgs);

	virtual void OnResize(ResizeEventArgs& eventArgs);

private:
	Window(const Window& otherWindow) = delete;
	Window& operator= (const Window& otherWindow) = delete;

	std::weak_ptr<RenderApp> m_pRenderApp;

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
