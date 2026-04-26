/**
* ImGui wapper.
*/

#include "imgui.h"

#include <d3dx12.h>
#include <wrl.h>

class CommandQueue;
class CommandList;
class Window;

class GUISystem
{
public:
	bool Initialize(HWND hwnd, ID3D12Device* device, CommandQueue* commandQueue);
	void Shutdown();

	void BeginFrame();
	void EndFrame();

	void Render(CommandList& commandList);

	LRESULT WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	void CreateDescriptorHeap(ID3D12Device* device);

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SrvHeap;
};
