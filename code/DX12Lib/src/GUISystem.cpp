#include "DX12LibPCH.h"

#include "GUISystem.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#include "CommandQueue.h"
#include "CommandList.h"
#include "Window.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool GUISystem::Initialize(HWND hwnd, ID3D12Device* device, CommandQueue* commandQueue)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui_ImplWin32_Init(hwnd);

	CreateDescriptorHeap(device);

	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device = device;
	initInfo.CommandQueue = commandQueue->GetD3D12CommandQueue().Get();
	initInfo.NumFramesInFlight = 2;
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;

	initInfo.SrvDescriptorHeap = m_SrvHeap.Get();

	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandle = m_SrvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle = m_SrvHeap->GetGPUDescriptorHandleForHeapStart();

	initInfo.LegacySingleSrvCpuDescriptor = cpuDescHandle;
	initInfo.LegacySingleSrvGpuDescriptor = gpuDescHandle;

	ImGui_ImplDX12_Init(&initInfo);

	return true;
}

void GUISystem::CreateDescriptorHeap(ID3D12Device* device)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;

	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_SrvHeap));
	assert(SUCCEEDED(hr));
}

void GUISystem::BeginFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void GUISystem::EndFrame()
{
	ImGui::Render();
}

void GUISystem::Render(CommandList& commandList)
{
	ID3D12DescriptorHeap* heaps[] = { m_SrvHeap.Get() };
	commandList.GetGraphicsCommandList()->SetDescriptorHeaps(1, heaps);

	ImGui_ImplDX12_RenderDrawData(
		ImGui::GetDrawData(),
		commandList.GetGraphicsCommandList().Get()
	);
}

LRESULT GUISystem::WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
}

void GUISystem::Shutdown()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	m_SrvHeap.Reset();
}
