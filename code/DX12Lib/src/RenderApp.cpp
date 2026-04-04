#include "DX12LibPCH.h"

#include "RenderApp.h"

#include "Application.h"
#include "Window.h"

RenderApp::RenderApp(const std::wstring& name, int width, int height, bool vSync)
	: m_Name(name)
	, m_Width(width)
	, m_Height(height)
	, m_vSync(vSync)
{
}

RenderApp::~RenderApp()
{
	assert(!m_pWindow && "Use RenderApp::Destroy() before destruction.");
}

bool RenderApp::Initialize()
{
	if (!DirectX::XMVerifyCPUSupport())
	{
		MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
		return false;
	}

	m_pWindow = Application::GetInstance().CreateRenderWindow(m_Name, m_Width, m_Height, m_vSync);
	m_pWindow->RegisterCallbacks(shared_from_this());
	m_pWindow->Show();

	return true;
}

void RenderApp::Destroy()
{
	Application::GetInstance().DestroyWindow(m_pWindow);
	m_pWindow.reset();
}

void RenderApp::OnUpdate(UpdateEventArgs& eventArgs)
{
}

void RenderApp::OnRender(RenderEventArgs& eventArgs)
{
}

void RenderApp::OnKeyPressed(KeyEventArgs& eventArgs)
{
	// By default, do nothing.
}

void RenderApp::OnKeyReleased(KeyEventArgs& eventArgs)
{
	// By default, do nothing.
}

void RenderApp::OnMouseMoved(class MouseMotionEventArgs& eventArgs)
{
	// By default, do nothing.
}

void RenderApp::OnMouseButtonPressed(MouseButtonEventArgs& eventArgs)
{
	// By default, do nothing.
}

void RenderApp::OnMouseButtonReleased(MouseButtonEventArgs& eventArgs)
{
	// By default, do nothing.
}

void RenderApp::OnMouseWheel(MouseWheelEventArgs& eventArgs)
{
	// By default, do nothing.
}

void RenderApp::OnResize(ResizeEventArgs& eventArgs)
{
	m_Width = eventArgs.m_Width;
	m_Height = eventArgs.m_Height;
}

void RenderApp::OnWindowDestroy()
{
	// If the Window which we are registered to is destroyed, 
	// then any resources which are associated to the window must be released.
	UnloadContent();
}