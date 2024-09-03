#pragma once

#include <glm/glm.hpp>
#include <map>

#include "Hazel/Renderer/RendererTypes.h"
#include "Image.h"

namespace Hazel {

	class Framebuffer;

	enum class FramebufferBlendMode
	{
		None = 0,
		OneZero,
		SrcAlphaOneMinusSrcAlpha,
		Additive,
		Zero_SrcColor
	};

	enum class AttachmentLoadOp
	{
		Inherit = 0, Clear = 1, Load = 2
	};

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(ImageFormat format) : Format(format) {}

		ImageFormat Format;
		bool Blend = true;
		FramebufferBlendMode BlendMode = FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha;
		AttachmentLoadOp LoadOp = AttachmentLoadOp::Inherit;
		// TODO: filtering/wrap
	};

	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(const std::initializer_list<FramebufferTextureSpecification>& attachments)
			: Attachments(attachments) {}

		std::vector<FramebufferTextureSpecification> Attachments;
	};

	struct FramebufferSpecification
	{
		float Scale = 1.0f;
		uint32_t Width = 0;
		uint32_t Height = 0;
		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		float DepthClearValue = 0.0f;
		bool ClearColorOnLoad = true;
		bool ClearDepthOnLoad = true;

		FramebufferAttachmentSpecification Attachments;
		uint32_t Samples = 1; // multisampling

		// TODO: Temp, needs scale
		bool NoResize = false;

		// Master switch (individual attachments can be disabled in FramebufferTextureSpecification)
		bool Blend = true;
		// None means use BlendMode in FramebufferTextureSpecification
		FramebufferBlendMode BlendMode = FramebufferBlendMode::None;

		// SwapChainTarget = screen buffer (i.e. no framebuffer)
		bool SwapChainTarget = false;

		// Will it be used for transfer ops?
		bool Transfer = false;

		// Note: these are used to attach multi-layered color/depth images 
		Ref<Image2D> ExistingImage;
		std::vector<uint32_t> ExistingImageLayers;
		
		// Specify existing images to attach instead of creating
		// new images. attachment index -> image
		std::map<uint32_t, Ref<Image2D>> ExistingImages;

		// At the moment this will just create a new render pass
		// with an existing framebuffer
		Ref<Framebuffer> ExistingFramebuffer;

		std::string DebugName;
	};

	class Framebuffer : public RefCounted
	{
	public:
		virtual ~Framebuffer() {}
		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) = 0;
		virtual void AddResizeCallback(const std::function<void(Ref<Framebuffer>)>& func) = 0;

		virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual RendererID GetRendererID() const = 0;

		virtual Ref<Image2D> GetImage(uint32_t attachmentIndex = 0) const = 0;
		virtual size_t GetColorAttachmentCount() const = 0;
		virtual bool HasDepthAttachment() const = 0;
		virtual Ref<Image2D> GetDepthImage() const = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};

}
