/**
* A mesh class encapsulates the index and vertex buffers for a geometric primitive.
*/
#pragma once

#include <DirectXMath.h>

#include <wrl/client.h>

#include <memory>
#include <vector>

#include <VertexBuffer.h>
#include <IndexBuffer.h>

/*
* Vertex struct holding position, normal vector, and texture mapping information.
*/

struct VertexPositionNormalColorTexture
{
	VertexPositionNormalColorTexture()
		: m_Position(0.f, 0.f, 0.f)
		, m_Normal(0.f, 0.f, 1.f)
		, m_Color(1.f, 1.f, 1.f, 1.f)
		, m_TextureCoordinate(0.f, 0.f)
	{
	}

	VertexPositionNormalColorTexture(
		const DirectX::XMFLOAT3& position,
		const DirectX::XMFLOAT3& normal,
		const DirectX::XMFLOAT4& color,
		const DirectX::XMFLOAT2& textureCoordinate
	)
		: m_Position(position)
		, m_Normal(normal)
		, m_Color(color)
		, m_TextureCoordinate(textureCoordinate)
	{
	}

	VertexPositionNormalColorTexture(
		const DirectX::FXMVECTOR position,
		const DirectX::FXMVECTOR normal,
		const DirectX::FXMVECTOR color,
		const DirectX::FXMVECTOR textureCoordinate)
	{
		XMStoreFloat3(&m_Position, position);
		XMStoreFloat3(&m_Normal, normal);
		XMStoreFloat4(&m_Color, color);
		XMStoreFloat2(&m_TextureCoordinate, textureCoordinate);
	}

	DirectX::XMFLOAT3 m_Position;
	DirectX::XMFLOAT3 m_Normal;
	DirectX::XMFLOAT4 m_Color;
	DirectX::XMFLOAT2 m_TextureCoordinate;

	static constexpr int InputElementCount = 4;
	static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

using VertexCollection = std::vector<VertexPositionNormalColorTexture>;
using IndexCollection = std::vector<uint16_t>;

class CommandList;

class Mesh
{
public:
	void Draw(CommandList& commandList);

	void UpdateCubeColors(CommandList& commandList, const DirectX::XMFLOAT4 (&colors)[8]);

	static std::unique_ptr<Mesh> CreateCube(
		CommandList& commandList,
		const float size = 1,
		const bool useLHCoordinateSystem = true);

	static std::unique_ptr<Mesh> CreateSphere(
		CommandList& commandList,
		const float diameter = 1.f,
		const size_t tessellation = 16,
		const bool useLHCoordonateSystem = true);

	static std::unique_ptr<Mesh> CreateCone(
		CommandList& commandList,
		const float diameter = 1.f,
		const float height = 1.f,
		const size_t tessellation = 32,
		const bool useLHCoordonateSystem = true);

private:
	friend struct std::default_delete<Mesh>;

	Mesh();
	Mesh(const Mesh& copy);
	virtual ~Mesh();

	void Initialize(CommandList& commandList, 
					VertexCollection& vertices,
					IndexCollection& indices, 
					const bool useLHCoordinateSystem);

	void GenerateCubeVertices(VertexCollection& outVertices,
							  const DirectX::XMFLOAT4* customColors);

	VertexBuffer m_VertexBuffer;
	IndexBuffer m_IndexBuffer;

	UINT m_IndexCount;
	float m_Size;
	bool m_UseLHCoordinateSystem;
};
