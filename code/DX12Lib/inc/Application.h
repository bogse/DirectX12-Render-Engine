/**
* The application class is used to create windows for our application.
*/
#pragma once

#include <windows.h>

#include <functional>
#include <memory>
#include <string>

class Window;
class WindowListener;

class Application
{
public:
	using WndProcHandlerCallback = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;

	// Create the application singleton with the application instance handle.
	static void Create(HINSTANCE hInst);

	// Destroy the application instance and all windows created by this application instance.
	static void Destroy();
	
	static Application& GetInstance();

    /**
	* Create a new DirectX12 render window instance.
	* @param windowName The name of the window. This name will appear in the title bar of the window. This name should be unique.
	* @param clientWidth The width (in pixels) of the window's client area.
	* @param clientHeight The height (in pixels) of the window's client area.
	* @param vSync Should the rendering be synchronized with the vertical refresh rate of the screen.
	* @param windowed If true, the window will be created in windowed mode. If false, the window will be created full-screen.
	* @returns The created window instance. If an error occurred while creating the window an invalid
	* window instance is returned. If a window with the given name already exists, that window will be
	* returned.
	*/
	std::shared_ptr<Window> CreateRenderWindow(
		WindowListener* listener,
		const std::wstring& windowName,
		int clientWidth,
		int clientHeight);

	void DestroyWindow(const std::wstring& windowName);
	void DestroyWindow(std::shared_ptr<Window> window);
	
	std::shared_ptr<Window> GetWindowByName(const std::wstring& windowName);

	HWND GetWindowHandle(const std::wstring& windowName);

	/**
	* Run the application loop and message pump. Return the error code if an error occurred.
	*/
	int Run();

	void ToggleFullscreen(const std::wstring& windowName);

	/**
	* Request to quit the application and close all windows. @param exitCode The error code to return to the invoking process.
	*/
	void Quit(int exitCode = 0);

	static WndProcHandlerCallback OnWndProcHandler;

protected:
	Application(HINSTANCE hInst);
	/**
	* Destroy the application instance and all windows associated with this application.
	*/
	virtual ~Application();

	void Initialize();

private:
	friend LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	Application(const Application& copy) = delete;
	Application& operator=(const Application& other) = delete;

	HINSTANCE m_hInstance;

	static uint64_t ms_FrameCount;
};
