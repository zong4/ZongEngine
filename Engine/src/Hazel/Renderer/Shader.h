#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Buffer.h"

#include "Hazel/Renderer/RendererTypes.h"
#include "Hazel/Renderer/ShaderUniform.h"

#include <filesystem>
#include <string>
#include <glm/glm.hpp>

#define HZ_HAS_SHADER_COMPILER !HZ_DIST

namespace Hazel
{
	namespace ShaderUtils {
		enum class SourceLang
		{
			NONE, GLSL, HLSL,
		};
	}

	enum class ShaderUniformType
	{
		None = 0, Bool, Int, UInt, Float, Vec2, Vec3, Vec4, Mat3, Mat4,
		IVec2, IVec3, IVec4
	};

	class ShaderUniform
	{
	public:
		ShaderUniform() = default;
		ShaderUniform(std::string name, ShaderUniformType type, uint32_t size, uint32_t offset);

		const std::string& GetName() const { return m_Name; }
		ShaderUniformType GetType() const { return m_Type; }
		uint32_t GetSize() const { return m_Size; }
		uint32_t GetOffset() const { return m_Offset; }

		static constexpr std::string_view UniformTypeToString(ShaderUniformType type);

		static void Serialize(StreamWriter* serializer, const ShaderUniform& instance)
		{
			serializer->WriteString(instance.m_Name);
			serializer->WriteRaw(instance.m_Type);
			serializer->WriteRaw(instance.m_Size);
			serializer->WriteRaw(instance.m_Offset);
		}

		static void Deserialize(StreamReader* deserializer, ShaderUniform& instance)
		{
			deserializer->ReadString(instance.m_Name);
			deserializer->ReadRaw(instance.m_Type);
			deserializer->ReadRaw(instance.m_Size);
			deserializer->ReadRaw(instance.m_Offset);
		}
	private:
		std::string m_Name;
		ShaderUniformType m_Type = ShaderUniformType::None;
		uint32_t m_Size = 0;
		uint32_t m_Offset = 0;
	};

	struct ShaderUniformBuffer
	{
		std::string Name;
		uint32_t Index;
		uint32_t BindingPoint;
		uint32_t Size;
		uint32_t RendererID;
		std::vector<ShaderUniform> Uniforms;
	};

	struct ShaderStorageBuffer
	{
		std::string Name;
		uint32_t Index;
		uint32_t BindingPoint;
		uint32_t Size;
		uint32_t RendererID;
		//std::vector<ShaderUniform> Uniforms;
	};

	struct ShaderBuffer
	{
		std::string Name;
		uint32_t Size = 0;
		std::unordered_map<std::string, ShaderUniform> Uniforms;

		static void Serialize(StreamWriter* serializer, const ShaderBuffer& instance)
		{
			serializer->WriteString(instance.Name);
			serializer->WriteRaw(instance.Size);
			serializer->WriteMap(instance.Uniforms);
		}

		static void Deserialize(StreamReader* deserializer, ShaderBuffer& instance)
		{
			deserializer->ReadString(instance.Name);
			deserializer->ReadRaw(instance.Size);
			deserializer->ReadMap(instance.Uniforms);
		}
	};

	class Shader : public RefCounted
	{
	public:
		using ShaderReloadedCallback = std::function<void()>;

		virtual void Reload(bool forceCompile = false) = 0;
		virtual void RT_Reload(bool forceCompile) = 0;

		virtual size_t GetHash() const = 0;
		
		virtual const std::string& GetName() const = 0;

		virtual void SetMacro(const std::string& name, const std::string& value) = 0;

		static Ref<Shader> Create(const std::string& filepath, bool forceCompile = false, bool disableOptimization = false);
		static Ref<Shader> LoadFromShaderPack(const std::string& filepath, bool forceCompile = false, bool disableOptimization = false);
		static Ref<Shader> CreateFromString(const std::string& source);

		virtual const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const = 0;
		virtual const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const = 0;

		virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) = 0;

		static constexpr const char* GetShaderDirectoryPath()
		{
			return "Resources/Shaders/";
		}
	};

	class ShaderPack;

	// This should be eventually handled by the Asset Manager
	class ShaderLibrary : public RefCounted
	{
	public:
		ShaderLibrary();
		~ShaderLibrary();

		void Add(const Ref<Shader>& shader);
		void Load(std::string_view path, bool forceCompile = false, bool disableOptimization = false);
		void Load(std::string_view name, const std::string& path);
		void LoadShaderPack(const std::filesystem::path& path);

		const Ref<Shader>& Get(const std::string& name) const;
		size_t GetSize() const { return m_Shaders.size(); }

		std::unordered_map<std::string, Ref<Shader>>& GetShaders() { return m_Shaders; }
		const std::unordered_map<std::string, Ref<Shader>>& GetShaders() const { return m_Shaders; }
	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
		Ref<ShaderPack> m_ShaderPack;
	};

}
