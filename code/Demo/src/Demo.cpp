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

constexpr int imGuiVertexCount = 8;

static DirectX::XMFLOAT4 imGuiColors[imGuiVertexCount] = {
	{ 0.0f, 0.0f, 0.0f, 1.0f }, // Corner 0 : Black
	{ 1.0f, 0.0f, 0.0f, 1.0f }, // Corner 1 : Red
	{ 0.0f, 1.0f, 0.0f, 1.0f }, // Corner 2 : Green
	{ 1.0f, 1.0f, 0.0f, 1.0f }, // Corner 3 : Yellow
	{ 0.0f, 0.0f, 1.0f, 1.0f }, // Corner 4 : Blue
	{ 1.0f, 0.0f, 1.0f, 1.0f }, // Corner 5 : Magenta
	{ 0.0f, 1.0f, 1.0f, 1.0f }, // Corner 6 : Cyan
	{ 1.0f, 1.0f, 1.0f, 1.0f }  // Corner 7 : White
};

Demo::Demo(const std::wstring& name, int width, int height, bool vSync)
	: Super(name, width, height, vSync)
	, m_PipelineOptions{true}
	, m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
	, m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
	, m_ModelMatrix(DirectX::XMMatrixIdentity())
	, m_Camera(Camera())
	, m_CameraController(m_Camera)
	, m_CubeMesh(nullptr)
	, m_CubeAnimation{ 90.f, 0.f, true }
	, m_CubeTransform
	{
		{ 0.f, 0.f, 0.f },
		{ 0.f, 0.f, 0.f },
		{ 1.f, 1.f, 1.f }
	}
	, m_RenderWireframe(false)
	, m_EnableTextures(true)
{
	CD3DX12_CLEAR_VALUE clearValue(DXGI_FORMAT_D32_FLOAT, 1.f, 0);
	m_DepthBuffer = Texture(
		CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_D32_FLOAT, width, height,
			1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		&clearValue,
		D3D12_RESOURCE_STATE_COMMON, L"DepthBuffer");
	DirectX::XMVECTOR cameraPos = DirectX::XMVectorSet(0.f, 0.f, -10.f, 1.f);
	m_Camera.SetPosition(cameraPos);
}

bool Demo::LoadContent()
{
	std::shared_ptr<CommandQueue> commandQueue = Application::GetInstance().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	std::shared_ptr<CommandList> commandList = commandQueue->GetCommandList();

	// Load texture.
	const std::wstring path = ASSET_DIR L"/Textures/DirectX12.png";
	commandList->LoadTextureFromFile(m_DirectXTexture, path);
	m_DirectXTexture.SetName(L"DirectX12.png");

	// Create a cube mesh.
	m_CubeMesh = Mesh::CreateCube(*commandList, 2.f);

	// Load the vertex shader.
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"shaders/VertexShader.cso", &vertexShaderBlob));

	// Load the pixel shader.
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"shaders/PixelShader.cso", &pixelShaderBlob));

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
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// Root parameters:
	// 0 - Vertex shader constant buffer (b0)
	// 1 - Pixel shader constant buffer (b1)
	// 2 - Shader resource view descriptor table (t0)
	CD3DX12_ROOT_PARAMETER1 rootParameters[3];
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange;
	descriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	rootParameters[2].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler;
	linearRepeatSampler.Init(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters),
		rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);

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
			VertexPositionNormalColorTexture::InputElements,
			VertexPositionNormalColorTexture::InputElementCount
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
	Super::OnUpdate(eventArgs);
	
	const float deltaTime = static_cast<float>(eventArgs.m_ElapsedTime);
	
	m_CameraController.Update(deltaTime);
	m_Camera.Update();
	UpdateAnimation(deltaTime);
	UpdateModelMatrix();
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
	{
		if (eventArgs.m_Alt)
		{
			m_pWindow->ToggleFullscreen();
		}
		break;
	}
	case KeyCode::Key::F11:
	{
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

void Demo::OnMouseMoved(MouseMotionEventArgs& eventArgs)
{
	m_CameraController.OnMouseMoved(eventArgs.m_RelX, eventArgs.m_RelY);
}

void Demo::OnMouseWheel(MouseWheelEventArgs& eventArgs)
{
	m_CameraController.OnMouseWheel(eventArgs.m_WheelDelta);
}

void Demo::OnResize(ResizeEventArgs& eventArgs)
{
	if (eventArgs.m_Width != GetClientWidth() || eventArgs.m_Height != GetClientHeight())
	{
		Super::OnResize(eventArgs);

		const float aspectRatio = static_cast<float>(eventArgs.m_Width) /
								  static_cast<float>(eventArgs.m_Height);

		m_Camera.SetAspectRatio(aspectRatio);

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

	m_DepthBuffer.Resize(width, height);
}

void Demo::RenderScenePass(CommandList* commandList)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_pWindow->GetCurrentRenderTargetView();
	// Clear the render targets.
	{
		const Resource& backBuffer = m_pWindow->GetCurrentRenderTarget();

		commandList->TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		commandList->ClearRTV(rtv, clearColor);
		commandList->ClearDepthStencilTexture(m_DepthBuffer, D3D12_CLEAR_FLAG_DEPTH);
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
	const D3D12_CPU_DESCRIPTOR_HANDLE& dsvHandle = m_DepthBuffer.GetDepthStencilView();
	commandList->SetRenderTargets(&rtv, &dsvHandle);

	// Bind the texture SRV to root parameter 2 (t0 in the pixel shader)
	if (m_EnableTextures)
	{
		commandList->SetShaderResourceView(2, 0, m_DirectXTexture,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// Update the MVP matrix.
	const XMMATRIX viewMatrix = m_Camera.GetViewMatrix();
	const XMMATRIX projectionMatrix = m_Camera.GetProjectionMatrix();
	const XMMATRIX mvpMatrix = m_ModelMatrix * viewMatrix * projectionMatrix;

	commandList->SetGraphicsDynamicConstantBuffer(0, mvpMatrix);

	if (m_EnableTextures)
	{
		m_PipelineOptions.EnableTextures = 1;
	}
	else
	{
		m_PipelineOptions.EnableTextures = 0;
	}

	commandList->SetGraphicsDynamicConstantBuffer(1, m_PipelineOptions);

	// Draw.
	m_CubeMesh->Draw(*commandList);
}

void Demo::RenderUIPass(CommandList* commandList)
{
	GUISystem& gui = *Application::GetInstance().GetGUISystem();

	gui.BeginFrame();

	ImGui::Begin("Debug");

	if (ImGui::CollapsingHeader("Performance"))
	{
		ImGui::Text("FPS: %.2f", m_FPS);
		ImGui::Text("Frame Time: %.2f ms", m_FPS > 0.f ? 1000.f / m_FPS : 0.f);
	}

	bool bufferDirty = false;

	if (ImGui::CollapsingHeader("Vertex Colors"))
	{
		for (size_t i = 0; i < imGuiVertexCount; ++i)
		{
			std::string label = "Vertex " + std::to_string(i);
			if (ImGui::ColorEdit4(label.c_str(), &imGuiColors[i].x))
			{
				bufferDirty = true;
			}
		}
	}

	static float uniformColor[4] = { 1.f, 1.f, 1.f, 1.f };

	if (ImGui::ColorEdit4("Uniform color", uniformColor))
	{
		for (size_t i = 0; i < imGuiVertexCount; ++i)
		{
			imGuiColors[i] =
			{
				uniformColor[0],
				uniformColor[1],
				uniformColor[2],
				uniformColor[3]
			};
		}

		bufferDirty = true;
	}

	if (bufferDirty)
	{
		m_CubeMesh->UpdateColors(*commandList, imGuiColors);
	}

	ImGui::Checkbox("Render wireframe", &m_RenderWireframe);

	ImGui::Checkbox("Enable textures", &m_EnableTextures);

	static ImTextureID textureID = ImTextureID_Invalid;
	const Application& app = Application::GetInstance();
	ID3D12Device2* device = app.GetDevice().Get();
	GUISystem* guiSystem = app.GetGUISystem();

	if (ImGui::CollapsingHeader("Texture Debugger"))
	{
		if (!textureID)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE textureCpuHandle = m_DirectXTexture.GetShaderResourceView();

			textureID = guiSystem->RegisterTexture(device, textureCpuHandle);
		}

		ImGui::Image(textureID, ImVec2(126, 126));
	}
	else if (textureID)
	{
		guiSystem->UnregisterTexture(device);
		textureID = ImTextureID_Invalid;
	}

	if (ImGui::CollapsingHeader("Transform"))
	{
		ImGui::TextWrapped(
			"Warning: If the transform doesn't match the cube, "
			"reset the Animation Rotation Angle.");
		ImGui::DragFloat3("Position##Cube", &m_CubeTransform.m_Position.x, 0.1f);
		ImGui::DragFloat3("Rotation", &m_CubeTransform.m_RotationDeg.x, 1.f, 0.f, 360.f);
		ImGui::DragFloat3("Scale", &m_CubeTransform.m_Scale.x, 0.1f);
	}

	if (ImGui::CollapsingHeader("Animation"))
	{
		ImGui::Checkbox("Enable Rotation", &m_CubeAnimation.m_EnableRotation);
		ImGui::SliderFloat("Rotation Speed", &m_CubeAnimation.m_RotationSpeedDegPerSec, 0.f, 360.f);
		ImGui::SliderFloat("Rotation Angle", &m_CubeAnimation.m_RotationAngleDeg, 0.f, 360.f);
	}

	if (ImGui::CollapsingHeader("Camera"))
	{
		DirectX::XMFLOAT3 cameraPosition = m_Camera.GetPosition();

		if (ImGui::SliderFloat3("Position##Camera", &cameraPosition.x, -100.f, 100.f))
		{
			DirectX::XMVECTOR newCameraPosition = DirectX::XMLoadFloat3(&cameraPosition);
			m_Camera.SetPosition(newCameraPosition);
		}

		float fov = m_Camera.GetFOV();
		if (ImGui::SliderFloat("FOV", &fov, 10.f, 90.f))
		{
			m_Camera.SetFOV(fov);
		}

		float aspectRatio = m_Camera.GetAspectRatio();
		if (ImGui::SliderFloat("Aspect Ratio", &aspectRatio, 0.1f, 4.0f))
		{
			m_Camera.SetAspectRatio(aspectRatio);
		}

		float nearPlane = m_Camera.GetNearPlane();
		if (ImGui::DragFloat("Near Plane", &nearPlane, 0.1f, 0.1f, 100.f))
		{
			m_Camera.SetNearPlane(nearPlane);
		}

		float farPlane = m_Camera.GetFarPlane();
		if (ImGui::DragFloat("Far Plane", &farPlane, 1.f, nearPlane + 1.f, 10000.f))
		{
			m_Camera.SetFarPlane(farPlane);
		}
	}

	if (ImGui::CollapsingHeader("Camera Controller"))
	{
		float controllerYaw = m_CameraController.GetYaw();

		if (ImGui::SliderFloat("Yaw", &controllerYaw, -180.f, 180.f))
		{
			m_CameraController.SetYaw(controllerYaw);
		}

		float controllerPitch = m_CameraController.GetPitch();

		if (ImGui::SliderFloat("Pitch", &controllerPitch, -89.f, 89.f))
		{
			m_CameraController.SetPitch(controllerPitch);
		}

		float moveSpeed = m_CameraController.GetMoveSpeed();

		if (ImGui::DragFloat("Move Speed", &moveSpeed, 0.5f, 0.1f, 500.f))
		{
			m_CameraController.SetMoveSpeed(moveSpeed);
		}

		float mouseSensitivity = m_CameraController.GetMouseSensitivity();

		if (ImGui::DragFloat("Mouse Sensitivity", &mouseSensitivity, 0.01f, 0.01f, 1.f))
		{
			m_CameraController.SetMouseSensitivity(mouseSensitivity);
		}
	}

	ImGui::End();

	gui.EndFrame();

	gui.Render(*commandList);
}

void Demo::UpdateAnimation(float deltaTime)
{
	if (m_CubeAnimation.m_EnableRotation)
	{
		m_CubeAnimation.m_RotationAngleDeg +=
			m_CubeAnimation.m_RotationSpeedDegPerSec * deltaTime;

		m_CubeAnimation.m_RotationAngleDeg = fmod(m_CubeAnimation.m_RotationAngleDeg, 360.f);
	}
}

void Demo::UpdateModelMatrix()
{
	XMMATRIX scale = XMMatrixScaling(
		m_CubeTransform.m_Scale.x,
		m_CubeTransform.m_Scale.y,
		m_CubeTransform.m_Scale.z);

	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(m_CubeTransform.m_RotationDeg.x),
		XMConvertToRadians(m_CubeTransform.m_RotationDeg.y),
		XMConvertToRadians(m_CubeTransform.m_RotationDeg.z));

	const XMVECTOR rotationAxis = XMVectorSet(0.f, 1.f, 1.f, 0.f);
	XMMATRIX animatedRotation = XMMatrixRotationAxis(
		rotationAxis, XMConvertToRadians(m_CubeAnimation.m_RotationAngleDeg));

	rotation = rotation * animatedRotation;

	XMMATRIX translation = XMMatrixTranslation(
		m_CubeTransform.m_Position.x,
		m_CubeTransform.m_Position.y,
		m_CubeTransform.m_Position.z);

	m_ModelMatrix = scale * rotation * translation;
}
