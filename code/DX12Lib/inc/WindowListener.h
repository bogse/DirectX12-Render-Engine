#pragma once

#include "EventArgs.h"

class WindowListener
{
public:
	virtual ~WindowListener() = default;

    virtual void OnUpdate(UpdateEventArgs& eventArgs) {}
    virtual void OnRender(RenderEventArgs& eventArgs) {}
    virtual void OnKeyPressed(KeyEventArgs& eventArgs) {}
    virtual void OnKeyReleased(KeyEventArgs& eventArgs) {}
    virtual void OnMouseMoved(MouseMotionEventArgs& eventArgs) {}
    virtual void OnMouseButtonPressed(MouseButtonEventArgs& eventArgs) {}
    virtual void OnMouseButtonReleased(MouseButtonEventArgs& eventArgs) {}
    virtual void OnMouseWheel(MouseWheelEventArgs& eventArgs) {}
    virtual void OnResize(ResizeEventArgs& eventArgs) {}
    virtual void OnWindowDestroy() {}
};