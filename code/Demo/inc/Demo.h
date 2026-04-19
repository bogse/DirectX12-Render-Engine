#pragma once

#include "RenderApp.h"
#include "IndexBuffer.h"
#include "Window.h"
#include "VertexBuffer.h"
#include "DescriptorAllocation.h"

#include <DirectXMath.h>

class RootSignature;
class DescriptorAllocation;

class Demo : public RenderApp
{
public:
	using Super = RenderApp;

	Demo(const std::wstring& name, int width, int height, bool vSync = false);
	virtual ~Demo();

	bool LoadContent() override;
	void UnloadContent() override;

protected:
	void OnUpdate(UpdateEventArgs& eventArgs) override;
	void OnRender(RenderEventArgs& eventArgs) override;
	void OnKeyPressed(KeyEventArgs& eventArgs) override;
	void OnMouseWheel(MouseWheelEventArgs& eventArgs) override;
	void OnResize(ResizeEventArgs& eventArgs) override;

private:
	// Resize the depth buffer to match the size of the client area.
	void ResizeDepthBuffer(int width, int height);

	// Vertex buffer for the cube.
	VertexBuffer m_VertexBuffer;
	IndexBuffer m_IndexBuffer;

	// Depth buffer.
	Resource m_DepthBuffer;
	// Descriptor (DSV) used to bind the depth buffer to the pipeline.
	DescriptorAllocation m_DSV;

	std::unique_ptr<RootSignature> m_RootSignature;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	float m_FoV;

	DirectX::XMMATRIX m_ModelMatrix;
	DirectX::XMMATRIX m_ViewMatrix;
	DirectX::XMMATRIX m_ProjectionMatrix;
};