#include "Game/Camera3D.hpp"
#include "Engine/Math/MathUtils.hpp"

Camera3D::Camera3D()
	: m_position(Vector3::ZERO)
	, m_yaw(0.f)
	, m_pitch(0.f)
	, m_roll(0.f)
	, m_speed(10.f)
{

}

Vector3 Camera3D::GetForwardXYZ() const
{
	return Vector3(SinDegrees(m_yaw) * CosDegrees(-m_pitch), SinDegrees(-m_pitch), CosDegrees(m_yaw) * CosDegrees(-m_pitch));
}

Vector3 Camera3D::GetForwardXZ() const
{
	return Vector3(SinDegrees(m_yaw), 0.f, CosDegrees(m_yaw));
}

Vector3 Camera3D::GetLeftXZ() const
{
	Vector3 forward = GetForwardXZ();
	return Vector3(-forward.z, 0.f, forward.x);
}

Matrix4 Camera3D::GetViewMatrix() const
{
	Matrix4 outputMatrix;
	outputMatrix.MakeIdentity();

// 	outputMatrix.RotateDegreesAboutX(-90.f);
// 	outputMatrix.RotateDegreesAboutZ(90.f);

	outputMatrix.RotateDegreesAboutY(-m_yaw);
	outputMatrix.RotateDegreesAboutX(m_pitch);
	outputMatrix.RotateDegreesAboutZ(m_roll);
	outputMatrix.m_translation = Vector4(m_position.x, m_position.y, m_position.z, 1.f);

	return outputMatrix.GetInverse();
}

Matrix4 Camera3D::GetInverseViewMatrix() const
{
	Matrix4 outputMatrix;
	outputMatrix.MakeIdentity();

	// 	outputMatrix.RotateDegreesAboutX(-90.f);
	// 	outputMatrix.RotateDegreesAboutZ(90.f);

	outputMatrix.RotateDegreesAboutY(-m_yaw);
	outputMatrix.RotateDegreesAboutX(m_pitch);
	outputMatrix.RotateDegreesAboutZ(m_roll);
	outputMatrix.m_translation = Vector4(m_position.x, m_position.y, m_position.z, 1.f);

	return outputMatrix;
}

Matrix4 Camera3D::GetInverseViewMatrixXZ() const
{
	Matrix4 outputMatrix;
	outputMatrix.MakeIdentity();

	// 	outputMatrix.RotateDegreesAboutX(-90.f);
	// 	outputMatrix.RotateDegreesAboutZ(90.f);

	outputMatrix.RotateDegreesAboutY(-m_yaw);
	//outputMatrix.RotateDegreesAboutX(m_pitch);
	outputMatrix.RotateDegreesAboutZ(m_roll);
	outputMatrix.m_translation = Vector4(m_position.x, m_position.y, m_position.z, 1.f);

	return outputMatrix;
}
