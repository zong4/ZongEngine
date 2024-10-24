#pragma once

#include <choc/containers/choc_Value.h>

namespace Engine::Utils {

	//==============================================================================
	/** A set of named properties. The value of each item is a Value object. */
	struct PropertySet
	{
		PropertySet();
		~PropertySet();
		PropertySet(const PropertySet&);
		PropertySet(PropertySet&&);
		PropertySet& operator= (const PropertySet&);
		PropertySet& operator= (PropertySet&&);

		struct Property
		{
			std::string name;
			choc::value::Value value;
		};

		bool IsEmpty() const;
		size_t Size() const;

		// TODO: JP. maybe we should return ValueView, or have a version that returns ValueView?
		choc::value::Value GetValue(std::string_view name) const;
		choc::value::Value GetValue(std::string_view name, const choc::value::Value& defaultReturnValue) const;
		bool HasValue(std::string_view name) const;

		bool GetBool(std::string_view name, bool defaultValue = false) const;
		float GetFloat(std::string_view name, float defaultValue = 0.0f) const;
		double GetDouble(std::string_view name, double defaultValue = 0.0) const;
		int32_t GetInt(std::string_view name, int32_t defaultValue = 0) const;
		int64_t GetInt64(std::string_view name, int64_t defaultValue = 0) const;
		std::string GetString(std::string_view name, std::string_view defaultValue = {}) const;

		void Set(std::string_view name, int32_t value);
		void Set(std::string_view name, int64_t value);
		void Set(std::string_view name, float value);
		void Set(std::string_view name, double value);
		void Set(std::string_view name, bool value);
		void Set(std::string_view name, const char* value);
		void Set(std::string_view name, std::string_view value);
		void Set(std::string_view name, choc::value::Value newValue);
		void Set(std::string_view name, const choc::value::ValueView& value);

		void Remove(std::string_view name);

		std::vector<std::string> GetNames() const;

		choc::value::Value ToExternalValue() const;
		static PropertySet FromExternalValue(const choc::value::ValueView& deserialized);

		// Sort property set by name (case insensitive)
		void Sort();

#if 0
		//? DEPRECATED
		std::string ToJSON() const;
		//? DEPRECATED
		static PropertySet FromJSON(const choc::value::ValueView& parsedJSON);
#endif

	private:
		std::unique_ptr<std::vector<Property>> properties;

		void Set(std::string_view name, const void* value) = delete;
		void SetInternal(std::string_view name, choc::value::Value newValue);
	};

} // namespace Engine::Utils
