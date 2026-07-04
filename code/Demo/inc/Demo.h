#pragma once

#include <Camera.h>
#include <CameraController.h>
#include <Light.h>
#include <Material.h>
#include <Mesh.h>
#include <RenderApp.h>
#include <RenderTarget.h>
#include <RootSignature.h>
#include <Texture.h>
#include <Window.h>

#include <DirectXMath.h>

class CommandList;

class Demo : public RenderApp
{
public:
	using Super = RenderApp;

	Demo(const std::wstring& name, int width, int height, bool vSync = false);

	bool LoadContent() override;
	void UnloadContent() override;

protected:
	void OnUpdate(UpdateEventArgs& eventArgs) override;
	void OnRender(RenderEventArgs& eventArgs) override;
	void OnKeyPressed(KeyEventArgs& eventArgs) override;
	void OnMouseMoved(MouseMotionEventArgs& eventArgs) override;
	void OnMouseWheel(MouseWheelEventArgs& eventArgs) override;
	void OnResize(ResizeEventArgs& eventArgs) override;

private:
	void RenderScenePass(CommandList* commandList);
	void RenderUIPass(CommandList* commandList);

	void UpdateAnimation(float deltaTime);
	void UpdateLights();
	void UpdateModelMatrix();

private:
	struct CubeAnimation
	{
		float m_RotationSpeedDegPerSec;
		float m_RotationAngleDeg;
		bool m_EnableRotation;
	};

	struct Transform
	{
		DirectX::XMFLOAT3 m_Position;
		DirectX::XMFLOAT3 m_RotationDeg;
		DirectX::XMFLOAT3 m_Scale;
	};

	struct PipelineOptionsCB
	{
		int EnableTextures;
		int EnableMips;
	};

	PipelineOptionsCB m_PipelineOptions;

	Texture m_DirectXTexture;
	Material m_ActiveMaterial;

	RenderTarget m_RenderTarget;

	std::unique_ptr<RootSignature> m_RootSignature;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_SolidPipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_WireframePipelineState;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	DirectX::XMMATRIX m_ModelMatrix;

	Camera m_Camera;
	CameraController m_CameraController;

	std::unique_ptr<Mesh> m_CubeMesh;
	std::unique_ptr<Mesh> m_SphereMesh;
	std::unique_ptr<Mesh> m_ConeMesh;

	CubeAnimation m_CubeAnimation;
	Transform m_CubeTransform;

	std::vector<DirectionalLight> m_DirectionalLights;
	std::vector<PointLight> m_PointLights;
	std::vector<SpotLight> m_SpotLights;

	bool m_RenderWireframe;
	bool m_EnableTextures;
	bool m_EnableMips;
};
