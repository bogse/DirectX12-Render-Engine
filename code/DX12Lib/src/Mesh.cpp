#include "DX12LibPCH.h"

#include "Mesh.h"

#include "Application.h"
#include "CommandList.h"

const D3D12_INPUT_ELEMENT_DESC VertexPositionNormalColorTexture::InputElements[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR",	  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

Mesh::Mesh()
	: m_IndexCount(0)
	, m_VertexBuffer{}
	, m_IndexBuffer{}
	, m_Size(0.f)
	, m_UseLHCoordinateSystem(true)
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
	const UINT strides[] = { sizeof(VertexPositionNormalColorTexture) };
	const UINT offsets[] = { 0 };

	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.SetVertexBuffer(0, m_VertexBuffer);
	commandList.SetIndexBuffer(m_IndexBuffer);
	commandList.DrawIndexed(m_IndexCount, 1, 0, 0, 0);
}

void Mesh::UpdateColors(CommandList& commandList, const DirectX::XMFLOAT4 (&colors)[8])
{
	VertexCollection tempVertices;
	tempVertices.reserve(24);

	GenerateCubeVertices(tempVertices, colors);
	commandList.CopyVertexBuffer(m_VertexBuffer, tempVertices);
}

std::unique_ptr<Mesh> Mesh::CreateCube(
	CommandList& commandList,
	const float size,
	const bool useLHCoordinateSystem)
{
	VertexCollection vertices;
	IndexCollection indices;

	constexpr int faceCount = 6;
	constexpr size_t verticesPerFace = 4;

	for (int i = 0; i < faceCount; ++i)
	{
		size_t vbase = i * verticesPerFace;

		// Six indices (two triangles) per face.
		indices.push_back(static_cast<uint16_t>(vbase + 0)); // Bottom-Left
		indices.push_back(static_cast<uint16_t>(vbase + 1)); // Top-Left
		indices.push_back(static_cast<uint16_t>(vbase + 2)); // Top-Right

		indices.push_back(static_cast<uint16_t>(vbase + 0)); // Bottom-Left
		indices.push_back(static_cast<uint16_t>(vbase + 2)); // Top-Right
		indices.push_back(static_cast<uint16_t>(vbase + 3)); // Bottom-Right
	}

	// Create the primitive object.
	std::unique_ptr<Mesh> mesh(new Mesh());

	mesh->m_Size = size;
	mesh->GenerateCubeVertices(vertices, nullptr);
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
	m_UseLHCoordinateSystem = useLHCoordinateSystem;
}

void Mesh::GenerateCubeVertices(VertexCollection& outVertices, const DirectX::XMFLOAT4* customColors)
{
	constexpr int faceCount = 6;
	constexpr int verticesPerFace = 4;

	// Normal vectors for the 6 faces of a cube
	constexpr DirectX::XMVECTORF32 faceNormals[faceCount] = {
		{  0.f,  0.f,  1.f }, // Front
		{  0.f,  0.f, -1.f }, // Back
		{  1.f,  0.f,  0.f }, // Right
		{ -1.f,  0.f,  0.f }, // Left
		{  0.f,  1.f,  0.f }, // Top
		{  0.f, -1.f,  0.f }  // Bottom
	};

	constexpr DirectX::XMFLOAT2 textureCoordinates[verticesPerFace] = {
		{ 1.f, 0.f },
		{ 1.f, 1.f },
		{ 0.f, 1.f },
		{ 0.f, 0.f }
	};

	const DirectX::XMVECTOR defaultColorVec = DirectX::XMVectorSet(1.f, 1.f, 1.f, 1.f);
	const float sideLength = m_Size / 2.f;

	for (int i = 0; i < faceCount; ++i)
	{
		const DirectX::XMVECTOR normal = faceNormals[i];

		// Get two vectors perpendicular both to the face normal and to each other.
		const DirectX::XMVECTOR basis =
			(i >= 4) ? DirectX::g_XMIdentityR2 : DirectX::g_XMIdentityR1;

		const DirectX::XMVECTOR side1 = DirectX::XMVector3Cross(normal, basis);
		const DirectX::XMVECTOR side2 = DirectX::XMVector3Cross(normal, side1);

		// Calculate the 4 corner positions
		DirectX::XMVECTOR pos[verticesPerFace];

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

		for (int j = 0; j < verticesPerFace; ++j)
		{
			DirectX::XMVECTOR vertexColor = defaultColorVec;

			if (customColors != nullptr)
			{
				DirectX::XMFLOAT3 posStructure;
				DirectX::XMStoreFloat3(&posStructure, pos[j]);

				// Convert the 3D signs into a 3-bit index (0 to 7)
				size_t bit0 = (posStructure.x > 0.f) ? 1 : 0; // Bit 0: X axis
				size_t bit1 = (posStructure.y > 0.f) ? 2 : 0; // Bit 1: Y axis
				size_t bit2 = (posStructure.z > 0.f) ? 4 : 0; // Bit 2: Z axis

				size_t targetCornerIndex = bit0 | bit1 | bit2;

				vertexColor = DirectX::XMLoadFloat4(&customColors[targetCornerIndex]);
			}

			outVertices.push_back(VertexPositionNormalColorTexture(
				pos[j],
				normal,
				vertexColor,
				DirectX::XMLoadFloat2(&textureCoordinates[j])));
		}
	}

	IndexCollection dummyIndices;

	if (m_UseLHCoordinateSystem)
		ReverseWinding(dummyIndices, outVertices);
}
