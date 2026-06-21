#include "DX12LibPCH.h"

#include "CameraController.h"
#include "Input.h"
#include "KeyCodes.h"

#include <algorithm>

namespace
{
	constexpr float BaseMoveSpeed = 4.f;
	constexpr float FastMoveSpeed = 16.f;
}

CameraController::CameraController(Camera& camera)
	: m_Camera(camera)
	, m_Yaw(0.f)
	, m_Pitch(0.f)
	, m_PreviousYaw(0.f)
	, m_PreviousPitch(0.f)
	, m_MoveSpeed(BaseMoveSpeed)
	, m_MouseSensitivity(0.1f)
	, m_MouseWheelDelta(0.f)
	, m_MoveForward(false)
	, m_MoveBackward(false)
	, m_MoveLeft(false)
	, m_MoveRight(false)
	, m_MoveUp(false)
	, m_MoveDown(false)
	, m_RightMouseDown(false)
{
}

void CameraController::Update(float deltaTime)
{
	HandleInput();

	const DirectX::XMVECTOR rotation =
		DirectX::XMQuaternionRotationRollPitchYaw(
			DirectX::XMConvertToRadians(m_Pitch),
			DirectX::XMConvertToRadians(m_Yaw),
			0.f);

	if (m_Yaw != m_PreviousYaw || m_Pitch != m_PreviousPitch)
	{
		m_Camera.SetRotation(rotation);
		m_PreviousYaw = m_Yaw;
		m_PreviousPitch = m_Pitch;
	}

	DirectX::XMVECTOR forward = DirectX::XMVector3Rotate(
		DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
		rotation);

	DirectX::XMVECTOR right = DirectX::XMVector3Rotate(
		DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
		rotation);

	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMVECTOR movement = DirectX::XMVectorZero();

	if (m_MoveForward)
		movement = DirectX::XMVectorAdd(movement, forward);

	if (m_MoveBackward)
		movement = DirectX::XMVectorSubtract(movement, forward);

	if (m_MoveRight)
		movement = DirectX::XMVectorAdd(movement, right);

	if (m_MoveLeft)
		movement = DirectX::XMVectorSubtract(movement, right);

	if (m_MoveUp)
		movement = DirectX::XMVectorAdd(movement, up);

	if (m_MoveDown)
		movement = DirectX::XMVectorSubtract(movement, up);

	if (!DirectX::XMVector3Equal(movement, DirectX::XMVectorZero()))
	{
		// Avoid faster diagonal movement
		movement = DirectX::XMVector3Normalize(movement);

		DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&m_Camera.GetPosition());

		const DirectX::XMVECTOR delta = DirectX::XMVectorScale(
			movement,
			m_MoveSpeed * deltaTime);

		position = DirectX::XMVectorAdd(position, delta);

		m_Camera.SetPosition(position);
	}

	if (m_MouseWheelDelta != 0.f)
	{
		float fov = m_Camera.GetFOV();
		fov -= m_MouseWheelDelta;
		fov = std::clamp(fov, 12.f, 90.f);
		m_Camera.SetFOV(fov);

		m_MouseWheelDelta = 0.f;
	}
}

void CameraController::OnMouseMoved(const int deltaX, const int deltaY)
{
	if (!m_RightMouseDown)
		return;

	m_Yaw += deltaX * m_MouseSensitivity;
	m_Pitch -= deltaY * m_MouseSensitivity;

	m_Pitch = std::clamp(m_Pitch, -89.f, 89.f);
}

void CameraController::OnMouseWheel(const float wheelDelta)
{
	m_MouseWheelDelta += wheelDelta;
}

void CameraController::HandleInput()
{
	m_MoveForward = Input::IsKeyPressed(KeyCode::Key::W);
	m_MoveBackward = Input::IsKeyPressed(KeyCode::Key::S);
	m_MoveLeft = Input::IsKeyPressed(KeyCode::Key::A);
	m_MoveRight = Input::IsKeyPressed(KeyCode::Key::D);
	m_MoveUp = Input::IsKeyPressed(KeyCode::Key::E);
	m_MoveDown = Input::IsKeyPressed(KeyCode::Key::Q);
	m_RightMouseDown = Input::IsKeyPressed(KeyCode::Key::RButton);
	m_MoveSpeed = Input::IsKeyPressed(KeyCode::Key::ShiftKey) ? FastMoveSpeed : BaseMoveSpeed;
}
