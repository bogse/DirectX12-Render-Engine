#pragma once

#include "RenderTarget.h"

#include <dxgi1_5.h>
#include <wrl/client.h>

#include <memory>

class CommandQueue;
class Device;
class Texture;

class SwapChain
{
public:
	/**
	* Present the swap chain's back buffer to the screen.
	*
	* @param texture The texture to copy to the swap chain's backbuffer before presenting.
	* By default, this is an empty texture. In this case, no copy will be performed.
	* Use the SwapChain::GetRenderTarget method to get a render target for the window's color buffer.
	*
	* @returns The current backbuffer index after the present.
	*/
	UINT Present(const std::shared_ptr<Texture>& texture = nullptr);

	/**
	* Block the current thread until the swap chain has finished presenting.
	* Doing this at the beginning of the update loop can improve input latency.
	*/
	void WaitForSwapChain();

	/**
	* Resize the swap chain's back buffers.
	* This should be called whenever the window is resized.
	*/
	void Resize(const uint32_t width, const uint32_t height);

	/**
	* Swap chain is in fullscreen exclusive mode.
	*/
	bool IsFullscreen() const
	{
		return m_Fullscreen;
	}

	/**
	* Check to see if tearing is supported.
	* &see https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays
	*/
	bool IsTearingSupported() const
	{
		return m_IsTearingSupported;
	}

	Microsoft::WRL::ComPtr<IDXGISwapChain4> GetDXGISwapChain() const
	{
		return m_dxgiSwapChain;
	}

	/**
	* Set the swap chain to fullscreen exclusive mode (true) or windowed mode (false).
	*/
	void SetFullscreen(const bool fullscreen)
	{
		m_Fullscreen = (m_Fullscreen != fullscreen) ? fullscreen : m_Fullscreen;
	}

	/**
	* Toggle fullscreen exclusive mode.
	*/
	void ToggleFullscreen()
	{
		SetFullscreen(!m_Fullscreen);
	}

	void SetVSync(bool vSync)
	{
		m_VSync = vSync;
	}

	void ToggleVSync()
	{
		SetVSync(!m_VSync);
	}

protected:
	/**
	* Swap chains can only be created through the Device.
	*/
	SwapChain(Device& device, HWND hWnd);
	virtual ~SwapChain() = default;

private:
	/**
	* Update the swap chain's RTVs.
	*/
	void UpdateRenderTargetViews();

	/**
	* Number of swap chain back buffers.
	*/
	static constexpr UINT BufferCount = 3;

	/**
	* The device that was used to create the SwapChain.
	*/
	Device& m_Device;

	/**
	* The command queue taht is used to create the swap chain.
	* The command queue will be signaled right after the Present
	* to ensure that the swap chain's back buffers are not in-flight
	* before the next frame is allowed to be rendered.
	*/
	CommandQueue&							m_CommandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_dxgiSwapChain;
	std::shared_ptr<Texture>				m_BackBufferTextures[BufferCount];
	mutable RenderTarget					m_RenderTarget;

	/**
	* The current back buffer index of the swap chain.
	*/
	UINT m_CurrentBackBufferIndex;

	/**
	* The fence values to wait for before leaving the Present method.
	*/
	UINT64 m_FenceValues[BufferCount];

	/**
	* A handle to a waitable object. Used to wait for the swap chain before presenting.
	*/
	HANDLE m_hFrameLatencyWaitableObject;

	/**
	* The window handle that is associated with this swap chain.
	*/
	HWND m_hWnd;

	/**
	* The current width/height of the swap chain.
	*/
	uint32_t m_Width;
	uint32_t m_Height;

	/**
	* Synchronize presentation with the vertical refresh rate of the screen.
	* Set to true to eliminate screen tearing.
	*/
	bool m_VSync;

	/**
	* Whether or not tearing is supported.
	*/
	bool m_IsTearingSupported;

	/**
	* Whether the application is in fullscreen exclusive mode or windowed mode.
	*/
	bool m_Fullscreen;
};