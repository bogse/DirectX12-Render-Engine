#pragma once

#include <DirectXMath.h>

class Camera
{
public:
	Camera();

	void Update();

	DirectX::XMMATRIX GetViewMatrix() const
	{
		return DirectX::XMLoadFloat4x4(&m_ViewMatrix);
	};

	DirectX::XMMATRIX GetProjectionMatrix()
	{
		return DirectX::XMLoadFloat4x4(&m_ProjectionMatrix);
	}

	const DirectX::XMFLOAT3& GetPosition() const { return m_Position; };
	const DirectX::XMFLOAT4& GetRotation() const { return m_Rotation; };

	float GetFOV() const { return m_FOV; }
	float GetAspectRatio() const { return m_AspectRatio; }
	float GetNearPlane() const { return m_NearPlane; }
	float GetFarPlane() const { return m_FarPlane; }

	void SetProjection(const float fov, const float aspectRatio,
		const float nearZ, const float farZ);

	void SetPosition(const DirectX::FXMVECTOR position);
	void SetRotation(const DirectX::XMVECTOR rotation);

	void SetFOV(const float fov);
	void SetAspectRatio(const float aspectRatio);
	void SetNearPlane(const float nearPlane);
	void SetFarPlane(const float setFarPlane);

private:
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();
	void ValidateClipPlanes();

	DirectX::XMFLOAT4X4 m_ViewMatrix;
	DirectX::XMFLOAT4X4 m_ProjectionMatrix;

	DirectX::XMFLOAT3 m_Position;
	DirectX::XMFLOAT4 m_Rotation;

	float m_FOV;
	float m_AspectRatio;
	float m_NearPlane;
	float m_FarPlane;

	bool m_IsProjectionDirty;
	bool m_IsViewDirty;
};