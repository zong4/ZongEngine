#pragma once

#include "Engine/Renderer/Material.h"

#include "Engine/Platform/Vulkan/VulkanShader.h"
#include "Engine/Platform/Vulkan/VulkanImage.h"
#include "Engine/Platform/Vulkan/DescriptorSetManager.h"

namespace Hazel {

	class VulkanMaterial : public Material
	{
	public:
		VulkanMaterial(const Ref<Shader>& shader, const std::string& name = "");
		VulkanMaterial(Ref<Material> material, const std::string& name = "");
		virtual ~VulkanMaterial() override;

		virtual void Invalidate() override;
		virtual void OnShaderReloaded() override;

		virtual void Set(const std::string& name, float value) override;
		virtual void Set(const std::string& name, int value) override;
		virtual void Set(const std::string& name, uint32_t value) override;
		virtual void Set(const std::string& name, bool value) override;
		virtual void Set(const std::string& name, const glm::ivec2& value) override;
		virtual void Set(const std::string& name, const glm::ivec3& value) override;
		virtual void Set(const std::string& name, const glm::ivec4& value) override;
		virtual void Set(const std::string& name, const glm::vec2& value) override;
		virtual void Set(const std::string& name, const glm::vec3& value) override;
		virtual void Set(const std::string& name, const glm::vec4& value) override;
		virtual void Set(const std::string& name, const glm::mat3& value) override;
		virtual void Set(const std::string& name, const glm::mat4& value) override;

		virtual void Set(const std::string& name, const Ref<Texture2D>& texture) override;
		virtual void Set(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex) override;
		virtual void Set(const std::string& name, const Ref<TextureCube>& texture) override;
		virtual void Set(const std::string& name, const Ref<Image2D>& image) override;
		virtual void Set(const std::string& name, const Ref<ImageView>& image) override;

		virtual float& GetFloat(const std::string& name) override;
		virtual int32_t& GetInt(const std::string& name) override;
		virtual uint32_t& GetUInt(const std::string& name) override;
		virtual bool& GetBool(const std::string& name) override;
		virtual glm::vec2& GetVector2(const std::string& name) override;
		virtual glm::vec3& GetVector3(const std::string& name) override;
		virtual glm::vec4& GetVector4(const std::string& name) override;
		virtual glm::mat3& GetMatrix3(const std::string& name) override;
		virtual glm::mat4& GetMatrix4(const std::string& name) override;

		virtual Ref<Texture2D> GetTexture2D(const std::string& name) override;
		virtual Ref<TextureCube> GetTextureCube(const std::string& name) override;

		virtual Ref<Texture2D> TryGetTexture2D(const std::string& name) override;
		virtual Ref<TextureCube> TryGetTextureCube(const std::string& name) override;

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			auto decl = FindUniformDeclaration(name);
			ZONG_CORE_ASSERT(decl, "Could not find uniform!");
			if (!decl)
				return;

			auto& buffer = m_UniformStorageBuffer;
			buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());
		}

		template<typename T>
		T& Get(const std::string& name)
		{
			auto decl = FindUniformDeclaration(name);
			ZONG_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = m_UniformStorageBuffer;
			return buffer.Read<T>(decl->GetOffset());
		}

		template<typename T>
		Ref<T> GetResource(const std::string& name)
		{
			return m_DescriptorSetManager.GetInput<T>(name);
		}

		template<typename T>
		Ref<T> TryGetResource(const std::string& name)
		{
			return m_DescriptorSetManager.GetInput<T>(name);
		}

		virtual uint32_t GetFlags() const override { return m_MaterialFlags; }
		virtual void SetFlags(uint32_t flags) override { m_MaterialFlags = flags; }
		virtual bool GetFlag(MaterialFlag flag) const override { return (uint32_t)flag & m_MaterialFlags; }
		virtual void SetFlag(MaterialFlag flag, bool value = true) override
		{
			if (value)
			{
				m_MaterialFlags |= (uint32_t)flag;
			}
			else
			{
				m_MaterialFlags &= ~(uint32_t)flag;
			}
		}

		virtual Ref<Shader> GetShader() override { return m_Shader; }
		virtual const std::string& GetName() const override { return m_Name; }

		Buffer GetUniformStorageBuffer() { return m_UniformStorageBuffer; }

		VkDescriptorSet GetDescriptorSet(uint32_t index)
		{
			if (m_DescriptorSetManager.GetFirstSetIndex() == UINT32_MAX)
				return nullptr;

			Prepare();
			return m_DescriptorSetManager.GetDescriptorSets(index)[0];
		}
		
		void Prepare();
	private:
		void Init();
		void AllocateStorage();

		void SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture);
		void SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex);
		void SetVulkanDescriptor(const std::string& name, const Ref<TextureCube>& texture);
		void SetVulkanDescriptor(const std::string& name, const Ref<Image2D>& image);
		void SetVulkanDescriptor(const std::string& name, const Ref<ImageView>& image);

		const ShaderUniform* FindUniformDeclaration(const std::string& name);
		const ShaderResourceDeclaration* FindResourceDeclaration(const std::string& name);
	private:
		Ref<VulkanShader> m_Shader;
		std::string m_Name;

		enum class PendingDescriptorType
		{
			None = 0, Texture2D, TextureCube, Image2D
		};

		// -- NEW v --
		// Per frame in flight
		DescriptorSetManager m_DescriptorSetManager;
		std::vector<VkDescriptorSet> m_MaterialDescriptorSets;

		// Map key is binding, vector index is array index (size 1 for non-array)
		std::map<uint32_t, std::vector<Ref<RendererResource>>> m_MaterialDescriptorImages;
		std::map<uint32_t, VkWriteDescriptorSet> m_MaterialWriteDescriptors;
		// -- NEW ^ --

		uint32_t m_MaterialFlags = 0;

		Buffer m_UniformStorageBuffer;

	};

}
