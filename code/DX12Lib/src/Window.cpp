#include "DX12LibPCH.h"

#include "Window.h"

#include "WindowListener.h"

Window::Window(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight)
	: m_hWnd(hWnd)
	, m_WindowName(windowName)
	, m_ClientWidth(clientWidth)
	, m_ClientHeight(clientHeight)
	, m_Fullscreen(false)
	, m_RenderClock()
	, m_UpdateClock()
	, m_PreviousMouseX(0)
	, m_PreviousMouseY(0)
{
	::GetWindowRect(m_hWnd, &m_WindowRect);
}

Window::~Window()
{
	// Window should be destroyed with Application::DestroyWindow before the window goes out of scope.
	assert(!m_hWnd && "Use Application::DestroyWindow before destruction.");
}

void Window::RegisterListener(WindowListener* listener)
{
	assert(listener && "Listener is nullptr.");

	if (std::find(m_Listeners.begin(), m_Listeners.end(), listener) == m_Listeners.end())
	{
		m_Listeners.push_back(listener);
	}
}

void Window::UnregisterListener(WindowListener* listener)
{
	assert(listener && "Listener is nullptr.");

	std::vector<WindowListener*>::iterator it = std::remove(m_Listeners.begin(), m_Listeners.end(), listener);
	m_Listeners.erase(it, m_Listeners.end());
}

void Window::UnregisterListeners()
{
	m_Listeners.clear();
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
	if (m_hWnd)
	{
		UnregisterListeners();
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
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
	SetFullscreen(!m_Fullscreen);
}

void Window::OnUpdate(UpdateEventArgs& eventArgs)
{
	m_UpdateClock.Tick();

	UpdateEventArgs updateEventArgs(
		m_UpdateClock.GetDeltaSeconds(),
		m_UpdateClock.GetTotalSeconds(),
		eventArgs.m_FrameNumber);

	for (WindowListener* listener : m_Listeners)
	{
		listener->OnUpdate(updateEventArgs);
	}
}

void Window::OnRender(RenderEventArgs& eventArgs)
{
	m_RenderClock.Tick();

	RenderEventArgs renderEventArgs(
		m_RenderClock.GetDeltaSeconds(),
		m_RenderClock.GetTotalSeconds(),
		eventArgs.m_FrameNumber);

	for (WindowListener* listener : m_Listeners)
	{
		listener->OnRender(renderEventArgs);
	}
}

void Window::OnKeyPressed(KeyEventArgs& eventArgs)
{
	for (WindowListener* listener : m_Listeners)
	{
		listener->OnKeyPressed(eventArgs);
	}
}

void Window::OnKeyReleased(KeyEventArgs& eventArgs)
{
	for (WindowListener* listener : m_Listeners)
	{
		listener->OnKeyReleased(eventArgs);
	}
}

void Window::OnMouseMoved(MouseMotionEventArgs& eventArgs)
{
	eventArgs.m_RelX = eventArgs.m_X - m_PreviousMouseX;
	eventArgs.m_RelY = eventArgs.m_Y - m_PreviousMouseY;

	m_PreviousMouseX = eventArgs.m_X;
	m_PreviousMouseY = eventArgs.m_Y;

	for (WindowListener* listener : m_Listeners)
	{
		listener->OnMouseMoved(eventArgs);
	}
}

void Window::OnMouseButtonPressed(MouseButtonEventArgs& eventArgs)
{
	for (WindowListener* listener : m_Listeners)
	{
		listener->OnMouseButtonPressed(eventArgs);
	}
}

void Window::OnMouseButtonReleased(MouseButtonEventArgs& eventArgs)
{
	for (WindowListener* listener : m_Listeners)
	{
		listener->OnMouseButtonReleased(eventArgs);
	}
}

void Window::OnMouseWheel(MouseWheelEventArgs& eventArgs)
{
	for (WindowListener* listener : m_Listeners)
	{
		listener->OnMouseWheel(eventArgs);
	}
}

void Window::OnResize(ResizeEventArgs& eventArgs)
{
	// Update the client size.
	if (m_ClientWidth != eventArgs.m_Width || m_ClientHeight != eventArgs.m_Height)
	{
		m_ClientWidth = std::max(1, eventArgs.m_Width);
		m_ClientHeight = std::max(1, eventArgs.m_Height);

		for (WindowListener* listener : m_Listeners)
		{
			listener->OnResize(eventArgs);
		}
	}
}
