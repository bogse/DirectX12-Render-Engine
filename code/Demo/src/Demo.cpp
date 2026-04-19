#include "Demo.h"

#include "Application.h"
#include "CommandList.h"
#include "CommandQueue.h"
#include "DescriptorAllocation.h"
#include "Helpers.h"
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

// Vertex data for a colored cube.
struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static WORD g_Indices[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

Demo::Demo(const std::wstring& name, int width, int height, bool vSync)
	: Super(name, width, height, vSync)
	, m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
	, m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
	, m_FoV(45.0)
{
	m_VertexBuffer.SetName(L"Demo::VertexBuffer");
	m_IndexBuffer.SetName(L"Demo::IndexBuffer");
	m_DepthBuffer.SetName(L"Demo::DepthBuffer");
}

Demo::~Demo()
{
}

bool Demo::LoadContent()
{
	std::shared_ptr<CommandQueue> commandQueue = Application::GetInstance().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	std::shared_ptr<CommandList> commandList = commandQueue->GetCommandList();

	// Upload vertex buffer data.
	commandList->CopyVertexBuffer(m_VertexBuffer, _countof(g_Vertices), sizeof(VertexPosColor), g_Vertices);

	// Upload index buffer data.
	commandList->CopyIndexBuffer(m_IndexBuffer, _countof(g_Indices), DXGI_FORMAT_R16_UINT, g_Indices);

	// Allocate space for the Depth Stencil View descriptor.
	m_DSV = Application::GetInstance().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// Load the vertex shader.
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"./VertexShader.cso", &vertexShaderBlob));

	// Load the pixel shader.
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"./PixelShader.cso", &pixelShaderBlob));

	// Create the vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0 , DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR"   , 0 , DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Create a root signature.
	Microsoft::WRL::ComPtr<ID3D12Device2> device = Application::GetInstance().GetDevice();

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and delay unnecessary access to certain pipeline stages.
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

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE		pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT			InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY	PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS					VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS					PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT	DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature = m_RootSignature->GetRootSignature().Get();
	pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineStateStream
	};
	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

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
	float angle = static_cast<float>(eventArgs.m_TotalTime * 90.0);
	const XMVECTOR rotationAxis = XMVectorSet(0.f, 1.f, 1.f, 0.f);
	m_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

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
	commandList->SetPipelineState(m_PipelineState.Get());
	commandList->SetGraphicsRootSignature(*m_RootSignature);

	// Setup the Input Assembler.
	commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetVertexBuffer(0, m_VertexBuffer);
	commandList->SetIndexBuffer(m_IndexBuffer);

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
	commandList->DrawIndexed(_countof(g_Indices));

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
	Super::OnResize(eventArgs);

	if (eventArgs.m_Width != GetClientWidth() || eventArgs.m_Height != GetClientHeight())
	{

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