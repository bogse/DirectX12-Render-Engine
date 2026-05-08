#include "DX12LibPCH.h"

#include "Mesh.h"

#include "Application.h"
#include "CommandList.h"

const D3D12_INPUT_ELEMENT_DESC VertexPositionNormalTexture::InputElements[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

Mesh::Mesh()
	: m_IndexCount(0)
	, m_VertexBuffer{}
	, m_IndexBuffer{}
{
	m_VertexBuffer.SetName(L"Demo::VertexBuffer");
	m_IndexBuffer.SetName(L"Demo::IndexBuffer");
}

Mesh::~Mesh()
{
	// Allocated resources will be cleaned automatically when the pointers go out of scope.
}

void Mesh::Draw(CommandList& commandList)
{
	const UINT strides[] = { sizeof(VertexPositionNormalTexture) };
	const UINT offsets[] = { 0 };

	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.SetVertexBuffer(0, m_VertexBuffer);
	commandList.SetIndexBuffer(m_IndexBuffer);
	commandList.DrawIndexed(m_IndexCount, 1, 0, 0, 0);
}

std::unique_ptr<Mesh> Mesh::CreateCube(
	CommandList& commandList,
	const float size,
	const bool useLHCoordinateSystem)
{
	constexpr int faceCount = 6;

	// Normal vectors for the 6 faces of a cube
	constexpr DirectX::XMVECTORF32 faceNormals[faceCount] = {
		{  0,  0,  1 }, // Front
		{  0,  0, -1 }, // Back
		{  1,  0,  0 }, // Right
		{ -1,  0,  0 }, // Left
		{  0,  1,  0 }, // Top
		{  0, -1,  0 }  // Bottom
	};

	constexpr DirectX::XMFLOAT2 textureCoordinates[4] = {
		{ 1, 0 },
		{ 1, 1 },
		{ 0, 1 },
		{ 0, 0 }
	};

	VertexCollection vertices;
	IndexCollection indices;

	const float sideLength = size / 2.0f;

	for (int i = 0; i < faceCount; ++i)
	{
		const DirectX::XMVECTOR normal = faceNormals[i];

		// Get two vectors perpendicular both to the face normal and to each other.
		const DirectX::XMVECTOR basis = 
			(i >= 4) ? DirectX::g_XMIdentityR2 : DirectX::g_XMIdentityR1;

		const DirectX::XMVECTOR side1 = DirectX::XMVector3Cross(normal, basis);
		const DirectX::XMVECTOR side2 = DirectX::XMVector3Cross(normal, side1);

		size_t vbase = vertices.size();

		// Six indices (two triangles) per face.
		indices.push_back(static_cast<uint16_t>(vbase + 0)); // Bottom-Left
		indices.push_back(static_cast<uint16_t>(vbase + 1)); // Top-Left
		indices.push_back(static_cast<uint16_t>(vbase + 2)); // Top-Right

		indices.push_back(static_cast<uint16_t>(vbase + 0));
		indices.push_back(static_cast<uint16_t>(vbase + 2));
		indices.push_back(static_cast<uint16_t>(vbase + 3)); // Bottom-Right

		// Calculate the 4 corner positions
		constexpr int numCorners = 4;
		DirectX::XMVECTOR pos[numCorners];

		pos[0] = DirectX::XMVectorScale(
			DirectX::XMVectorSubtract(
				DirectX::XMVectorSubtract(normal, side1),
				side2),
			sideLength);

		pos[1] = DirectX::XMVectorScale(
			DirectX::XMVectorAdd(
				DirectX::XMVectorSubtract(normal, side1),
				side2), 
			sideLength);

		pos[2] = DirectX::XMVectorScale(
			DirectX::XMVectorAdd(
				DirectX::XMVectorAdd(normal, side1),
				side2), 
			sideLength);

		pos[3] = DirectX::XMVectorScale(
			DirectX::XMVectorSubtract(
				DirectX::XMVectorAdd(normal, side1),
				side2), 
			sideLength);

		for (int j = 0; j < numCorners; ++j)
		{
			vertices.push_back(VertexPositionNormalTexture(
				pos[j], 
				normal, 
				DirectX::XMLoadFloat2(&textureCoordinates[j]
			)));
		}
	}

	// Create the primitive object.
	std::unique_ptr<Mesh> mesh(new Mesh());

	mesh->Initialize(commandList, vertices, indices, useLHCoordinateSystem);

	return mesh;
}

// Helper for flipping winding of geometric primitives for LH vs RH coordinate system.
static void ReverseWinding(IndexCollection& indices, VertexCollection& vertices)
{
	assert((indices.size() % 3) == 0);

	for (IndexCollection::iterator it = indices.begin(); it != indices.end(); it += 3)
	{
		std::swap(*it, *(it + 2));
	}

	for (VertexCollection::iterator it = vertices.begin(); it != vertices.end(); ++it)
	{
		it->m_TextureCoordinate.x = (1.f - it->m_TextureCoordinate.x);
	}
}

void Mesh::Initialize(
	CommandList& commandList,
	VertexCollection& vertices,
	IndexCollection& indices,
	const bool useLHCoordinateSystem)
{
	if (vertices.size() >= USHRT_MAX)
		throw std::exception("Too many vertices for 16-bit index buffer.");

	if (useLHCoordinateSystem)
		ReverseWinding(indices, vertices);

	commandList.CopyVertexBuffer(m_VertexBuffer, vertices);
	commandList.CopyIndexBuffer(m_IndexBuffer, indices);

	m_IndexCount = static_cast<UINT>(indices.size());
}