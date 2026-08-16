#include "DX12LibPCH.h"

#include "RenderApp.h"

#include "Application.h"
#include "Device.h"
#include "GUISystem.h"
#include "SwapChain.h"

RenderApp::RenderApp(const std::wstring& name, int width, int height)
	: m_Name(name)
	, m_Width(width)
	, m_Height(height)
	, m_FPS(0.f)
{
}

RenderApp::~RenderApp()
{
	m_SwapChain.reset();
	m_Device.reset();
	assert(!m_GUISystem && "Use RenderApp::Destroy() before destruction.");
}

bool RenderApp::Initialize()
{
	if (!DirectX::XMVerifyCPUSupport())
	{
		MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
		return false;
	}

	Application& app = Application::GetInstance();

	app.CreateRenderWindow(this, m_Name, m_Width, m_Height);

	HWND hWnd = app.GetWindowHandle(m_Name);

	m_Device = Device::Create();
	m_Device->Initialize();

	m_SwapChain = m_Device->CreateSwapChain(hWnd);

	// Initialize imgui wrapper class.
	m_GUISystem = std::make_unique<GUISystem>();

	m_GUISystem->Initialize(
		hWnd,
		m_Device->GetD3D12Device().Get(),
		&m_Device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

	Application::OnWndProcHandler = [this](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
		assert(m_GUISystem);

		if (m_GUISystem->WndProcHandler(hWnd, message, wParam, lParam))
			return 1;

		return 0;
	};

	return true;
}

void RenderApp::Destroy()
{
	Application::GetInstance().DestroyWindow(m_Name);

	// Flush any commands in the commands queues before quitting;
	m_Device->Flush();

	m_GUISystem->Shutdown();

	m_GUISystem.reset();
}

int RenderApp::Run()
{
	if (!Initialize())
		return 1;

	if (!LoadContent())
		return 2;

	int retCode = Application::GetInstance().Run();

	UnloadContent();
	Destroy();

	return retCode;
}

UINT RenderApp::Present(const std::shared_ptr<Texture>& texture)
{
	return m_SwapChain->Present(texture);
}

void RenderApp::OnUpdate(UpdateEventArgs& eventArgs)
{
	m_SwapChain->WaitForSwapChain();

	static uint64_t frameCount = 0;
	static double totalTime = 0.0;

	totalTime += eventArgs.m_ElapsedTime;
	frameCount++;

	if (totalTime > 1.0)
	{
		m_FPS = frameCount / totalTime;
		frameCount = 0;
		totalTime = 0.0;
	}
}

void RenderApp::OnRender(RenderEventArgs& eventArgs)
{
}

void RenderApp::OnKeyPressed(KeyEventArgs& eventArgs)
{
	Application& app = Application::GetInstance();

	switch (eventArgs.m_Key)
	{
	case KeyCode::Key::Escape:
	{
		app.Quit(0);
		break;
	}
	case KeyCode::Key::Enter:
	{
		if (eventArgs.m_Alt)
		{
			app.ToggleFullscreen(m_Name);
		}
		break;
	}
	case KeyCode::Key::F11:
	{
		app.ToggleFullscreen(m_Name);
		break;
	}
	case KeyCode::Key::V:
	{
		m_SwapChain->ToggleVSync();
		break;
	}
	}
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

	m_SwapChain->Resize(m_Width, m_Height);
}
