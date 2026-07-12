#include "DX12LibPCH.h"

#include "Window.h"

#include "Application.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "Device.h"
#include "ResourceStateTracker.h"
#include "RenderApp.h"
#include "SwapChain.h"

Window::Window(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
	: m_hWnd(hWnd)
	, m_WindowName(windowName)
	, m_ClientWidth(clientWidth)
	, m_ClientHeight(clientHeight)
	, m_RenderClock()
	, m_UpdateClock()
	, m_FrameValues{0}
	, m_PreviousMouseX(0)
	, m_PreviousMouseY(0)
{
	const Application& app = Application::GetInstance();
	m_SwapChain = app.GetDevicePtr()->CreateSwapChain(hWnd);
}

Window::~Window()
{
	// Window should be destroyed with Application::DestroyWindow before the window goes out of scope.
	assert(!m_hWnd && "Use Application::DestroyWindow before destruction.");
}

HWND Window::GetWindowHandle() const
{
	return m_hWnd;
}

const std::wstring& Window::GetWindowName() const
{
	return m_WindowName;
}

void Window::Show()
{
	::ShowWindow(m_hWnd, SW_SHOW);
}

void Window::Hide()
{
	::ShowWindow(m_hWnd, SW_HIDE);
}

void Window::Destroy()
{
	if (std::shared_ptr<RenderApp> pRenderApp = m_pRenderApp.lock())
	{
		// Notify the registered render app that the window is being destroyed.
		pRenderApp->OnWindowDestroy();
	}

	if (m_hWnd)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
}

int Window::GetClientWidth() const
{
	return m_ClientWidth;
}

int Window::GetClientHeight() const
{
	return m_ClientHeight;
}

void Window::SetVSync(bool vSync)
{
	m_SwapChain->SetVSync(vSync);
}

void Window::ToggleVSync()
{
	m_SwapChain->ToggleVSync();
}

bool Window::IsFullscreen() const
{
	return m_Fullscreen;
}

void Window::SetFullscreen(bool fullscreen)
{
	if (m_Fullscreen != fullscreen)
	{
		m_Fullscreen = fullscreen;

		if (m_Fullscreen) // Switching to fullscreen.
		{
			// Store the current window dimensions so they can be restored
			// when switching out of fullscreen state.
			::GetWindowRect(m_hWnd, &m_WindowRect);

			// Set the window style to a borderless window so the client area fills the entire screen.
			UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

			::SetWindowLongW(m_hWnd, GWL_STYLE, windowStyle);

			// Query the name of the nearest display device for the window.
			// This is required to set the fullscreen dimensions of the window
			// when using a multi-monitor setup.
			HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfo(hMonitor, &monitorInfo);

			::SetWindowPos(m_hWnd, HWND_TOPMOST,
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(m_hWnd, SW_MAXIMIZE);
		}
		else
		{
			// Restore all the window decorators.
			::SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			::SetWindowPos(m_hWnd, HWND_NOTOPMOST,
				m_WindowRect.left,
				m_WindowRect.top,
				m_WindowRect.right - m_WindowRect.left,
				m_WindowRect.bottom - m_WindowRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(m_hWnd, SW_NORMAL);
		}
	}
}

void Window::ToggleFullscreen()
{
	m_SwapChain->ToggleFullscreen();
}

void Window::RegisterCallbacks(std::shared_ptr<RenderApp> pRenderApp)
{
	m_pRenderApp = pRenderApp;
}

void Window::OnUpdate(UpdateEventArgs& eventArgs)
{
	m_SwapChain->WaitForSwapChain();

	m_UpdateClock.Tick();

	if (std::shared_ptr<RenderApp> pRenderApp = m_pRenderApp.lock())
	{
		UpdateEventArgs updateEventArgs(m_UpdateClock.GetDeltaSeconds(), m_UpdateClock.GetTotalSeconds(), eventArgs.m_FrameNumber);
		pRenderApp->OnUpdate(updateEventArgs);
	}
}

void Window::OnRender(RenderEventArgs& eventArgs)
{
	m_RenderClock.Tick();

	if (std::shared_ptr<RenderApp> pRenderApp = m_pRenderApp.lock())
	{
		RenderEventArgs renderEventArgs(m_RenderClock.GetDeltaSeconds(), m_RenderClock.GetTotalSeconds(), eventArgs.m_FrameNumber);
		pRenderApp->OnRender(renderEventArgs);
	}
}

void Window::OnKeyPressed(KeyEventArgs& eventArgs)
{
	if (std::shared_ptr<RenderApp> pRenderApp = m_pRenderApp.lock())
	{
		pRenderApp->OnKeyPressed(eventArgs);
	}
}

void Window::OnKeyReleased(KeyEventArgs& eventArgs)
{
	if (std::shared_ptr<RenderApp> pRenderApp = m_pRenderApp.lock())
	{
		pRenderApp->OnKeyReleased(eventArgs);
	}
}

void Window::OnMouseMoved(MouseMotionEventArgs& eventArgs)
{
	eventArgs.m_RelX = eventArgs.m_X - m_PreviousMouseX;
	eventArgs.m_RelY = eventArgs.m_Y - m_PreviousMouseY;

	m_PreviousMouseX = eventArgs.m_X;
	m_PreviousMouseY = eventArgs.m_Y;

	if (std::shared_ptr<RenderApp> pRenderApp = m_pRenderApp.lock())
	{
		pRenderApp->OnMouseMoved(eventArgs);
	}
}

void Window::OnMouseButtonPressed(MouseButtonEventArgs& eventArgs)
{
	if (std::shared_ptr<RenderApp> pRenderApp = m_pRenderApp.lock())
	{
		pRenderApp->OnMouseButtonPressed(eventArgs);
	}
}

void Window::OnMouseButtonReleased(MouseButtonEventArgs& eventArgs)
{
	if (std::shared_ptr<RenderApp> pRenderApp = m_pRenderApp.lock())
	{
		pRenderApp->OnMouseButtonReleased(eventArgs);
	}
}

void Window::OnMouseWheel(MouseWheelEventArgs& eventArgs)
{
	if (std::shared_ptr<RenderApp> pRenderApp = m_pRenderApp.lock())
	{
		pRenderApp->OnMouseWheel(eventArgs);
	}
}

void Window::OnResize(ResizeEventArgs& eventArgs)
{
	// Update the client size.
	if (m_ClientWidth != eventArgs.m_Width || m_ClientHeight != eventArgs.m_Height)
	{
		m_ClientWidth = std::max(1, eventArgs.m_Width);
		m_ClientHeight = std::max(1, eventArgs.m_Height);

		m_SwapChain->Resize(m_ClientWidth, m_ClientHeight);
	}

	if (std::shared_ptr<RenderApp> pRenderApp = m_pRenderApp.lock())
	{
		pRenderApp->OnResize(eventArgs);
	}
}

UINT Window::Present(const std::shared_ptr<Texture>& texture)
{
	return m_SwapChain->Present(texture);
}
