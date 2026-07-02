#pragma once

#include <DirectXMath.h>

struct DirectionalLight
{
	DirectionalLight()
		: DirectionWS(0.f, 0.f, 1.f, 0.f)
		, DirectionVS(0.f, 0.f, 1.f, 0.f)
		, Color(1.f, 1.f, 1.f, 1.f)
	{}

	DirectX::XMFLOAT4 DirectionWS;
	DirectX::XMFLOAT4 DirectionVS;
	DirectX::XMFLOAT4 Color;
};

struct PointLight
{
	PointLight()
		: PositionWS(0.f, 0.f, 0.f, 1.f)
		, PositionVS(0.f, 0.f, 0.f, 1.f)
		, Color(1.f, 1.f, 1.f, 1.f)
		, ConstantAttenuation(1.f)
		, LinearAttenuation(0.f)
		, QuadraticAttenuation(0.f)
		, Padding(0.f)
	{}

	DirectX::XMFLOAT4 PositionWS;
	DirectX::XMFLOAT4 PositionVS;
	DirectX::XMFLOAT4 Color;
	float ConstantAttenuation;
	float LinearAttenuation;
	float QuadraticAttenuation;
	float Padding;
};

struct LightProperties
{
	uint32_t NumDirectionalLights;
	uint32_t NumPointLights;
};
