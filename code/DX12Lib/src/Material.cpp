#include "DX12LibPCH.h"

#include "Material.h"

Material MaterialPresets::CreateSatinWood()
{
	Material material;
	material.Emissive = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	material.Ambient = DirectX::XMFLOAT4(0.05f, 0.05f, 0.05f, 1.0f);
	material.Diffuse = DirectX::XMFLOAT4(0.45f, 0.28f, 0.12f, 1.0f); // Warm brown base
	material.Specular = DirectX::XMFLOAT4(0.10f, 0.10f, 0.10f, 1.0f); // Very weak, colorless reflection
	material.SpecularPower = 8.0f;
	return material;
}