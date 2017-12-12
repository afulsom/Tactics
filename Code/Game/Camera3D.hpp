#pragma once
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Matrix4.hpp"


struct CameraState
{
	CameraState(Vector3 position, float yaw, float pitch, float roll)
		: position(position)
		, yaw(yaw)
		, pitch(pitch)
		, roll(roll)
	{}

	Vector3 position;
	float yaw;
	float pitch;
	float roll;
};


class Camera3D
{
public:
	Vector3 m_position;
	float m_yaw;
	float m_pitch;
	float m_roll;
	
	float m_speed;

	Camera3D();

 	Vector3 GetForwardXYZ() const;
	Vector3 GetForwardXZ() const;
// 	Vector3 GetLeftXYZ() const;
	Vector3 GetLeftXZ() const;
// 	Vector3 GetUpXYZ() const;

	Matrix4 GetViewMatrix() const;
	Matrix4 GetInverseViewMatrix() const;
	Matrix4 GetInverseViewMatrixXZ() const;
};