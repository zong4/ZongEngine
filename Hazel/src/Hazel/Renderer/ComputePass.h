#pragma once

#include "Hazel/Core/Base.h"

#include "Framebuffer.h"

#include "UniformBufferSet.h"
#include "StorageBufferSet.h"
#include "Texture.h"
#include "PipelineCompute.h"

namespace Hazel {

	struct ComputePassSpecification
	{
		Ref<PipelineCompute> Pipeline;
		std::string DebugName;
		glm::vec4 MarkerColor;
	};

	class ComputePass : public RefCounted
	{
	public:
		virtual ~ComputePass() = default;

		virtual ComputePassSpecification& GetSpecification() = 0;
		virtual const ComputePassSpecification& GetSpecification() const = 0;

		virtual Ref<Shader> GetShader() const = 0;

		virtual void SetInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet) = 0;
		virtual void SetInput(std::string_view name, Ref<UniformBuffer> uniformBuffer) = 0;

		virtual void SetInput(std::string_view name, Ref<StorageBufferSet> storageBufferSet) = 0;
		virtual void SetInput(std::string_view name, Ref<StorageBuffer> storageBuffer) = 0;

		virtual void SetInput(std::string_view name, Ref<Texture2D> texture) = 0;
		virtual void SetInput(std::string_view name, Ref<TextureCube> textureCube) = 0;
		virtual void SetInput(std::string_view name, Ref<Image2D> image) = 0;

		virtual Ref<Image2D> GetOutput(uint32_t index) = 0;
		virtual Ref<Image2D> GetDepthOutput() = 0;
		virtual bool HasDescriptorSets() const = 0;
		virtual uint32_t GetFirstSetIndex() const = 0;

		virtual bool Validate() = 0;
		virtual void Bake() = 0;
		virtual bool Baked() const = 0;
		virtual void Prepare() = 0;

		virtual Ref<PipelineCompute> GetPipeline() const = 0;

		static Ref<ComputePass> Create(const ComputePassSpecification& spec);
	};

}
