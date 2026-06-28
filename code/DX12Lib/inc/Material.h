#pragma once

#include <DirectXMath.h>

struct Material
{
	Material()
		: Emissive(0.f, 0.f, 0.f, 1.f)
		, Ambient(0.1f, 0.1f, 0.1f, 1.f)
		, Diffuse(1.f, 1.f, 1.f, 1.f)
		, Specular(0.2f, 0.2f, 0.2f, 1.f)
		, SpecularPower(32.f)
		, Padding(0.f, 0.f, 0.f)
	{}

	DirectX::XMFLOAT4 Emissive;
	DirectX::XMFLOAT4 Ambient;
	DirectX::XMFLOAT4 Diffuse;
	DirectX::XMFLOAT4 Specular;
	float			  SpecularPower;
	uint32_t		  Padding[3];
};

class MaterialPresets
{
public:
	static Material CreateSatinWood();
};