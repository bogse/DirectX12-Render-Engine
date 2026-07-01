#include "DX12LibPCH.h"

#include "Mesh.h"

#include "Application.h"
#include "CommandList.h"

namespace
{
	inline DirectX::XMVECTOR GetCircleVector(const size_t i, const size_t tessellation)
	{
		const float angle = i * DirectX::XM_2PI / static_cast<float>(tessellation);
		float dx;
		float dz;

		DirectX::XMScalarSinCos(&dx, &dz, angle);

		return DirectX::XMVectorSet(dx, 0, dz, 0);
	}

	inline DirectX::XMVECTOR GetCircleTangent(const size_t i, const size_t tessellation)
	{
		const float angle = (static_cast<float>(i) * DirectX::XM_2PI / static_cast<float>(tessellation)) + DirectX::XM_PIDIV2;
		float dx;
		float dz;

		DirectX::XMScalarSinCos(&dx, &dz, angle);

		return DirectX::XMVectorSet(dx, 0.f, dz, 0.f);
	}

	// Helper creates a triangle fan to close the end of a cylinder / cone.
	void CreateCylinderCap(
		VertexCollection& vertices,
		IndexCollection& indices,
		const size_t tessellation,
		const float height,
		const float radius,
		const bool isTop)
	{
		const size_t vbase = vertices.size();

		// Create cap indices matching a clean triangle fan pattern anchored at center (vbase).
		for (size_t i = 0; i < tessellation - 2; ++i)
		{
			size_t i1 = (i + 1) % tessellation;
			size_t i2 = (i + 2) % tessellation;

			if (isTop)
			{
				std::swap(i1, i2);
			}

			indices.push_back(static_cast<uint16_t>(vbase));
			indices.push_back(static_cast<uint16_t>(vbase + i1));
			indices.push_back(static_cast<uint16_t>(vbase + i2));
		}

		// Which end of the cylinder is this?
		DirectX::XMVECTOR normal = DirectX::g_XMIdentityR1;
		DirectX::XMVECTOR textureScale = DirectX::g_XMNegativeOneHalf;

		if (!isTop)
		{
			normal = DirectX::XMVectorNegate(normal);
			textureScale = DirectX::XMVectorMultiply(textureScale, DirectX::g_XMNegateX);
		}

		// Create cap vertices.
		for (size_t i = 0; i < tessellation; ++i)
		{
			DirectX::XMVECTOR circleVector = GetCircleVector(i, tessellation);

			DirectX::XMVECTOR position = DirectX::XMVectorAdd(
				DirectX::XMVectorScale(circleVector, radius),
				DirectX::XMVectorScale(normal, height));

			DirectX::XMVECTOR color = DirectX::g_XMOne;

			// Swizzle X and Z into X and Y for planar cap mapping coordinates
			DirectX::XMVECTOR textureCoordinate = DirectX::XMVectorMultiplyAdd(
				DirectX::XMVectorSwizzle<0, 2, 3, 3>(circleVector), textureScale, DirectX::g_XMOneHalf);

			vertices.push_back(VertexPositionNormalColorTexture(position, normal, color, textureCoordinate));
		}
	}
}

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

void Mesh::UpdateCubeColors(CommandList& commandList, const DirectX::XMFLOAT4 (&colors)[8])
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

std::unique_ptr<Mesh> Mesh::CreateSphere(CommandList& commandList,
	const float diameter,
	const size_t tessellation,
	const bool useLHCoordonateSystem)
{
	VertexCollection vertices;
	IndexCollection indices;

	if (tessellation < 3)
		throw std::out_of_range("Tesselation parameter out of range.");

	const float radius = diameter / 2.f;
	const size_t verticalSegments = tessellation;
	const size_t horizontalSegments = tessellation * 2;

	// Create rings of vertices at progressively higher latitudes.
	for (size_t i = 0; i <= verticalSegments; ++i)
	{
		const float v = 1.f - static_cast<float>(i) / verticalSegments;

		const float latitude = (i * DirectX::XM_PI / verticalSegments) - DirectX::XM_PIDIV2;
		float dy;
		float dxz;

		DirectX::XMScalarSinCos(&dy, &dxz, latitude);

		// Create a single ring of vertices at this latitude.
		for (size_t j = 0; j <= horizontalSegments; ++j)
		{
			const float u = static_cast<float>(j) / horizontalSegments;

			const float longitude = j * DirectX::XM_2PI / horizontalSegments;
			float dx;
			float dz;

			DirectX::XMScalarSinCos(&dx, &dz, longitude);

			dx *= dxz;
			dz *= dxz;

			DirectX::XMVECTOR normal = DirectX::XMVectorSet(dx, dy, dz, 0);
			DirectX::XMVECTOR position = DirectX::XMVectorScale(normal, radius);
			DirectX::XMVECTOR color = DirectX::XMVectorSet(1.f, 1.f, 1.f, 1.f);
			DirectX::XMVECTOR textureCoordonate = DirectX::XMVectorSet(u, v, 0, 0);

			vertices.push_back(VertexPositionNormalColorTexture(position, normal, color, textureCoordonate));
		}
	}

	// Fill the index buffer with triangles joining each pair of latitude rings.
	const size_t stride = horizontalSegments + 1;

	for (size_t i = 0; i < verticalSegments; ++i)
	{
		for (size_t j = 0; j < horizontalSegments; ++j)
		{
			const size_t nextI = i + 1;
			const size_t nextJ = j + 1;

			// Define indices for the 4 corners of the quad patch.
			uint16_t bottomLeft	 = static_cast<uint16_t>(i * stride + j);
			uint16_t topLeft	 = static_cast<uint16_t>(nextI * stride + j);
			uint16_t bottomRight = static_cast<uint16_t>(i * stride + nextJ);
			uint16_t topRight	 = static_cast<uint16_t>(nextI * stride + nextJ);

			indices.push_back(bottomLeft);
			indices.push_back(topLeft);
			indices.push_back(bottomRight);

			indices.push_back(bottomRight);
			indices.push_back(topLeft);
			indices.push_back(topRight);
		}
	}

	// Create the primitive object.
	std::unique_ptr<Mesh> mesh(new Mesh());

	mesh->Initialize(commandList, vertices, indices, useLHCoordonateSystem);

	return mesh;
}

std::unique_ptr<Mesh> Mesh::CreateCone(
	CommandList& commandList,
	const float diameter,
	const float height,
	const size_t tessellation,
	const bool useLHCoordonateSystem)
{
	VertexCollection vertices;
	IndexCollection indices;

	if (tessellation < 3)
		throw std::out_of_range("Tesselation parameter out of range.");

	const float halfHeight = height / 2;

	DirectX::XMVECTOR topOffset = DirectX::XMVectorScale(DirectX::g_XMIdentityR1, halfHeight);

	const float radius = diameter / 2.f;
	const size_t stride = tessellation + 1;

	// Create a ring of triangles around the outside of the cone.
	for (size_t i = 0; i <= tessellation; ++i)
	{
		DirectX::XMVECTOR circleVector = GetCircleVector(i, tessellation);

		DirectX::XMVECTOR sideOffset = DirectX::XMVectorScale(circleVector, radius);

		DirectX::XMVECTOR pt = DirectX::XMVectorSubtract(sideOffset, topOffset);

		DirectX::XMVECTOR normal = DirectX::XMVector3Cross(
			GetCircleTangent(i, tessellation),
			DirectX::XMVectorSubtract(topOffset, pt));
		normal = DirectX::XMVector3Normalize(normal);

		DirectX::XMVECTOR color = DirectX::g_XMOne;

		float u = static_cast<float>(i) / tessellation;
		DirectX::XMVECTOR textureCoordinate = DirectX::XMVectorSet(u, 0.f, 0.f, 0.f);

		// Duplicate the top vertex for distinct normals
		vertices.push_back(VertexPositionNormalColorTexture(topOffset, normal, color, DirectX::g_XMZero));
		vertices.push_back(VertexPositionNormalColorTexture(pt, normal, color, textureCoordinate + DirectX::g_XMIdentityR1));
	}

	for (size_t i = 0; i < tessellation; ++i)
	{
		uint16_t currentTop	   = static_cast<uint16_t>(i * 2);
		uint16_t currentBottom = static_cast<uint16_t>(i * 2 + 1);
		uint16_t nextBottom    = static_cast<uint16_t>((i + 1) * 2 + 1);

		indices.push_back(currentTop);
		indices.push_back(nextBottom);
		indices.push_back(currentBottom);
	}

	// Create flat triangle fan caps to seal the bottom.
	CreateCylinderCap(vertices, indices, tessellation, halfHeight, radius, false);

	// Create the primitive object.
	std::unique_ptr<Mesh> mesh(new Mesh());

	mesh->Initialize(commandList, vertices, indices, useLHCoordonateSystem);

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
