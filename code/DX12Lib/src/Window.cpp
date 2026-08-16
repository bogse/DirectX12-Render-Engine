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

void Window::RegisterEvents(WindowListener* listener)
{
	m_Update.AddListener([listener](UpdateEventArgs& eventArgs) { listener->OnUpdate(eventArgs); });
	m_Render.AddListener([listener](RenderEventArgs& eventArgs) { listener->OnRender(eventArgs); });
	m_KeyPressed.AddListener([listener](KeyEventArgs& eventArgs) { listener->OnKeyPressed(eventArgs); });
	m_KeyReleased.AddListener([listener](KeyEventArgs& eventArgs) { listener->OnKeyReleased(eventArgs); });
	m_MouseMoved.AddListener([listener](MouseMotionEventArgs& eventArgs) { listener->OnMouseMoved(eventArgs); });
	m_MouseButtonPressed.AddListener([listener](MouseButtonEventArgs& eventArgs) { listener->OnMouseButtonPressed(eventArgs); });
	m_MouseButtonReleased.AddListener([listener](MouseButtonEventArgs& eventArgs) { listener->OnMouseButtonReleased(eventArgs); });
	m_MouseWheel.AddListener([listener](MouseWheelEventArgs& eventArgs) { listener->OnMouseWheel(eventArgs); });
	m_Resize.AddListener([listener](ResizeEventArgs& eventArgs) { listener->OnResize(eventArgs); });
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
	EventArgs eventArgs;

	if (m_hWnd)
	{
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

	m_Update.Broadcast(updateEventArgs);
}

void Window::OnRender(RenderEventArgs& eventArgs)
{
	m_RenderClock.Tick();

	RenderEventArgs renderEventArgs(
		m_RenderClock.GetDeltaSeconds(),
		m_RenderClock.GetTotalSeconds(),
		eventArgs.m_FrameNumber);

	m_Render.Broadcast(renderEventArgs);
}

void Window::OnKeyPressed(KeyEventArgs& eventArgs)
{
	m_KeyPressed.Broadcast(eventArgs);
}

void Window::OnKeyReleased(KeyEventArgs& eventArgs)
{
	m_KeyReleased.Broadcast(eventArgs);
}

void Window::OnMouseMoved(MouseMotionEventArgs& eventArgs)
{
	eventArgs.m_RelX = eventArgs.m_X - m_PreviousMouseX;
	eventArgs.m_RelY = eventArgs.m_Y - m_PreviousMouseY;

	m_PreviousMouseX = eventArgs.m_X;
	m_PreviousMouseY = eventArgs.m_Y;

	m_MouseMoved.Broadcast(eventArgs);
}

void Window::OnMouseButtonPressed(MouseButtonEventArgs& eventArgs)
{
	m_MouseButtonPressed.Broadcast(eventArgs);
}

void Window::OnMouseButtonReleased(MouseButtonEventArgs& eventArgs)
{
	m_MouseButtonReleased.Broadcast(eventArgs);
}

void Window::OnMouseWheel(MouseWheelEventArgs& eventArgs)
{
	m_MouseWheel.Broadcast(eventArgs);
}

void Window::OnResize(ResizeEventArgs& eventArgs)
{
	// Update the client size.
	if (m_ClientWidth != eventArgs.m_Width || m_ClientHeight != eventArgs.m_Height)
	{
		m_ClientWidth = std::max(1, eventArgs.m_Width);
		m_ClientHeight = std::max(1, eventArgs.m_Height);

		m_Resize.Broadcast(eventArgs);
	}
}
