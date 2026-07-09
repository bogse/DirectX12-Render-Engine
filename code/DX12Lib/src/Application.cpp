#include "DX12LibPCH.h"

#include "Application.h"

#include "CommandQueue.h"
#include "Device.h"
#include "GUISystem.h"
#include "Input.h"
#include "RenderApp.h"
#include "Window.h"

constexpr wchar_t WINDOW_CLASS_NAME[] = L"DX12RenderWindowClass";

using WindowPtr =		std::shared_ptr<Window>;
using WindowMap =		std::map<HWND, WindowPtr>;
using WindowNameMap =	std::map <std::wstring, WindowPtr>;

static Application*		g_pSingleton = nullptr;
static WindowMap		g_Windows;
static WindowNameMap	g_WindowByName;

uint64_t Application::ms_FrameCount = 0;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// A wrapper struct to allow shared pointers for the window class.
// This is needed because the constructor and destructor for the Window
// class are protected and not accessible by the std::make_shared method.
struct MakeWindow : public Window
{
	MakeWindow(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
		: Window(hWnd, windowName, clientWidth, clientHeight, vSync)
	{
	}
};

Application::Application(HINSTANCE hInst)
	: m_hInstance(hInst)
	, m_TearingSupported(false)
{}

Application::~Application()
{
	Flush();

	m_GUISystem->Shutdown();
	m_GUISystem.reset();
}

void Application::Initialize()
{
	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window
	// to achieve 100% scaling while still allowing non-client window content to
	// be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

#if defined(_DEBUG)
	// Always enable the debug layer before doing anything DX12 related
	// so all possible errors generated while creating DX12 objects
	// are caught by the debug layer.
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
	// Enable these if you want full validation (will slow down rendering a lot).
	//debugInterface->SetEnableGPUBasedValidation(TRUE);
	//debugInterface->SetEnableSynchronizedCommandQueueValidation(TRUE);
#endif

	WNDCLASSEXW wndClass = { 0 };

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = &WndProc;
	wndClass.hInstance = m_hInstance;
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = WINDOW_CLASS_NAME;

	if (!RegisterClassExW(&wndClass))
	{
		MessageBoxA(NULL, "Unable to register the window class.", "Error", MB_OK | MB_ICONERROR);
	}

	m_Device = Device::Create();
	m_Device->Initialize();

	m_TearingSupported = CheckTearingSupport();

	// Initialize frame counter
	ms_FrameCount = 0;
}

void Application::Create(HINSTANCE hInst)
{
	if (!g_pSingleton)
	{
		g_pSingleton = new Application(hInst);
		g_pSingleton->Initialize();
	}
}

Application& Application::GetInstance()
{
	assert(g_pSingleton);
	return *g_pSingleton;
}

void Application::Destroy()
{
	if (g_pSingleton)
	{
		assert(g_Windows.empty() && g_WindowByName.empty() &&
			"All windows should be destroyed before destroying the application instance.");

		delete g_pSingleton;
		g_pSingleton = nullptr;
	}
}

bool Application::CheckTearingSupport()
{
	BOOL allowTearing = false;

	// Rather than create a DXGI 1.5 factory interface directly, we create the the 
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enabled the 
	// graphics debugging tools which will not support the 1.5 factory interface
	// until a future update.
	Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing));
		}
	}

	return allowTearing == TRUE;
}

bool Application::IsTearingSupported() const
{
	return m_TearingSupported;
}

std::shared_ptr<Window> Application::CreateRenderWindow(const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
{
	// First check if a window with the given name already exists.
	WindowNameMap::iterator windowIter = g_WindowByName.find(windowName);
	if (windowIter != g_WindowByName.end())
	{
		return windowIter->second;
	}

	RECT windowRect = { 0, 0, clientWidth, clientHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hWnd = CreateWindowW(WINDOW_CLASS_NAME, windowName.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, nullptr, m_hInstance, nullptr);

	if (!hWnd)
	{
		MessageBoxA(NULL, "Could not create the render window.", "Error", MB_OK | MB_ICONERROR);
		return nullptr;
	}

	WindowPtr pWindow = std::make_shared<MakeWindow>(hWnd, windowName, clientWidth, clientHeight, vSync);

	g_Windows.insert(WindowMap::value_type(hWnd, pWindow));
	g_WindowByName.insert(WindowNameMap::value_type(windowName, pWindow));

	// Initialize imgui wrapper class.
	m_GUISystem = std::make_unique<GUISystem>();

	m_GUISystem->Initialize(
		hWnd,
		m_Device->GetD3D12Device().Get(),
		&GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)
	);

	return pWindow;
}

void Application::DestroyWindow(std::shared_ptr<Window> window)
{
	if (window) 
		window->Destroy();
}

void Application::DestroyWindow(const std::wstring& windowName)
{
	WindowPtr pWindow = GetWindowByName(windowName);
	if (pWindow)
	{
		DestroyWindow(pWindow);
	}
}

std::shared_ptr<Window> Application::GetWindowByName(const std::wstring& windowName)
{
	std::shared_ptr<Window> window;
	WindowNameMap::iterator iter = g_WindowByName.find(windowName);
	if (iter != g_WindowByName.end())
	{
		window = iter->second;
	}

	return window;
}

int Application::Run(std::shared_ptr<RenderApp> pRenderApp)
{
	if (!pRenderApp->Initialize()) 
		return 1;

	if (!pRenderApp->LoadContent()) 
		return 2;

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Flush any commands in the commands queues before quitting;
	Flush();

	pRenderApp->UnloadContent();
	pRenderApp->Destroy();

	return static_cast<int>(msg.wParam);
}

void Application::Quit(int exitCode)
{
	PostQuitMessage(exitCode);
}

Microsoft::WRL::ComPtr<ID3D12Device2> Application::GetDevice() const
{
	return m_Device->GetD3D12Device();
}

CommandQueue& Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
	return m_Device->GetCommandQueue(type);
}

void Application::Flush()
{
	m_Device->Flush();
}

DescriptorAllocation Application::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
	return m_Device->AllocateDescriptors(type, numDescriptors);
}

void Application::ReleaseStaleDescriptors(uint64_t finishedFrame)
{
	m_Device->ReleaseStaleDescriptors(finishedFrame);
}

UINT Application::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	return m_Device->GetDescriptorHandleIncrementSize(type);
}

// Remove a window from our window lists.
static void RemoveWindow(HWND hWnd)
{
	WindowMap::iterator windowIter = g_Windows.find(hWnd);
	if (windowIter != g_Windows.end())
	{
		WindowPtr pWindow = windowIter->second;
		g_WindowByName.erase(pWindow->GetWindowName());
		g_Windows.erase(windowIter);
	}
}

// Convert the message ID into a MouseButton ID
MouseButtonEventArgs::MouseButton DecodeMouseButton(UINT messageID)
{
	MouseButtonEventArgs::MouseButton mouseButton = MouseButtonEventArgs::None;
	switch (messageID)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Left;
		break;
	}
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Right;
		break;
	}
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Middle;
		break;
	}
	}

	return mouseButton;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	GUISystem* gui = Application::GetInstance().GetGUISystem();

	if (gui && gui->WndProcHandler(hWnd, message, wParam, lParam))
	{
		return 1;
	}

	WindowPtr pWindow;
	{
		WindowMap::iterator iter = g_Windows.find(hWnd);
		if (iter != g_Windows.end())
		{
			pWindow = iter->second;
		}
	}

	if (pWindow)
	{
		switch (message)
		{
		case WM_PAINT:
		{
			UpdateEventArgs updateEventArgs(0.0f, 0.0f, Application::ms_FrameCount);
			// Delta time will be filled in by the Window.
			pWindow->OnUpdate(updateEventArgs);

			RenderEventArgs renderEventArgs(0.0f, 0.0f, Application::ms_FrameCount);
			// Delta time will be filled in by the Window.
			pWindow->OnRender(renderEventArgs);

			break;
		}
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			MSG charMsg;
			// Get the Unicode character (UTF-16)
			unsigned int c = 0;
			// For printable characters, the next message will be WM_CHAR.
			// This message contains the character code we need to send the KeyPressed event.
			// Inspired by the SDL 1.2 implementation.
			if (PeekMessage(&charMsg, hWnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
			{
				GetMessage(&charMsg, hWnd, 0, 0);
				c = static_cast<unsigned int>(charMsg.wParam);
			}

			bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			KeyCode::Key key = (KeyCode::Key)wParam;
			unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

			KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, shift, control, alt);
			pWindow->OnKeyPressed(keyEventArgs);

			Input::SetKeyState(key, true);
			break;
		}
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			KeyCode::Key key = (KeyCode::Key)wParam;
			unsigned int c = 0;
			unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

			// Determine which key was released by converting the key code and the scan code
			// to a printable character (if possible).
			// Inspired by the SDL 1.2 implementation.
			unsigned char keyboardState[256];
			GetKeyboardState(keyboardState);
			wchar_t translatedCharacters[4];
			if (int result = ToUnicodeEx(static_cast<UINT>(wParam), scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0)
			{
				c = translatedCharacters[0];
			}

			KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Released, shift, control, alt);
			pWindow->OnKeyReleased(keyEventArgs);

			Input::SetKeyState(key, false);
			break;
		}
		// The default window procedure will play a system notification sound
		// when pressing the Alt+Enter keyboard combination if this message is not handled.
		case WM_SYSCHAR:
			break;
		case WM_MOUSEMOVE:
		{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			MouseMotionEventArgs mouseMotionEventArgs(lButton, mButton, rButton, control, shift, x, y);
			pWindow->OnMouseMoved(mouseMotionEventArgs);
			break;
		}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y);
			pWindow->OnMouseButtonPressed(mouseButtonEventArgs);

			if (message == WM_LBUTTONDOWN)		Input::SetKeyState(KeyCode::Key::LButton, true);
			else if (message == WM_RBUTTONDOWN)	Input::SetKeyState(KeyCode::Key::RButton, true);
			else if (message == WM_MBUTTONDOWN)	Input::SetKeyState(KeyCode::Key::MButton, true);
			break;
		}
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y);
			pWindow->OnMouseButtonReleased(mouseButtonEventArgs);

			if (message == WM_LBUTTONUP)		Input::SetKeyState(KeyCode::Key::LButton, false);
			else if (message == WM_RBUTTONUP)	Input::SetKeyState(KeyCode::Key::RButton, false);
			else if (message == WM_MBUTTONUP)	Input::SetKeyState(KeyCode::Key::MButton, false);

			break;
		}
		case WM_MOUSEWHEEL:
		{
			// The distance the mouse wheel is rotated. A positiove value indicates the wheel was rotated to the right.
			float zDelta = ((int)(short)HIWORD(wParam)) / (float)WHEEL_DELTA;
			short keyStates = (short)LOWORD(wParam);

			bool lButton = (keyStates & MK_LBUTTON) != 0;
			bool rButton = (keyStates & MK_RBUTTON) != 0;
			bool mButton = (keyStates & MK_MBUTTON) != 0;
			bool shift = (keyStates & MK_SHIFT) != 0;
			bool control = (keyStates & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			// Convert the screen coordinates to client coordinates.
			POINT clientToScreenPoint;
			clientToScreenPoint.x = x;
			clientToScreenPoint.y = y;
			ScreenToClient(hWnd, &clientToScreenPoint);

			MouseWheelEventArgs mouseWheelEventArgs(zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y);
			pWindow->OnMouseWheel(mouseWheelEventArgs);
			break;
		}
		case WM_SIZE:
		{
			int width = ((int)(short)LOWORD(lParam));
			int height = ((int)(short)HIWORD(lParam));

			ResizeEventArgs resizeEventArgs(width, height);
			pWindow->OnResize(resizeEventArgs);
			break;
		}
		case WM_KILLFOCUS:
		{
			Input::ClearStates();
			break;
		}
		case WM_DESTROY:
		{
			// If a window is being destroyed, remove it from the window maps.
			RemoveWindow(hWnd);

			if (g_Windows.empty())
			{
				// If there are no more window, quit the application.
				PostQuitMessage(0);
			}
			break;
		}
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
	}
	else
	{
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}

	return 0;
}