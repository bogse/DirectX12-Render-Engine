#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl/client.h>
#include <dxgi1_6.h>

#include "Events.h"
#include "HighResolutionClock.h"
#include "Texture.h"

class RenderApp;
class SwapChain;

class Window
{
public:
	static constexpr UINT BufferCount = 3;

	HWND GetWindowHandle() const;

	void Destroy();

	const std::wstring& GetWindowName() const;

	int GetClientWidth() const;
	int GetClientHeight() const;

	void SetVSync(bool vSync);
	void ToggleVSync();

	bool IsFullscreen() const;
	void SetFullscreen(const bool fullscreen);
	void ToggleFullscreen();

	void Show();
	void Hide();

	/*
	* Present the swapchain's back buffer to the screen. Returns the current back buffer index after the present.
	*/
	UINT Present(const std::shared_ptr<Texture>& texture = nullptr);

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
	Window(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync);
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

	HWND m_hWnd;

	std::wstring m_WindowName;

	int m_ClientWidth;
	int m_ClientHeight;
	bool m_Fullscreen;

	HighResolutionClock m_UpdateClock;
	HighResolutionClock m_RenderClock;

	uint64_t m_FrameValues[BufferCount];

	std::weak_ptr<RenderApp> m_pRenderApp;

	std::shared_ptr<SwapChain> m_SwapChain;

	RECT m_WindowRect;

	int m_PreviousMouseX;
	int m_PreviousMouseY;
};