#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Memory.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <typeinfo>

namespace Hazel::Utils {

	class ValueWrapper
	{
	public:
		ValueWrapper() = default;
		ValueWrapper(bool value) : ValueWrapper(&value, sizeof(bool)) {}
		ValueWrapper(int8_t value) : ValueWrapper(&value, sizeof(int8_t)) {}
		ValueWrapper(int16_t value) : ValueWrapper(&value, sizeof(int16_t)) {}
		ValueWrapper(int32_t value) : ValueWrapper(&value, sizeof(int32_t)) {}
		ValueWrapper(int64_t value) : ValueWrapper(&value, sizeof(int64_t)) {}
		ValueWrapper(uint8_t value) : ValueWrapper(&value, sizeof(uint8_t)) {}
		ValueWrapper(uint16_t value) : ValueWrapper(&value, sizeof(uint16_t)) {}
		ValueWrapper(uint32_t value) : ValueWrapper(&value, sizeof(uint32_t)) {}
		ValueWrapper(uint64_t value) : ValueWrapper(&value, sizeof(uint64_t)) {}
		ValueWrapper(float value) : ValueWrapper(&value, sizeof(float)) {}
		ValueWrapper(double value) : ValueWrapper(&value, sizeof(double)) {}
		ValueWrapper(const std::string& value) : ValueWrapper(static_cast<const void*>(value.c_str()), value.size()) {}
		ValueWrapper(const glm::vec2& value) : ValueWrapper(static_cast<const void*>(glm::value_ptr<float>(value)), sizeof(float) * 2) {}
		ValueWrapper(const glm::vec3& value) : ValueWrapper(static_cast<const void*>(glm::value_ptr<float>(value)), sizeof(float) * 3) {}
		ValueWrapper(const glm::vec4& value) : ValueWrapper(static_cast<const void*>(glm::value_ptr<float>(value)), sizeof(float) * 4) {}

		ValueWrapper(const void* data, size_t size)
			: m_Size(size)
		{
			m_ValueBuffer = hnew byte[size];
			memcpy(m_ValueBuffer, data, size);
		}

		ValueWrapper(const ValueWrapper& other)
		{
			m_Size = other.m_Size;
			if (m_Size)
			{
				m_ValueBuffer = hnew byte[m_Size];
				memcpy(m_ValueBuffer, other.m_ValueBuffer, m_Size);
			}
		}

		ValueWrapper& operator=(const ValueWrapper& other)
		{
			if (m_ValueBuffer != other.m_ValueBuffer)
			{
				ReleaseBuffer();

				m_Size = other.m_Size;
				if (m_Size)
				{
					m_ValueBuffer = hnew byte[m_Size];
					memcpy(m_ValueBuffer, other.m_ValueBuffer, m_Size);
				}
				else
				{
					m_ValueBuffer = nullptr;
				}
			}

			return *this;
		}

		~ValueWrapper()
		{
			m_Size = 0;
			hdelete[] m_ValueBuffer;
			m_ValueBuffer = nullptr;
		}

		void ReleaseBuffer()
		{
			m_Size = 0;
			hdelete[] m_ValueBuffer;
			m_ValueBuffer = nullptr;
		}

		bool HasValue() const { return m_Size > 0 && m_ValueBuffer != nullptr; }

		template<typename TValueType>
		TValueType Get() const
		{
			HZ_CORE_VERIFY(HasValue(), "ValueWrapper::Get - No value!");

			if constexpr (std::is_same<TValueType, std::string>::value)
				return TValueType((char*)m_ValueBuffer, m_Size);
			else
				return *(TValueType*)m_ValueBuffer;
		}

		template<typename TValueType>
		TValueType GetOrDefault(const TValueType& defaultValue) const
		{
			if (!HasValue())
				return defaultValue;

			if constexpr (std::is_same<TValueType, std::string>::value)
				return TValueType((char*)m_ValueBuffer, m_Size);
			else
				return *(TValueType*)m_ValueBuffer;
		}

		void* GetRawData() const { return m_ValueBuffer; }
		size_t GetDataSize() const { return m_Size; }

		template<typename TValueType>
		void Set(TValueType value)
		{
			HZ_CORE_VERIFY(HasValue(), "Trying to set the value of an empty ValueWrapper!");

			if constexpr (std::is_same<TValueType, std::string>::value)
			{
				if (value.size() > m_Size)
				{
					hdelete[] m_ValueBuffer;
					m_ValueBuffer = hnew byte[value.size()];
					m_Size = value.size();
				}

				memcpy(m_ValueBuffer, value.data(), value.size());
			}
			else
			{
				HZ_CORE_VERIFY(m_Size == sizeof(value));
				memcpy(m_ValueBuffer, &value, sizeof(value));
			}
		}

		static ValueWrapper OfSize(size_t size)
		{
			ValueWrapper value;
			value.m_ValueBuffer = hnew byte[size];
			value.m_Size = size;
			return value;
		}

		static ValueWrapper Empty() { return ValueWrapper(); }

	private:
		size_t m_Size = 0;
		byte* m_ValueBuffer = nullptr;
	};

}
