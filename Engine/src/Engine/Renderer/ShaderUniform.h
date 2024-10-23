#pragma once

#include "Engine/Core/Base.h"
#include "Engine/Core/Log.h"

#include "Engine/Serialization/StreamReader.h"
#include "Engine/Serialization/StreamWriter.h"

#include <string>
#include <vector>

namespace Hazel {

	enum class ShaderDomain
	{
		None = 0, Vertex = 0, Pixel = 1 // unused
	};

	class ShaderResourceDeclaration
	{
	public:
		ShaderResourceDeclaration() = default;
		ShaderResourceDeclaration(const std::string& name, uint32_t set, uint32_t resourceRegister, uint32_t count)
			: m_Name(name), m_Set(set), m_Register(resourceRegister), m_Count(count) { }

		virtual const std::string& GetName() const { return m_Name; }
		virtual uint32_t GetSet() const { return m_Set; }
		virtual uint32_t GetRegister() const { return m_Register; }
		virtual uint32_t GetCount() const { return m_Count; }

		static void Serialize(StreamWriter* serializer, const ShaderResourceDeclaration& instance)
		{
			serializer->WriteString(instance.m_Name);
			serializer->WriteRaw(instance.m_Set);
			serializer->WriteRaw(instance.m_Register);
			serializer->WriteRaw(instance.m_Count);
		}

		static void Deserialize(StreamReader* deserializer, ShaderResourceDeclaration& instance)
		{
			deserializer->ReadString(instance.m_Name);
			deserializer->ReadRaw(instance.m_Set);
			deserializer->ReadRaw(instance.m_Register);
			deserializer->ReadRaw(instance.m_Count);
		}
	private:
		std::string m_Name;
		uint32_t m_Set = 0;
		uint32_t m_Register = 0;
		uint32_t m_Count = 0;
	};

	typedef std::vector<ShaderResourceDeclaration*> ShaderResourceList;

}
