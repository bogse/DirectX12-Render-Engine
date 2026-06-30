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

struct LightProperties
{
	uint32_t NumDirectionalLights;
};