#include "DX12LibPCH.h"

#include "RenderTarget.h"

#include "Texture.h"

RenderTarget::RenderTarget()
{
}

void RenderTarget::AttachTexture(
	const AttachmentPoint attachmentPoint,
	std::shared_ptr<Texture> texture)
{
	m_Textures[static_cast<size_t>(attachmentPoint)] = texture;
}

std::shared_ptr<Texture> RenderTarget::GetTexture(const AttachmentPoint attachmentPoint) const
{
	return m_Textures[static_cast<size_t>(attachmentPoint)];
}

void RenderTarget::Resize(uint32_t width, uint32_t height)
{
	for (const std::shared_ptr<Texture>& texture : m_Textures)
	{
		assert(texture != nullptr && 
			"RenderTarget contains an uninitilized or null texture during resize.");

		texture->Resize(width, height);
	}
}

D3D12_RT_FORMAT_ARRAY RenderTarget::GetRenderTargetFormats() const
{
	D3D12_RT_FORMAT_ARRAY rtvFormats = {};

	constexpr int minColorSlot = static_cast<int>(AttachmentPoint::MinColorSlot);
	constexpr int maxColorSlot = static_cast<int>(AttachmentPoint::MaxColorSlot);
	for (int i = minColorSlot; i <= maxColorSlot; ++i)
	{
		const std::shared_ptr<Texture>& texture = m_Textures[i];
		if (texture)
			rtvFormats.RTFormats[rtvFormats.NumRenderTargets++] = texture->GetD3D12ResourceDesc().Format;
	}

	for (UINT i = rtvFormats.NumRenderTargets; i < 8; ++i)
	{
		rtvFormats.RTFormats[i] = DXGI_FORMAT_UNKNOWN;
	}

	return rtvFormats;
}

DXGI_FORMAT RenderTarget::GetDepthStencilFormat() const
{
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
	const std::shared_ptr<Texture>& depthStencilTexture = m_Textures[static_cast<size_t>(AttachmentPoint::DepthStencil)];
	if (depthStencilTexture)
		dsvFormat = depthStencilTexture->GetD3D12ResourceDesc().Format;

	return dsvFormat;
}

DXGI_SAMPLE_DESC RenderTarget::GetSampleDesc() const
{
	DXGI_SAMPLE_DESC sampleDesc = { 1u, 0u };
	constexpr int minColorSlot = static_cast<int>(AttachmentPoint::MinColorSlot);
	constexpr int maxColorSlot = static_cast<int>(AttachmentPoint::MaxColorSlot);
	for (int i = minColorSlot; i <= maxColorSlot; ++i)
	{
		const std::shared_ptr<Texture>& texture = m_Textures[i];
		if (texture)
		{
			sampleDesc = texture->GetD3D12ResourceDesc().SampleDesc;
			break;
		}
	}

	return sampleDesc;
}
