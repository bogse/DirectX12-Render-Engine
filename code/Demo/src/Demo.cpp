#include "Demo.h"

#include "Application.h"
#include "CommandList.h"
#include "CommandQueue.h"
#include "DescriptorAllocation.h"
#include "GUISystem.h"
#include "Helpers.h"
#include "Mesh.h"
#include "RootSignature.h"
#include "Window.h"

#include <wrl.h>

#include <d3dx12.h>
#include <d3dcompiler.h>

#include <algorithm>

#if defined(min)
	#undef min
#endif

#if defined(max)
	#undef max
#endif

using namespace DirectX;

Demo::Demo(const std::wstring& name, int width, int height, bool vSync)
	: Super(name, width, height, vSync)
	, m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
	, m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
	, m_ModelMatrix(DirectX::XMMatrixIdentity())
	, m_ViewMatrix(DirectX::XMMatrixIdentity())
	, m_ProjectionMatrix(DirectX::XMMatrixIdentity())
	, m_CubeMesh(nullptr)
	, m_CubeAnimation{ 90.f, 0.f, true }
	, m_FoV(45.0)
	, m_RenderWireframe(false)
{
	m_DepthBuffer.SetName(L"Demo::DepthBuffer");
}

bool Demo::LoadContent()
{
	std::shared_ptr<CommandQueue> commandQueue = Application::GetInstance().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	std::shared_ptr<CommandList> commandList = commandQueue->GetCommandList();

	// Create a cube mesh.
	m_CubeMesh = Mesh::CreateCube(*commandList, 2.f);

	// Allocate space for the Depth Stencil View descriptor.
	m_DSV = Application::GetInstance().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// Load the vertex shader.
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"./VertexShader.cso", &vertexShaderBlob));

	// Load the pixel shader.
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"./PixelShader.cso", &pixelShaderBlob));

	// Create a root signature.
	Microsoft::WRL::ComPtr<ID3D12Device2> device = Application::GetInstance().GetDevice();

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// A single 32-bit constant root parameter that is used by the vertex shader.
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

	m_RootSignature = std::make_unique<RootSignature>(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

	auto CreatePSO = [this, &device, &vertexShaderBlob, &pixelShaderBlob](D3D12_FILL_MODE fillMode,
		Microsoft::WRL::ComPtr<ID3D12PipelineState>& outPSO)
	{
		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE		pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT			InputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY	PrimitiveTopologyType;
			CD3DX12_PIPELINE_STATE_STREAM_VS					VS;
			CD3DX12_PIPELINE_STATE_STREAM_PS					PS;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT	DSVFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER			RasterizerState;
		} pipelineStateStream;

		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

		CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
		rasterizerDesc.FillMode = fillMode;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

		pipelineStateStream.pRootSignature = m_RootSignature->GetRootSignature().Get();
		pipelineStateStream.InputLayout =
		{ 
			VertexPositionNormalTexture::InputElements,
			VertexPositionNormalTexture::InputElementCount 
		};
		pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		pipelineStateStream.RTVFormats = rtvFormats;
		pipelineStateStream.RasterizerState = rasterizerDesc;

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = 
		{
			sizeof(PipelineStateStream), 
			&pipelineStateStream
		};

		ThrowIfFailed(device->CreatePipelineState(
			&pipelineStateStreamDesc,
			IID_PPV_ARGS(&outPSO)));
	};

	CreatePSO(D3D12_FILL_MODE_SOLID, m_SolidPipelineState);
	CreatePSO(D3D12_FILL_MODE_WIREFRAME, m_WireframePipelineState);

	uint64_t fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

	// Resize/Create the depth buffer.
	ResizeDepthBuffer(GetClientWidth(), GetClientHeight());

	return true;
}

void Demo::UnloadContent()
{
}

void Demo::OnUpdate(UpdateEventArgs& eventArgs)
{
	static uint64_t frameCount = 0;
	static double totalTime = 0.0;

	Super::OnUpdate(eventArgs);

	totalTime += eventArgs.m_ElapsedTime;
	frameCount++;

	if (totalTime > 1.0)
	{
		double fps = frameCount / totalTime;

		char buffer[512];
		sprintf_s(buffer, "FPS: %f\n", fps);
		OutputDebugStringA(buffer);

		frameCount = 0;
		totalTime = 0.0;
	}

	// Update the model matrix.
	if (m_CubeAnimation.m_RotateCube)
	{
		m_CubeAnimation.m_CurrentAngle += 
			m_CubeAnimation.m_RotationSpeed * static_cast<float>(eventArgs.m_ElapsedTime);
	}

	const XMVECTOR rotationAxis = XMVectorSet(0.f, 1.f, 1.f, 0.f);
	m_ModelMatrix = XMMatrixRotationAxis(
		rotationAxis, XMConvertToRadians(m_CubeAnimation.m_CurrentAngle));

	// Update the view matrix.
	const XMVECTOR eyePosition = XMVectorSet(0.f, 0.f, -10.f, 1.f);
	const XMVECTOR focusPoint = XMVectorSet(0.f, 0.f, 0.f, 1.f);
	const XMVECTOR upDirection = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	// Update the projection matrix.
	float aspectRatio = GetClientWidth() / static_cast<float>(GetClientHeight());
	m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), aspectRatio, 0.1f, 100.0f);
}

void Demo::OnRender(RenderEventArgs& eventArgs)
{
	Super::OnRender(eventArgs);

	std::shared_ptr<CommandQueue> commandQueue = Application::GetInstance().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	std::shared_ptr<CommandList> commandList = commandQueue->GetCommandList();

	RenderScenePass(commandList.get());
	RenderUIPass(commandList.get());

	commandQueue->ExecuteCommandList(commandList);

	// Present.
	m_pWindow->Present();
}

void Demo::OnKeyPressed(KeyEventArgs& eventArgs)
{
	Super::OnKeyPressed(eventArgs);

	switch (eventArgs.m_Key)
	{
	case KeyCode::Key::Escape:
	{
		Application::GetInstance().Quit(0);
		break;
	}
	case KeyCode::Key::Enter:
		if (eventArgs.m_Alt)
		{
	case KeyCode::Key::F11:
		m_pWindow->ToggleFullscreen();
		break;
		}
	case KeyCode::Key::V:
	{
		m_pWindow->ToggleVSync();
		break;
	}
	}
}

void Demo::OnMouseWheel(MouseWheelEventArgs& eventArgs)
{
	m_FoV -= eventArgs.m_WheelDelta;
	m_FoV = std::clamp(m_FoV, 12.0f, 90.0f);

	char buffer[256];
	sprintf_s(buffer, "FoV: %f\n", m_FoV);
	OutputDebugStringA(buffer);
}

void Demo::OnResize(ResizeEventArgs& eventArgs)
{
	if (eventArgs.m_Width != GetClientWidth() || eventArgs.m_Height != GetClientHeight())
	{
		Super::OnResize(eventArgs);

		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
			static_cast<float>(eventArgs.m_Width), static_cast<float>(eventArgs.m_Height));

		ResizeDepthBuffer(eventArgs.m_Width, eventArgs.m_Height);
	}
}

void Demo::ResizeDepthBuffer(int width, int height)
{
	// Flush any GPU commands that might be referencing the depth buffer.
	Application::GetInstance().Flush();

	width = std::max(1, width);
	height = std::max(1, height);

	Microsoft::WRL::ComPtr<ID3D12Device2> device = Application::GetInstance().GetDevice();

	// Resize screen dependent resources.
	// Create a depth buffer.
	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil = { 1.0, 0 };

	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC depthDesc(CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
		1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));

	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&depthBuffer)
	));

	m_DepthBuffer.SetD3D12Resource(depthBuffer);

	// Create (initialize) the depth-stencil view.
	device->CreateDepthStencilView(depthBuffer.Get(), nullptr, m_DSV.GetDescriptorHandle());
}

void Demo::RenderScenePass(CommandList* commandList)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_pWindow->GetCurrentRenderTargetView();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_DSV.GetDescriptorHandle();
	// Clear the render targets.
	{
		const Resource& backBuffer = m_pWindow->GetCurrentRenderTarget();

		commandList->TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		commandList->ClearRTV(rtv, clearColor);
		commandList->ClearDepth(dsv);
	}

	// Set pipeline state and root signature.
	if (m_RenderWireframe)
	{
		commandList->SetPipelineState(m_WireframePipelineState.Get());
	}
	else
	{
		commandList->SetPipelineState(m_SolidPipelineState.Get());
	}
	commandList->SetGraphicsRootSignature(*m_RootSignature);

	// Setup the Rasterizer State.
	commandList->SetViewport(m_Viewport);
	commandList->SetScissorRect(m_ScissorRect);

	// Bind the Render Targets to the Output Merger stage.
	commandList->SetRenderTargets(&rtv, &dsv);

	// Update the MVP matrix.
	XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
	commandList->SetGraphicsDynamicConstantBuffer(0, mvpMatrix);

	// Draw.
	m_CubeMesh->Draw(*commandList);
}

void Demo::RenderUIPass(CommandList* commandList)
{
	GUISystem& gui = *Application::GetInstance().GetGUISystem();

	gui.BeginFrame();

	ImGui::Begin("Debug");

	ImGui::SliderFloat("FoV", &m_FoV, 10.f, 90.f);

	ImGui::Checkbox("Render wireframe", &m_RenderWireframe);

	ImGui::Checkbox("Rotate Cube", &m_CubeAnimation.m_RotateCube);
	ImGui::SliderFloat("Rotation Speed", &m_CubeAnimation.m_RotationSpeed, 0.f, 360.f);

	ImGui::End();

	gui.EndFrame();

	gui.Render(*commandList);
}
