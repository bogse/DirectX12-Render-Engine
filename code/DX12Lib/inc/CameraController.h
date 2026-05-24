#pragma once

#include "Camera.h"

namespace KeyCode
{
	enum class Key : unsigned int;
}

class CameraController
{
public:
	CameraController(Camera& camera);

	void Update(float deltaTime);

	void OnKeyPressed(const KeyCode::Key key);
	void OnKeyReleased(const KeyCode::Key key);
	void OnMouseMoved(const int deltaX, const int deltaY);
	void OnMouseButtonPressed(const bool isRightKeyPressed);
	void OnMouseButtonReleased(const bool isRightKeyPressed);
	void OnMouseWheel(const float wheelDelta);

	float GetYaw() const { return m_Yaw; }
	float GetPitch() const { return m_Pitch; }
	float GetMoveSpeed() const { return m_MoveSpeed; }
	float GetMouseSensitivity() const { return m_MouseSensitivity; }

	void SetYaw(const float yaw) { m_Yaw = yaw; }
	void SetPitch(const float pitch) { m_Pitch = pitch; }
	void SetMoveSpeed(const float moveSpeed) { m_MoveSpeed = moveSpeed; }
	void SetMouseSensitivity(const float mouseSensitivity) { m_MouseSensitivity = mouseSensitivity; }

private:
	void HandleInput();

	Camera& m_Camera;

	float m_Yaw;
	float m_Pitch;
	float m_PreviousYaw;
	float m_PreviousPitch;

	float m_MoveSpeed;
	float m_MouseSensitivity;
	float m_MouseWheelDelta;

	bool m_MoveForward;
	bool m_MoveBackward;
	bool m_MoveLeft;
	bool m_MoveRight;
	bool m_MoveUp;
	bool m_MoveDown;

	bool m_RightMouseDown;
};