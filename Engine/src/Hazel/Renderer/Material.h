#pragma once

#include "Hazel/Core/Base.h"

#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/Texture.h"

#include <unordered_set>

namespace Hazel {

	enum class MaterialFlag
	{
		None       = BIT(0),
		DepthTest  = BIT(1),
		Blend      = BIT(2),
		TwoSided   = BIT(3),
		DisableShadowCasting = BIT(4)
	};

	class Material : public RefCounted
	{
	public:
		static Ref<Material> Create(const Ref<Shader>& shader, const std::string& name = "");
		static Ref<Material> Copy(const Ref<Material>& other, const std::string& name = "");
		virtual ~Material() {}

		virtual void Invalidate() = 0;
		virtual void OnShaderReloaded() = 0;

		virtual void Set(const std::string& name, float value) = 0;
		virtual void Set(const std::string& name, int value) = 0;
		virtual void Set(const std::string& name, uint32_t value) = 0;
		virtual void Set(const std::string& name, bool value) = 0;
		virtual void Set(const std::string& name, const glm::vec2& value) = 0;
		virtual void Set(const std::string& name, const glm::vec3& value) = 0;
		virtual void Set(const std::string& name, const glm::vec4& value) = 0;
		virtual void Set(const std::string& name, const glm::ivec2& value) = 0;
		virtual void Set(const std::string& name, const glm::ivec3& value) = 0;
		virtual void Set(const std::string& name, const glm::ivec4& value) = 0;

		virtual void Set(const std::string& name, const glm::mat3& value) = 0;
		virtual void Set(const std::string& name, const glm::mat4& value) = 0;

		virtual void Set(const std::string& name, const Ref<Texture2D>& texture) = 0;
		virtual void Set(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex) = 0;
		virtual void Set(const std::string& name, const Ref<TextureCube>& texture) = 0;
		virtual void Set(const std::string& name, const Ref<Image2D>& image) = 0;
		virtual void Set(const std::string& name, const Ref<ImageView>& image) = 0;

		virtual float& GetFloat(const std::string& name) = 0;
		virtual int32_t& GetInt(const std::string& name) = 0;
		virtual uint32_t& GetUInt(const std::string& name) = 0;
		virtual bool& GetBool(const std::string& name) = 0;
		virtual glm::vec2& GetVector2(const std::string& name) = 0;
		virtual glm::vec3& GetVector3(const std::string& name) = 0;
		virtual glm::vec4& GetVector4(const std::string& name) = 0;
		virtual glm::mat3& GetMatrix3(const std::string& name) = 0;
		virtual glm::mat4& GetMatrix4(const std::string& name) = 0;

		virtual Ref<Texture2D> GetTexture2D(const std::string& name) = 0;
		virtual Ref<TextureCube> GetTextureCube(const std::string& name) = 0;

		virtual Ref<Texture2D> TryGetTexture2D(const std::string& name) = 0;
		virtual Ref<TextureCube> TryGetTextureCube(const std::string& name) = 0;

#if 0
		template<typename T>
		T& Get(const std::string& name)
		{
			auto decl = m_Material->FindUniformDeclaration(name);
			HZ_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = m_UniformStorageBuffer;
			return buffer.Read<T>(decl->GetOffset());
		}

		template<typename T>
		Ref<T> GetResource(const std::string& name)
		{
			auto decl = m_Material->FindResourceDeclaration(name);
			HZ_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			uint32_t slot = decl->GetRegister();
			HZ_CORE_ASSERT(slot < m_Textures.size(), "Texture slot is invalid!");
			return Ref<T>(m_Textures[slot]);
		}

		template<typename T>
		Ref<T> TryGetResource(const std::string& name)
		{
			auto decl = m_Material->FindResourceDeclaration(name);
			if (!decl)
				return nullptr;

			uint32_t slot = decl->GetRegister();
			if (slot >= m_Textures.size())
				return nullptr;

			return Ref<T>(m_Textures[slot]);
		}
#endif

		virtual uint32_t GetFlags() const = 0;
		virtual void SetFlags(uint32_t flags) = 0;

		virtual bool GetFlag(MaterialFlag flag) const = 0;
		virtual void SetFlag(MaterialFlag flag, bool value = true) = 0;

		virtual Ref<Shader> GetShader() = 0;
		virtual const std::string& GetName() const = 0;
	};

}
