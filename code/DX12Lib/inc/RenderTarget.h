#pragma once

#include <array>

enum class AttachmentPoint : uint8_t
{
	MinColorSlot = 0,
	Color0 = MinColorSlot,
	Color1,
	Color2,
	Color3,
	Color4,
	Color5,
	Color6,
	Color7,
	MaxColorSlot = Color7,

	DepthStencil,

	NumAttachmentPoints
};

class Texture;

class RenderTarget
{
public:
	using TextureArray = std::array<std::shared_ptr<Texture>,
		static_cast<size_t>(AttachmentPoint::NumAttachmentPoints)>;

	RenderTarget();

	/**
	* Disable copying to prevent shared GPU state.
	*/
	RenderTarget(const RenderTarget& other) = delete;
	RenderTarget& operator=(const RenderTarget& other) = delete;

	RenderTarget(RenderTarget&& other) = default;
	RenderTarget& operator=(RenderTarget&& other) = default;

	/**
	* Attach a texture to the render target.
	* The texture will be copied into the texture array.
	*/
	void AttachTexture(const AttachmentPoint attachmentPoint, std::shared_ptr<Texture> texture);
	
	std::shared_ptr<Texture> GetTexture(const AttachmentPoint attachmentPoint) const;

	/**
	* Get a list of all the textures attached to the render target.
	* This method is primarily used by the CommandList when binding the
	* render target to the output merger stage of the rendering pipeline.
	*/
	const TextureArray& GetTextures() const { return m_Textures; }

	/**
	* Resize all of the textures associated with the render target.
	*/
	void Resize(uint32_t width, uint32_t height);

	/**
	* Get the render target formats of the textures currently 
	* attached to this render target object.
	* This is needed to configure the Pipeline State Object.
	*/
	D3D12_RT_FORMAT_ARRAY GetRenderTargetFormats() const;

	/**
	* Get the format of the attached depth/stencil buffer.
	*/
	DXGI_FORMAT GetDepthStencilFormat() const;

	/**
	* Get the sample description of the render target.
	*/
	DXGI_SAMPLE_DESC GetSampleDesc() const;

private:
	TextureArray m_Textures;
};