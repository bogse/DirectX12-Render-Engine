#include "DX12LibPCH.h"

#include "Camera.h"

Camera::Camera()
	: m_ViewMatrix
	{
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	}
	, m_ProjectionMatrix
	{
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	}
	, m_Position{ 0.f, 0.f, 0.f }
	, m_Rotation{ 0.f, 0.f, 0.f, 1.f }
	, m_FOV(45.f)
	, m_AspectRatio(1.778f)
	, m_NearPlane(0.1f)
	, m_FarPlane(1000.f)
	, m_IsProjectionDirty(true)
	, m_IsViewDirty(true)
{
	Update();
}

void Camera::Update()
{
	if (m_IsViewDirty)
	{
		UpdateViewMatrix();
		m_IsViewDirty = false;
	}

	if (m_IsProjectionDirty)
	{
		UpdateProjectionMatrix();
		m_IsProjectionDirty = false;
	}
}

void Camera::SetProjection(const float fov, const float aspectRatio,
	const float nearZ, const float farZ)
{
	m_FOV = fov;
	m_AspectRatio = aspectRatio;
	m_NearPlane = nearZ;
	m_FarPlane = farZ;

	m_IsProjectionDirty = true;
}

void Camera::SetPosition(const DirectX::XMVECTOR position)
{
	DirectX::XMStoreFloat3(&m_Position, position);

	m_IsViewDirty = true;
}

void Camera::SetRotation(const DirectX::XMVECTOR rotation)
{
	DirectX::XMStoreFloat4(&m_Rotation, rotation);

	m_IsViewDirty = true;
}

void Camera::SetFOV(const float fov)
{
	m_FOV = fov;
	m_IsProjectionDirty = true;
}

void Camera::SetAspectRatio(const float aspectRatio)
{
	m_AspectRatio = aspectRatio;
	m_IsProjectionDirty = true;
}

void Camera::SetNearPlane(const float nearPlane)
{
	m_NearPlane = std::max(0.01f, nearPlane);
	
	ValidateClipPlanes();

	m_IsProjectionDirty = true;
}

void Camera::SetFarPlane(const float farPlane)
{
	m_FarPlane = std::max(farPlane, m_NearPlane + 0.1f);
	
	ValidateClipPlanes();

	m_IsProjectionDirty = true;
}

void Camera::UpdateViewMatrix()
{
	DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&m_Position);
	DirectX::XMVECTOR rotation = DirectX::XMLoadFloat4(&m_Rotation);

	DirectX::XMMATRIX transform =
		DirectX::XMMatrixRotationQuaternion(rotation) *
		DirectX::XMMatrixTranslationFromVector(position);

	DirectX::XMMATRIX view = DirectX::XMMatrixInverse(nullptr, transform);

	DirectX::XMStoreFloat4x4(&m_ViewMatrix, view);
}

void Camera::UpdateProjectionMatrix()
{
	DirectX::XMMATRIX projectionMatrix = 
		DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(m_FOV),
			m_AspectRatio,
			m_NearPlane,
			m_FarPlane);

	DirectX::XMStoreFloat4x4(&m_ProjectionMatrix, projectionMatrix);
}

void Camera::ValidateClipPlanes()
{
	if (m_FarPlane <= m_NearPlane)
		m_FarPlane = m_NearPlane + 0.01f;
}
