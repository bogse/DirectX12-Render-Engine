#include "DX12LibPCH.h"

#include "SwapChain.h"

#include "Adapter.h"
#include "CommandList.h"
#include "CommandQueue.h"
#include "Device.h"
#include "RenderTarget.h"
#include "ResourceStateTracker.h"
#include "Texture.h"

SwapChain::SwapChain(Device& device, HWND hWnd)
	: m_Device(device)
	, m_CommandQueue(device.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT))
	, m_BackBufferTextures{ nullptr }
	, m_FenceValues{ 0 }
	, m_hWnd(hWnd)
	, m_Width(0u)
	, m_Height(0u)
	, m_VSync(true)
	, m_IsTearingSupported(false)
	, m_Fullscreen(false)
{
	assert(hWnd);

	std::shared_ptr<Adapter> adapter = device.GetAdapter();
	const Microsoft::WRL::ComPtr<IDXGIAdapter>& dxgiAdapter = adapter->GetDXGIAdapter();

	Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGIFactory5> dxgiFactory5;
	ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));
	ThrowIfFailed(dxgiFactory.As(&dxgiFactory5));

	BOOL allowTearing = FALSE;
	if (SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(BOOL))))
	{
		m_IsTearingSupported = (allowTearing == TRUE);
	}

	RECT windowRect;
	::GetClientRect(hWnd, &windowRect);

	m_Width  = windowRect.right - windowRect.left;
	m_Height = windowRect.bottom - windowRect.top;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width			= m_Width;
	swapChainDesc.Height		= m_Height;
	swapChainDesc.Format		= DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo		= FALSE;
	swapChainDesc.SampleDesc	= { 1, 0 };
	swapChainDesc.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount	= BufferCount;
	swapChainDesc.Scaling		= DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect	= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode		= DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommented to always allow tearing if tearing support is available.
	swapChainDesc.Flags = m_IsTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& d3d12CommandQueue = m_CommandQueue.GetD3D12CommandQueue();

	Microsoft::WRL::ComPtr<IDXGISwapChain1> dxgiSwapChain1;
	ThrowIfFailed(dxgiFactory5->CreateSwapChainForHwnd(
		d3d12CommandQueue.Get(), m_hWnd, &swapChainDesc, nullptr, nullptr, &dxgiSwapChain1));

	ThrowIfFailed(dxgiSwapChain1.As(&m_dxgiSwapChain));
	
	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
	ThrowIfFailed(dxgiFactory5->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER));

	m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

	m_dxgiSwapChain->SetMaximumFrameLatency(1);

	m_hFrameLatencyWaitableObject = m_dxgiSwapChain->GetFrameLatencyWaitableObject();

	for (int i = 0; i < BufferCount; ++i)
	{
		std::wstring resourceName = L"Backbuffer[" + std::to_wstring(i) + L"]";
		m_BackBufferTextures[i] = std::make_shared<Texture>(resourceName);
	}

	UpdateRenderTargetViews();
}

UINT SwapChain::Present(const std::shared_ptr<Texture>& texture)
{
	std::shared_ptr<CommandList> commandList = m_CommandQueue.GetCommandList();
	std::shared_ptr<Texture> backBuffer = m_BackBufferTextures[m_CurrentBackBufferIndex];

	if (texture)
		commandList->CopyResource(*backBuffer, *texture);

	commandList->TransitionBarrier(*backBuffer, D3D12_RESOURCE_STATE_PRESENT);
	m_CommandQueue.ExecuteCommandList(commandList);

	UINT syncInterval = m_VSync ? 1 : 0;
	UINT presentFlags = m_IsTearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(m_dxgiSwapChain->Present(syncInterval, presentFlags));

	m_FenceValues[m_CurrentBackBufferIndex] = m_CommandQueue.Signal();

	m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

	const UINT64 fenceValue = m_FenceValues[m_CurrentBackBufferIndex];
	m_CommandQueue.WaitForFenceValue(fenceValue);

	m_Device.ReleaseStaleDescriptors(fenceValue);

	return m_CurrentBackBufferIndex;
}

void SwapChain::WaitForSwapChain()
{
	// Wait for 1 second (should never have to wait that long...)
	DWORD result = ::WaitForSingleObjectEx(m_hFrameLatencyWaitableObject, 1000, TRUE);
}

void SwapChain::Resize(uint32_t width, uint32_t height)
{
	if (m_Width != width || m_Height != height)
	{
		m_Width = std::max(1u, width);
		m_Height = std::max(1u, height);

		m_Device.Flush();

		for (int i = 0; i < BufferCount; ++i)
		{
			ID3D12Resource* resource = m_BackBufferTextures[i]->GetD3D12Resource().Get();
			ResourceStateTracker::RemoveGlobalResourceState(resource);
			m_BackBufferTextures[i]->Reset();
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(m_dxgiSwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(m_dxgiSwapChain->ResizeBuffers(
			BufferCount, m_Width, m_Height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

		UpdateRenderTargetViews();
	}
}

void SwapChain::UpdateRenderTargetViews()
{
	for (int i = 0; i < BufferCount; ++i)
	{
		m_BackBufferTextures[i]->SetName(L"Backbuffer[" + std::to_wstring(i) + L"]");

		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(m_dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		ResourceStateTracker::AddGlobalResourceState(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);

		m_BackBufferTextures[i]->SetD3D12Resource(backBuffer);
		m_BackBufferTextures[i]->CreateViews();
	}
}
