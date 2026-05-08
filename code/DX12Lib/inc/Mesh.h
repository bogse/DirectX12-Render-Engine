/**
* A mesh class encapsulates the index and vertex buffers for a geometric primitive.
*/
#pragma once

#include <DirectXMath.h>


#include <wrl.h>

#include <memory>
#include <vector>

#include <VertexBuffer.h>
#include <IndexBuffer.h>

/*
* Vertex struct holding position, normal vector, and texture mapping information.
*/

struct VertexPositionNormalTexture
{
	VertexPositionNormalTexture()
		: m_Position(0.f, 0.f, 0.f)
		, m_Normal(0.f, 0.f, 1.f)
		, m_TextureCoordinate(0.f, 0.f)
	{
	}

	VertexPositionNormalTexture(
		const DirectX::XMFLOAT3& position,
		const DirectX::XMFLOAT3& normal,
		const DirectX::XMFLOAT2& textureCoordinate
	)
		: m_Position(position)
		, m_Normal(normal)
		, m_TextureCoordinate(textureCoordinate)
	{
	}

	VertexPositionNormalTexture(
		DirectX::FXMVECTOR position,
		DirectX::FXMVECTOR normal,
		DirectX::FXMVECTOR textureCoordinate)
	{
		XMStoreFloat3(&m_Position, position);
		XMStoreFloat3(&m_Normal, normal);
		XMStoreFloat2(&m_TextureCoordinate, textureCoordinate);
	}

	DirectX::XMFLOAT3 m_Position;
	DirectX::XMFLOAT3 m_Normal;
	DirectX::XMFLOAT2 m_TextureCoordinate;

	static constexpr int InputElementCount = 3;
	static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

using VertexCollection = std::vector<VertexPositionNormalTexture>;
using IndexCollection = std::vector<uint16_t>;

class CommandList;

class Mesh
{
public:
	void Draw(CommandList& commandList);

	static std::unique_ptr<Mesh> CreateCube(
		CommandList& commandList,
		const float size = 1,
		const bool useLHCoordinateSystem = true);

private:
	friend struct std::default_delete<Mesh>;

	Mesh();
	Mesh(const Mesh& copy);
	virtual ~Mesh();

	void Initialize(CommandList& commandList, 
					VertexCollection& vertices,
					IndexCollection& indices, 
					const bool useLHCoordinateSystem);

	VertexBuffer m_VertexBuffer;
	IndexBuffer m_IndexBuffer;

	UINT m_IndexCount;
};
