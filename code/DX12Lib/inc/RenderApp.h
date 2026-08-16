#pragma once

#include "Event.h"

#include <memory>
#include <string>

class Device;
class GUISystem;
class SwapChain;
class Texture;
class Window;

class RenderApp
{
public:
	RenderApp(const std::wstring& name, int width, int height);
	virtual ~RenderApp();

	virtual bool Initialize();
	virtual bool LoadContent() = 0;
	virtual void UnloadContent() = 0;
	virtual void Destroy();

	int Run();

	int GetClientWidth() const
	{
		return m_Width;
	}

	int GetClientHeight() const
	{
		return m_Height;
	}

protected:
	friend class Window;

	/*
	* Present the swapchain's back buffer to the screen. Returns the current back buffer index after the present.
	*/
	UINT Present(const std::shared_ptr<Texture>& texture = nullptr);

	virtual void OnUpdate(UpdateEventArgs& eventArgs);
	virtual void OnRender(RenderEventArgs& eventArgs);

	virtual void OnKeyPressed(KeyEventArgs& eventArgs);
	virtual void OnKeyReleased(KeyEventArgs& eventArgs);
	virtual void OnMouseMoved(MouseMotionEventArgs& eventArgs);
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& eventArgs);
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& eventArgs);
	virtual void OnMouseWheel(MouseWheelEventArgs& eventArgs);
	virtual void OnResize(ResizeEventArgs& eventArgs);

	virtual void OnWindowDestroy();

	std::shared_ptr<Window> m_pWindow;
	std::shared_ptr<Device> m_Device;
	std::shared_ptr<SwapChain> m_SwapChain;
	std::unique_ptr<GUISystem> m_GUISystem;

	float m_FPS;

private:
	void RegisterEvents();

	std::wstring m_Name;
	int m_Width;
	int m_Height;
};
