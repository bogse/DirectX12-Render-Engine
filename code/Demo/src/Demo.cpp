#include "Demo.h"

#include "Application.h"
#include "CommandList.h"
#include "CommandQueue.h"
#include "DescriptorAllocation.h"
#include "GUISystem.h"
#include "Helpers.h"
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

namespace
{
	// Build a look-at (world) matrix from a point, up and direction vectors.
	XMMATRIX XM_CALLCONV LookAtMatrix(FXMVECTOR position, FXMVECTOR direction, FXMVECTOR up)
	{
		assert(!XMVector3Equal(direction, XMVectorZero()));
		assert(!XMVector3Equal(up, XMVectorZero()));

		// Generate an LH View orientation matrix looking from origin(0, 0, 0).
		XMMATRIX viewMatrix = XMMatrixLookToLH(XMVectorZero(), direction, up);

		// Transpose the 3x3 orientation to invert it back into a World Matrix
		// and append the translation position direclty into the 4th row (r[3]).
		XMMATRIX worldMatrix = XMMatrixTranspose(viewMatrix);
		worldMatrix.r[3] = XMVectorSetW(position, 1.f);

		return worldMatrix;
	}

	template<typename T, typename Func>
	void RenderUILightCategory(const char* title, std::vector<T>& lights, Func editCallback, int idOffset)
	{
		if (ImGui::CollapsingHeader(title))
		{
			for (size_t i = 0; i < lights.size(); ++i)
			{
				const int id = static_cast<int>(i) + idOffset;
				ImGui::PushID(id);
				ImGui::Text("%s #%d", title, id);

				editCallback(lights[i]);

				ImGui::Separator();
				ImGui::PopID();
			}
		}
	}
}

Demo::Demo(const std::wstring& name, int width, int height, bool vSync)
	: Super(name, width, height, vSync)
	, m_PipelineOptions{true, true}
	, m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
	, m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
	, m_ModelMatrix(DirectX::XMMatrixIdentity())
	, m_Camera(Camera())
	, m_CameraController(m_Camera)
	, m_CubeMesh(nullptr)
	, m_SphereMesh(nullptr)
	, m_ConeMesh(nullptr)
	, m_CubeAnimation{ 90.f, 0.f, true }
	, m_CubeTransform
	{
		{ 0.f, 0.f, 0.f },
		{ 0.f, 0.f, 0.f },
		{ 1.f, 1.f, 1.f }
	}
	, m_DirectionalLights()
	, m_PointLights()
	, m_SpotLights()
	, m_RenderWireframe(false)
	, m_EnableTextures(true)
	, m_EnableMips(true)
{
	DirectX::XMVECTOR cameraPos = DirectX::XMVectorSet(0.f, 0.f, -10.f, 1.f);
	m_Camera.SetPosition(cameraPos);
}

bool Demo::LoadContent()
{
	std::shared_ptr<CommandQueue> commandQueue = Application::GetInstance().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	std::shared_ptr<CommandList> commandList = commandQueue->GetCommandList();

	// Load texture.
	const std::wstring path = ASSET_DIR L"/Textures/DirectX12.png";
	commandList->LoadTextureFromFile(m_DirectXTexture, path, true);

	// Create meshes.
	m_CubeMesh = Mesh::CreateCube(*commandList, 2.f);
	m_SphereMesh = Mesh::CreateSphere(*commandList, 0.25f);
	m_ConeMesh = Mesh::CreateCone(*commandList, 0.25f, 0.25f);

	// Create an off-screen render target with a single color buffer and a depth buffer.
	const DXGI_SAMPLE_DESC& sampleDesc = m_RenderTarget.GetSampleDesc();

	const CD3DX12_RESOURCE_DESC colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		GetClientWidth(), GetClientHeight(),
		1, 1,
		sampleDesc.Count, sampleDesc.Quality,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	D3D12_CLEAR_VALUE colorClearValue;
	colorClearValue.Format = colorDesc.Format;
	colorClearValue.Color[0] = 0.4f;
	colorClearValue.Color[1] = 0.6f;
	colorClearValue.Color[2] = 0.9f;
	colorClearValue.Color[3] = 1.0f;

	std::shared_ptr<Texture> colorTexture = std::make_shared<Texture>(
		colorDesc, &colorClearValue, D3D12_RESOURCE_STATE_RENDER_TARGET, L"Color Render Target");

	// Create a depth buffer.
	const CD3DX12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		GetClientWidth(),
		GetClientHeight(),
		1, 1,
		sampleDesc.Count, sampleDesc.Quality,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
	depthClearValue.DepthStencil = { 1.f, 0u };

	std::shared_ptr<Texture> depthTexture = std::make_shared<Texture>(
		depthDesc, &depthClearValue, D3D12_RESOURCE_STATE_DEPTH_WRITE, L"Depth Render Target");

	// Attach the textures to the render target.
	m_RenderTarget.AttachTexture(AttachmentPoint::Color0, std::move(colorTexture));
	m_RenderTarget.AttachTexture(AttachmentPoint::DepthStencil, std::move(depthTexture));

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

	// Root parameters.
	CD3DX12_ROOT_PARAMETER1 rootParameters[8];

	// MaterialCB for VertexShader (b0)
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

	// MaterialCB for PixelShader (b0, space1)
	rootParameters[1].InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

	// Texture shader resource view descriptor table (t3)
	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange;
	descriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
	rootParameters[2].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

	// LightPropertiesCB (b1)
	rootParameters[3].InitAsConstants(3, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	// DirectionalLights Structured Buffer SRV (t0)
	rootParameters[4].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

	// PointLights Structured Buffer SRV (t1)
	rootParameters[5].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

	// SpotLights Structured Buffer SRV (t2)
	rootParameters[6].InitAsShaderResourceView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

	// PipelineOptionsCB (b0, space9)
	rootParameters[7].InitAsConstantBufferView(0, 9, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler;
	linearRepeatSampler.Init(
		0,
		D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	linearRepeatSampler.MipLODBias = 1.f;

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
			CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC			SampleDesc;
		} pipelineStateStream;

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
		pipelineStateStream.DSVFormat = m_RenderTarget.GetDepthStencilFormat();
		pipelineStateStream.RTVFormats = m_RenderTarget.GetRenderTargetFormats();
		pipelineStateStream.SampleDesc = m_RenderTarget.GetSampleDesc();
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

	m_ActiveMaterial = MaterialPresets::CreateSatinWood();

	DirectionalLight sun;
	sun.DirectionWS = DirectX::XMFLOAT4(0.5f, -0.5f, 0.7f, 0.f);
	sun.Color		= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);

	m_DirectionalLights.push_back(sun);

	PointLight pointLight;
	pointLight.PositionWS			= DirectX::XMFLOAT4(0.f, -3.f, 0.f, 1.f);
	pointLight.Color				= DirectX::XMFLOAT4(1.f, 0.f, 0.f, 1.f);
	pointLight.ConstantAttenuation  = 1.f;
	pointLight.LinearAttenuation	= 0.1f;
	pointLight.QuadraticAttenuation = 0.02f;

	m_PointLights.push_back(pointLight);

	float outerAngleDegrees = 35.f;

	SpotLight spotLight;
	spotLight.PositionWS			= DirectX::XMFLOAT4(-2.f, 2.f, -2.f, 1.f);
	spotLight.DirectionWS			= DirectX::XMFLOAT4(0.6f, -0.6f, 0.6f, 0.f);
	spotLight.Color					= DirectX::XMFLOAT4(0.f, 0.f, 2.f, 1.f);
	spotLight.SpotAngle				= cosf(DirectX::XMConvertToRadians(outerAngleDegrees));
	spotLight.ConstantAttenuation	= 1.f;
	spotLight.LinearAttenuation		= 0.05f;
	spotLight.QuadraticAttenuation	= 0.01f;

	m_SpotLights.push_back(spotLight);

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
	UpdateLights();
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
	std::shared_ptr<Texture> finalColorTex = m_RenderTarget.GetTexture(AttachmentPoint::Color0);
	m_pWindow->Present(finalColorTex);
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

		m_RenderTarget.Resize(eventArgs.m_Width, eventArgs.m_Height);
	}
}

void Demo::RenderScenePass(CommandList* commandList)
{
	// Clear the render targets.
	{
		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		commandList->ClearRenderTargetTexture(
			*m_RenderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
		commandList->ClearDepthStencilTexture(
			*m_RenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
	}

	// Set pipeline state
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
	commandList->SetRenderTarget(m_RenderTarget);

	// Set the root signature slots.
	const XMMATRIX viewMatrix = m_Camera.GetViewMatrix();
	const XMMATRIX projectionMatrix = m_Camera.GetProjectionMatrix();

	struct TransformMatrices
	{
		XMMATRIX ModelView;
		XMMATRIX ModelViewProjection;
	} transformMatrices;

	transformMatrices.ModelView = m_ModelMatrix * viewMatrix;
	transformMatrices.ModelViewProjection = transformMatrices.ModelView * projectionMatrix;

	commandList->SetGraphicsDynamicConstantBuffer(0, transformMatrices);

	commandList->SetGraphicsDynamicConstantBuffer(1, m_ActiveMaterial);

	if (m_EnableTextures)
	{
		commandList->SetShaderResourceView(2, 0, m_DirectXTexture,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	LightProperties lightProperties;
	lightProperties.NumDirectionalLights = static_cast<uint32_t>(m_DirectionalLights.size());
	lightProperties.NumPointLights		 = static_cast<uint32_t>(m_PointLights.size());
	lightProperties.NumSpotLights		 = static_cast<uint32_t>(m_SpotLights.size());

	commandList->SetGraphics32BitConstants(3, lightProperties);
	commandList->SetGraphicsDynamicStructuredBuffer(4, m_DirectionalLights);
	commandList->SetGraphicsDynamicStructuredBuffer(5, m_PointLights);
	commandList->SetGraphicsDynamicStructuredBuffer(6, m_SpotLights);

	m_PipelineOptions.EnableTextures = m_EnableTextures ? 1 : 0;
	m_PipelineOptions.EnableMips = m_EnableMips ? 1 : 0;

	commandList->SetGraphicsDynamicConstantBuffer(7, m_PipelineOptions);

	// Draw.
	m_CubeMesh->Draw(*commandList);

	Material lightMaterial;
	lightMaterial.Ambient  = { 0.f, 0.f, 0.f, 1.f };
	lightMaterial.Diffuse  = { 0.f, 0.f, 0.f, 1.f };
	lightMaterial.Specular = { 0.f, 0.f, 0.f, 1.f };

	for (const PointLight& pointLight : m_PointLights)
	{
		lightMaterial.Emissive = pointLight.Color;

		DirectX::XMVECTOR lightPosition = DirectX::XMLoadFloat4(&pointLight.PositionWS);

		DirectX::XMMATRIX lightModelMatrix = DirectX::XMMatrixTranslationFromVector(lightPosition);

		TransformMatrices lightTransforms;
		lightTransforms.ModelView = lightModelMatrix * viewMatrix;
		lightTransforms.ModelViewProjection = lightTransforms.ModelView * projectionMatrix;

		commandList->SetGraphicsDynamicConstantBuffer(0, lightTransforms);
		commandList->SetGraphicsDynamicConstantBuffer(1, lightMaterial);

		m_SphereMesh->Draw(*commandList);
	}

	for (const SpotLight& spotLight : m_SpotLights)
	{
		lightMaterial.Emissive = spotLight.Color;

		DirectX::XMVECTOR lightPosition = DirectX::XMLoadFloat4(&spotLight.PositionWS);
		DirectX::XMVECTOR lightDirection = DirectX::XMLoadFloat4(&spotLight.DirectionWS);
		DirectX::XMVECTOR normDirection = DirectX::XMVector3Normalize(lightDirection);

		DirectX::XMVECTOR defaultUp = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);
		DirectX::XMVECTOR fallbackUp = DirectX::XMVectorSet(0.f, 0.f, 1.f, 0.f);

		// Prevent LookAt singularity when light direction aligns with the default up vector.
		DirectX::XMVECTOR dotValue = DirectX::XMVectorAbs(DirectX::XMVector3Dot(normDirection, defaultUp));
		DirectX::XMVECTOR controlMask = DirectX::XMVectorGreaterOrEqual(dotValue, DirectX::XMVectorReplicate(0.999f));

		// Branchless fallback choice: Select fallbackUp if parallel, otherwise keep defaultUp.
		DirectX::XMVECTOR up = DirectX::XMVectorSelect(defaultUp, fallbackUp, controlMask);

		DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(-90.f));
		DirectX::XMMATRIX worldMatrix = rotationMatrix * LookAtMatrix(lightPosition, lightDirection, up);

		TransformMatrices lightTransforms;
		lightTransforms.ModelView = worldMatrix * viewMatrix;
		lightTransforms.ModelViewProjection = lightTransforms.ModelView * projectionMatrix;

		commandList->SetGraphicsDynamicConstantBuffer(0, lightTransforms);
		commandList->SetGraphicsDynamicConstantBuffer(1, lightMaterial);

		m_ConeMesh->Draw(*commandList);
	}
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
		m_CubeMesh->UpdateCubeColors(*commandList, imGuiColors);
	}

	ImGui::Checkbox("Render wireframe", &m_RenderWireframe);

	ImGui::Checkbox("Enable textures", &m_EnableTextures);
	ImGui::Checkbox("Enable mips", &m_EnableMips);

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

	if (ImGui::CollapsingHeader("Material Properties"))
	{
		ImGui::ColorEdit3("Ambient Tint", &m_ActiveMaterial.Ambient.x);
		ImGui::ColorEdit3("Diffuse Tint", &m_ActiveMaterial.Diffuse.x);
		ImGui::ColorEdit3("Specular Tint", &m_ActiveMaterial.Specular.x);
		ImGui::ColorEdit3("Emissive Tint", &m_ActiveMaterial.Emissive.x);

		ImGui::Separator();

		ImGui::SliderFloat("Specular Power", &m_ActiveMaterial.SpecularPower, 1.f, 256.f);

		ImGui::Separator();

		ImGui::Text("Presets:");

		if (ImGui::Button("Default Matte"))
		{
			m_ActiveMaterial = Material();
		}

		ImGui::SameLine();

		if (ImGui::Button("Satin Wood"))
		{
			m_ActiveMaterial = MaterialPresets::CreateSatinWood();
		}
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

	if (ImGui::CollapsingHeader("Lighting"))
	{
		ImGui::Indent();

		const auto editDirectionalLight = [](DirectionalLight& directionalLight) {
			if (ImGui::SliderFloat3("Direction", &directionalLight.DirectionWS.x, -1.f, 1.f))
			{
				XMVECTOR directionVector = XMLoadFloat4(&directionalLight.DirectionWS);
				XMStoreFloat4(&directionalLight.DirectionWS, XMVector3Normalize(directionVector));
			}

			ImGui::ColorEdit3("Color", &directionalLight.Color.x);
			};

		const auto editPointLight = [](PointLight& pointLight) {
			ImGui::SliderFloat3("Position", &pointLight.PositionWS.x, -10.f, 10.f);
			ImGui::ColorEdit3("Color", &pointLight.Color.x);
			ImGui::SliderFloat("Constant Attenuation", &pointLight.ConstantAttenuation, 0.01f, 2.f);
			ImGui::SliderFloat("Linear Attenuation", &pointLight.LinearAttenuation, 0.f, 1.f);
			ImGui::SliderFloat("Quadratic Attenuation", &pointLight.QuadraticAttenuation, 0.f, 0.5f);
			};

		const auto editSpotLight = [](SpotLight& spotLight) {
			ImGui::SliderFloat3("Position", &spotLight.PositionWS.x, -10.f, 10.f);

			if (ImGui::SliderFloat3("Direction", &spotLight.DirectionWS.x, -1.f, 1.f))
			{
				XMVECTOR directionVector = XMLoadFloat4(&spotLight.DirectionWS);
				if (*XMVector3LengthSq(directionVector).m128_f32 > 0.01f)
				{
					XMStoreFloat4(&spotLight.DirectionWS, XMVector3Normalize(directionVector));
				}
			}

			ImGui::ColorEdit3("Color", &spotLight.Color.x);

			const float currentAngle = acosf(spotLight.SpotAngle);
			float degrees = XMConvertToDegrees(currentAngle);
			if (ImGui::SliderFloat("Beam Angle", &degrees, 1.f, 89.f))
			{
				spotLight.SpotAngle = cosf(XMConvertToRadians(degrees));
			}

			ImGui::SliderFloat("Constant Attenuation", &spotLight.ConstantAttenuation, 0.01f, 2.f);
			ImGui::SliderFloat("Linear Attenuation", &spotLight.LinearAttenuation, 0.f, 1.f);
			ImGui::SliderFloat("Quadratic Attenuation", &spotLight.QuadraticAttenuation, 0.f, 0.5f);
		};

		RenderUILightCategory("Directional Light", m_DirectionalLights, editDirectionalLight, 0);
		RenderUILightCategory("Point Light"		 , m_PointLights	  , editPointLight		, 100);
		RenderUILightCategory("Spotlight"		 , m_SpotLights		  , editSpotLight		, 200);

		ImGui::Unindent();
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

void Demo::UpdateLights()
{
	DirectX::XMMATRIX viewMatrix = m_Camera.GetViewMatrix();

	for (DirectionalLight& directionalLight : m_DirectionalLights)
	{
		DirectX::XMVECTOR directionWS = DirectX::XMLoadFloat4(&directionalLight.DirectionWS);
		DirectX::XMVECTOR directionVS = DirectX::XMVector3TransformNormal(directionWS, viewMatrix);
		DirectX::XMStoreFloat4(&directionalLight.DirectionVS, DirectX::XMVector3Normalize(directionVS));
	}

	for (PointLight& pointLight : m_PointLights)
	{
		DirectX::XMVECTOR positionWS = DirectX::XMLoadFloat4(&pointLight.PositionWS);
		DirectX::XMVECTOR positionVS = DirectX::XMVector3TransformNormal(positionWS, viewMatrix);
		DirectX::XMStoreFloat4(&pointLight.PositionVS, positionVS);
	}

	for (SpotLight& spotLight : m_SpotLights)
	{
		DirectX::XMVECTOR positionWS = DirectX::XMLoadFloat4(&spotLight.PositionWS);
		DirectX::XMVECTOR positionVS = DirectX::XMVector3Transform(positionWS, viewMatrix);
		DirectX::XMStoreFloat4(&spotLight.PositionVS, positionVS);

		DirectX::XMVECTOR directionWS = DirectX::XMLoadFloat4(&spotLight.DirectionWS);
		DirectX::XMVECTOR directionVS = DirectX::XMVector3TransformNormal(directionWS, viewMatrix);
		DirectX::XMStoreFloat4(&spotLight.DirectionVS, directionVS);
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
