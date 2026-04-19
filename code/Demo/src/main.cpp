#include <Application.h>
#include "Demo.h"

#include <dxgidebug.h>
#include <Shlwapi.h>

void ReportLiveObjects()
{
	IDXGIDebug1* dxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	dxgiDebug->Release();
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int mCmdShow)
{
	int retCode = 0;

	// Set the working directory to the path of the executable.
	WCHAR path[MAX_PATH];
	
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
	if (argv)
	{
		for (int i = 0; i < argc; ++i)
		{
			// -wd Speficfy the working directory.
			if (wcscmp(argv[i], L"-wd") == 0)
			{
				wcscpy_s(path, argv[++i]);
				SetCurrentDirectoryW(path);
			}
			LocalFree(argv);
		}
	}

	Application::Create(hInstance);
	{
		std::shared_ptr<Demo> demo = std::make_shared<Demo>(L"DirectX 12 Render Engine", 1280, 720);
		retCode = Application::GetInstance().Run(demo);
	}
	Application::Destroy();

	atexit(&ReportLiveObjects);

	return retCode;
}