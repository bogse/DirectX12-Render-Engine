#pragma once

#include "Events.h"

#include <memory>
#include <string>

class Window;

class RenderApp : public std::enable_shared_from_this<RenderApp>
{
public:
	RenderApp(const std::wstring& name, int width, int height, bool vSync);
	virtual ~RenderApp();

	virtual bool Initialize();
	virtual bool LoadContent() = 0;
	virtual void UnloadContent() = 0;
	virtual void Destroy();

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

private:
	std::wstring m_Name;
	int m_Width;
	int m_Height;
	bool m_vSync;
};