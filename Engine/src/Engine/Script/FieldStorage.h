#pragma once

#include "Engine/Core/Base.h"
#include "Engine/Core/Ref.h"
#include "Engine/Core/Buffer.h"
#include "Engine/Core/UUID.h"
#include "GCManager.h"
#include "ScriptTypes.h"

namespace Hazel {

	enum class FieldFlag
	{
		None = -1,
		ReadOnly = BIT(0),
		Static = BIT(1),
		Public = BIT(2),
		Private = BIT(3),
		Protected = BIT(4),
		Internal = BIT(5),
		IsArray = BIT(6)
	};

	enum class FieldType
	{
		Void,
		Bool,
		Int8, Int16, Int32, Int64,
		UInt8, UInt16, UInt32, UInt64,
		Float, Double,
		String,
		Vector2, Vector3, Vector4,
		AssetHandle,
		Prefab,
		Entity,
		Mesh, StaticMesh,
		Material, PhysicsMaterial,
		Texture2D,
		Scene
	};

	struct FieldInfo
	{
		uint32_t ID;
		std::string Name;
		FieldType Type;
		uint32_t Size;
		bool IsProperty;
		Buffer DefaultValueBuffer;

		uint64_t Flags = 0;
		std::string DisplayName = "";


		bool HasFlag(FieldFlag flag) const { return Flags & (uint64_t)flag; }

		bool IsWritable() const
		{
			return !HasFlag(FieldFlag::ReadOnly) && HasFlag(FieldFlag::Public);
		}

		bool IsArray() const { return HasFlag(FieldFlag::IsArray); }
	};

	namespace FieldUtils {

		inline FieldType StringToFieldType(const std::string& typeString)
		{
			if (typeString == "Void") return FieldType::Void;
			if (typeString == "Bool") return FieldType::Bool;
			if (typeString == "Int8") return FieldType::Int8;
			if (typeString == "Int16") return FieldType::Int16;
			if (typeString == "Int32") return FieldType::Int32;
			if (typeString == "Int64") return FieldType::Int64;
			if (typeString == "UInt8") return FieldType::UInt8;
			if (typeString == "UInt16") return FieldType::UInt16;
			if (typeString == "UInt32") return FieldType::UInt32;
			if (typeString == "UInt64") return FieldType::UInt64;
			if (typeString == "Float") return FieldType::Float;
			if (typeString == "Double") return FieldType::Double;
			if (typeString == "String") return FieldType::String;
			if (typeString == "Vector2") return FieldType::Vector2;
			if (typeString == "Vector3") return FieldType::Vector3;
			if (typeString == "Vector4") return FieldType::Vector4;
			if (typeString == "AssetHandle") return FieldType::AssetHandle;
			if (typeString == "Prefab") return FieldType::Prefab;
			if (typeString == "Entity") return FieldType::Entity;
			if (typeString == "Mesh") return FieldType::Mesh;
			if (typeString == "StaticMesh") return FieldType::StaticMesh;
			if (typeString == "Material") return FieldType::Material;
			if (typeString == "PhysicsMaterial") return FieldType::PhysicsMaterial;
			if (typeString == "Texture2D") return FieldType::Texture2D;
			if (typeString == "Scene") return FieldType::Scene;

			ZONG_CORE_VERIFY(false);
			return FieldType::Void;
		}

		inline const char* FieldTypeToString(FieldType fieldType)
		{
			switch (fieldType)
			{
				case FieldType::Void: return "Void";
				case FieldType::Bool: return "Bool";
				case FieldType::Int8: return "Int8";
				case FieldType::Int16: return "Int16";
				case FieldType::Int32: return "Int32";
				case FieldType::Int64: return "Int64";
				case FieldType::UInt8: return "UInt8";
				case FieldType::UInt16: return "UInt16";
				case FieldType::UInt32: return "UInt32";
				case FieldType::UInt64: return "UInt64";
				case FieldType::Float: return "Float";
				case FieldType::Double: return "Double";
				case FieldType::String: return "String";
				case FieldType::Vector2: return "Vector2";
				case FieldType::Vector3: return "Vector3";
				case FieldType::Vector4: return "Vector4";
				case FieldType::AssetHandle: return "AssetHandle";
				case FieldType::Prefab: return "Prefab";
				case FieldType::Entity: return "Entity";
				case FieldType::Mesh: return "Mesh";
				case FieldType::StaticMesh: return "StaticMesh";
				case FieldType::Material: return "Material";
				case FieldType::PhysicsMaterial: return "PhysicsMaterial";
				case FieldType::Texture2D: return "Texture2D";
				case FieldType::Scene: return "Scene";
			}

			ZONG_CORE_VERIFY(false);
			return "Unknown";
		}

		inline uint32_t GetFieldTypeSize(FieldType type)
		{
			switch (type)
			{
				case FieldType::Bool: return sizeof(bool);
				case FieldType::Int8: return sizeof(int8_t);
				case FieldType::Int16: return sizeof(int16_t);
				case FieldType::Int32: return sizeof(int32_t);
				case FieldType::Int64: return sizeof(int64_t);
				case FieldType::UInt8: return sizeof(uint8_t);
				case FieldType::UInt16: return sizeof(uint16_t);
				case FieldType::UInt32: return sizeof(uint32_t);
				case FieldType::UInt64: return sizeof(uint64_t);
				case FieldType::Float: return sizeof(float);
				case FieldType::Double: return sizeof(double);
				case FieldType::String: return sizeof(char);
				case FieldType::Vector2: return sizeof(glm::vec2);
				case FieldType::Vector3: return sizeof(glm::vec3);
				case FieldType::Vector4: return sizeof(glm::vec4);
				case FieldType::AssetHandle:
				case FieldType::Entity:
				case FieldType::Prefab:
				case FieldType::Mesh:
				case FieldType::StaticMesh:
				case FieldType::Material:
				case FieldType::PhysicsMaterial:
				case FieldType::Scene:
				case FieldType::Texture2D:
					return sizeof(UUID);
			}

			ZONG_CORE_VERIFY(false);
			return 0;
		}
		
		inline bool IsPrimitiveType(FieldType type)
		{
			switch (type)
			{
				case FieldType::String: return false;
				case FieldType::Entity: return false;
				case FieldType::Prefab: return false;
				case FieldType::Mesh: return false;
				case FieldType::StaticMesh: return false;
				case FieldType::Material: return false;
				case FieldType::PhysicsMaterial: return false;
				case FieldType::Texture2D: return false;
				case FieldType::Scene: return false;
			}

			return true;
		}

		inline bool IsAsset(FieldType type)
		{
			switch (type)
			{
				case FieldType::Prefab:          return true;
				case FieldType::Mesh:            return true;
				case FieldType::StaticMesh:      return true;
				case FieldType::Material:        return true;
				case FieldType::PhysicsMaterial: return true;
				case FieldType::Texture2D:       return true;
				case FieldType::Scene:			 return true;
			}

			return false;
		}
	}

	class FieldStorageBase : public RefCounted
	{
	public:
		FieldStorageBase(FieldInfo* fieldInfo)
			: m_FieldInfo(fieldInfo) {}

		virtual void SetRuntimeInstance(GCHandle instance) = 0;
		virtual void CopyFrom(const Ref<FieldStorageBase>& other) = 0;

		virtual Buffer GetValueBuffer() const = 0;
		virtual void SetValueBuffer(const Buffer& buffer) = 0;

		const FieldInfo* GetFieldInfo() const { return m_FieldInfo; }

	protected:
		FieldInfo* m_FieldInfo = nullptr;
	};

	class FieldStorage : public FieldStorageBase
	{
	public:
		FieldStorage(FieldInfo* fieldInfo)
			: FieldStorageBase(fieldInfo)
		{
			m_DataBuffer = Buffer::Copy(fieldInfo->DefaultValueBuffer);
		}

		template<typename T>
		T GetValue() const
		{
			if (m_RuntimeInstance != nullptr)
			{
				Buffer valueBuffer;
				bool success = GetValueRuntime(valueBuffer);

				if (!success)
					return T();

				T value = T();
				memcpy(&value, valueBuffer.Data, valueBuffer.Size);
				valueBuffer.Release();
				return value;
			}

			if (!m_DataBuffer)
				return T();

			return *(m_DataBuffer.As<T>());
		}

		template<>
		std::string GetValue() const
		{
			if (m_RuntimeInstance != nullptr)
			{
				Buffer valueBuffer;
				bool success = GetValueRuntime(valueBuffer);

				if (!success)
					return std::string();

				std::string value((char*)valueBuffer.Data, valueBuffer.Size / sizeof(char));
				valueBuffer.Release();
				return value;
			}

			if (!m_DataBuffer)
				return std::string();

			return std::string((char*)m_DataBuffer.Data);
		}

		template<typename T>
		void SetValue(const T& value)
		{
			ZONG_CORE_VERIFY(sizeof(T) == m_FieldInfo->Size);

			if (m_RuntimeInstance != nullptr)
			{
				SetValueRuntime(&value);
			}
			else
			{
				if (!m_DataBuffer)
					m_DataBuffer.Allocate(m_FieldInfo->Size);
				m_DataBuffer.Write(&value, sizeof(T));
			}
		}

		template<>
		void SetValue<std::string>(const std::string& value)
		{
			if (m_RuntimeInstance != nullptr)
			{
				SetValueRuntime(&value);
			}
			else
			{
				if (m_DataBuffer.Size <= value.length() * sizeof(char))
				{
					m_DataBuffer.Release();
					m_DataBuffer.Allocate((value.length() * 2) * sizeof(char));
				}
				
				m_DataBuffer.ZeroInitialize();
				strcpy((char*)m_DataBuffer.Data, value.c_str());
			}
		}

		virtual void SetRuntimeInstance(GCHandle instance) override
		{
			m_RuntimeInstance = instance;

			if (m_RuntimeInstance)
			{
				if (m_FieldInfo->Type == FieldType::String)
				{
					std::string str((char*)m_DataBuffer.Data, m_DataBuffer.Size / sizeof(char));
					SetValueRuntime(&str);
				}
				else
				{
					SetValueRuntime(m_DataBuffer.Data);
				}
			}
		}

		virtual void CopyFrom(const Ref<FieldStorageBase>& other)
		{
			Ref<FieldStorage> fieldStorage = other.As<FieldStorage>();

			if (m_RuntimeInstance != nullptr)
			{
				Buffer valueBuffer;
				if (fieldStorage->GetValueRuntime(valueBuffer))
				{
					SetValueRuntime(valueBuffer.Data);
					valueBuffer.Release();
				}
			}
			else
			{
				m_DataBuffer.Release();
				m_DataBuffer = Buffer::Copy(fieldStorage->m_DataBuffer);
			}
		}

		virtual Buffer GetValueBuffer() const override
		{
			if (m_RuntimeInstance == nullptr)
				return m_DataBuffer;

			Buffer result;
			GetValueRuntime(result);
			return result;
		}

		virtual void SetValueBuffer(const Buffer& buffer)
		{
			if (m_RuntimeInstance != nullptr)
				SetValueRuntime(buffer.Data);
			else
				m_DataBuffer = Buffer::Copy(buffer);
		}

	private:
		bool GetValueRuntime(Buffer& outBuffer) const;
		void SetValueRuntime(const void* data);

	private:
		Buffer m_DataBuffer;
		GCHandle m_RuntimeInstance = nullptr;
	};

	class ArrayFieldStorage : public FieldStorageBase
	{
	public:
		ArrayFieldStorage(FieldInfo* fieldInfo)
			: FieldStorageBase(fieldInfo)
		{
			m_DataBuffer = Buffer::Copy(fieldInfo->DefaultValueBuffer);
			m_Length = (uint32_t)(m_DataBuffer.Size / m_FieldInfo->Size);
		}

		template<typename T>
		T GetValue(uint32_t index) const
		{
			if (m_RuntimeInstance != nullptr)
			{
				T value = T();
				GetValueRuntime(index, &value);
				return value;
			}

			if (!m_DataBuffer)
				return T();

			uint32_t offset = index * sizeof(T);
			return m_DataBuffer.Read<T>(offset);
		}

		template<>
		std::string GetValue(uint32_t index) const
		{
			return "";
		}

		template<typename T>
		void SetValue(uint32_t index, const T& value)
		{
			ZONG_CORE_VERIFY(sizeof(T) == m_FieldInfo->Size);

			if (m_RuntimeInstance != nullptr)
			{
				SetValueRuntime(index, &value);
			}
			else
			{
				m_DataBuffer.Write(&value, sizeof(T), index * sizeof(T));
			}
		}

		template<>
		void SetValue<std::string>(uint32_t index, const std::string& value)
		{
			if (m_RuntimeInstance != nullptr)
			{
				SetValueRuntime(index, &value);
			}
			else
			{
				auto& stringBuffer = (Buffer&)m_DataBuffer[index * sizeof(Buffer)];
				if (stringBuffer.Size != value.size())
				{
					stringBuffer.Release();
					stringBuffer.Allocate((value.length() * 2) * sizeof(char));
				}

				stringBuffer.ZeroInitialize();
				memcpy(stringBuffer.Data, value.c_str(), value.length() * sizeof(char));
			}
		}

		virtual void SetRuntimeInstance(GCHandle instance) override
		{
			m_RuntimeInstance = instance;

			if (m_FieldInfo->Type == FieldType::String)
				return;

			if (m_RuntimeInstance)
				SetRuntimeArray(m_DataBuffer);
		}

		virtual void CopyFrom(const Ref<FieldStorageBase>& other)
		{
			Ref<ArrayFieldStorage> fieldStorage = other.As<ArrayFieldStorage>();

			if (m_FieldInfo->Type == FieldType::String)
				return;

			if (m_RuntimeInstance != nullptr)
			{
				Buffer valueBuffer;
				if (fieldStorage->GetRuntimeArray(valueBuffer))
				{
					SetRuntimeArray(valueBuffer);
					valueBuffer.Release();
				}
			}
			else
			{
				m_DataBuffer.Release();
				m_DataBuffer = Buffer::Copy(fieldStorage->m_DataBuffer);
			}
		}

		void Resize(uint32_t newLength);
		void RemoveAt(uint32_t index);

		uint32_t GetLength() const { return m_RuntimeInstance != nullptr ? GetLengthRuntime() : m_Length; }

		virtual Buffer GetValueBuffer() const override
		{
			if (m_FieldInfo->Type == FieldType::String)
				return Buffer();

			if (m_RuntimeInstance == nullptr)
				return m_DataBuffer;

			Buffer result;
			GetRuntimeArray(result);
			return result;
		}

		virtual void SetValueBuffer(const Buffer& buffer)
		{
			if (m_FieldInfo->Type == FieldType::String)
				return;

			if (m_RuntimeInstance != nullptr)
			{
				SetRuntimeArray(buffer);
				m_Length = (uint32_t)(buffer.Size / m_FieldInfo->Size);
			}
			else
			{
				m_DataBuffer = Buffer::Copy(m_FieldInfo->DefaultValueBuffer);

				if (m_DataBuffer.Size < buffer.Size)
				{
					m_DataBuffer.Release();
					m_DataBuffer = Buffer::Copy(buffer);
				}
				else
				{
					m_DataBuffer.Write(buffer.Data, buffer.Size);
				}

				m_Length = (uint32_t)(m_DataBuffer.Size / m_FieldInfo->Size);
			}
		}

	private:
		bool GetRuntimeArray(Buffer& outData) const;
		void SetRuntimeArray(const Buffer& data);
		void GetValueRuntime(uint32_t index, void* data) const;
		void SetValueRuntime(uint32_t index, const void* data);
		uint32_t GetLengthRuntime() const;

	private:
		Buffer m_DataBuffer;
		uint32_t m_Length = 0;
		GCHandle m_RuntimeInstance = nullptr;
	};
}
