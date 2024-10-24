#include "pch.h"
#include "SoundGraphPatchPreset.h"

#include "Engine/Core/Hash.h"

namespace Engine
{
	// TODO: move this into some sort of SoundGraph utility header
	namespace Utils
	{
		static inline bool IsWaveAsset(const choc::value::Type& type) { return type.isInt64(); }
		static inline bool IsWaveAsset(const choc::value::ValueView& value) { return value.isInt64(); }

		//? NOT USED
#if 0
		static inline void CopyArrayData(const choc::value::ValueView& destCustomArrayObject, const choc::value::ValueView& sourceArrayData)
		{
			choc::value::ValueView destination = destCustomArrayObject["data"];
			const size_t currentDataSize = destination.getType().getValueDataSize();
			const size_t newDataSize = sourceArrayData.getType().getValueDataSize();

			ZONG_CORE_ASSERT(destination.size() >= sourceArrayData.size());
			ZONG_CORE_ASSERT(currentDataSize >= newDataSize);

			std::memcpy(destination.getRawData(), sourceArrayData.getRawData(), newDataSize);
		}
#endif
	}

	const SoundGraphPatchPreset::Parameter* SoundGraphPatchPreset::begin() const { return m_Parameters.begin(); }
	const SoundGraphPatchPreset::Parameter* SoundGraphPatchPreset::end() const { return m_Parameters.end(); }
	size_t SoundGraphPatchPreset::size() const { return m_Parameters.size(); }
	SoundGraphPatchPreset::Parameter* Engine::SoundGraphPatchPreset::begin() { return m_Parameters.begin(); }
	SoundGraphPatchPreset::Parameter* Engine::SoundGraphPatchPreset::end() { return m_Parameters.end(); }

	SoundGraphPatchPreset::SoundGraphPatchPreset(const Utils::PropertySet& propertySet, const std::vector<uint32_t>& handles)
	{
		InitializeFromPropertySet(propertySet, handles);
	}

	void SoundGraphPatchPreset::InitializeFromPropertySet(const Utils::PropertySet& propertySet, const std::vector<uint32_t>& handles)
	{
		m_Parameters.clear();
		m_ChangedRangeEnd = nullptr;

		const auto& names = propertySet.GetNames();
		if (handles.size() != names.size())
		{
			ZONG_CORE_ASSERT(false);
			return;
		}

		for (uint32_t i = 0; i < names.size(); ++i)
		{
			const std::string& name = names[i];
			AddParameter({ handles[i], name, propertySet.GetValue(name) });
		}

		TranslateParameters();
	}

	//! might want to use this later at some point
	void SoundGraphPatchPreset::TranslateParameters()
	{
#if 0 //? Old SOUL stuff
		for (auto& parameter : m_Parameters)
		{
			choc::value::ValueView value = parameter.Value;
			choc::value::Value arrayTypeValue;

			if (value.isObjectWithClassName("AssetHandle"))
			{
				//! At this point wave asset is in form of uint64_t converted to string
				parameter.Value = Utils::CreateWaveValue(value);
			}
			else if (value.isArray())
			{
				//auto annotation = endpoint.annotation.get();

				// 'size' annotation should've been set from this same GraphInput parameter
				//const int32_t arrayMaxSize = annotation["size"].getWithDefault<int32_t>(0);
				//ZONG_CORE_ASSERT(arrayMaxSize == value.size());

				parameter.Value = Utils::CreateArrayParameterValue(value);
			}
		}
#endif
	}

	bool SoundGraphPatchPreset::SetParameter(std::string_view name, const choc::value::ValueView& value)
	{
		const bool isArray = value.isArray();

		// For now we don't support sending array of values to SoundGraph patch
		if (isArray && Utils::IsWaveAsset(value[0]) || value.isObject())
		{
			ZONG_CORE_ASSERT(false, "Dynamically changing SoundGraph Wave asset arrays is not implemented yet!");
			return false;
		}

		for (auto& parameter : m_Parameters)
		{
			if (parameter.Name == name)
			{
				if (isArray)
				{
					ZONG_CORE_ASSERT(value.size() <= 32);
					parameter.Value = value;
				}
				else
				{
					parameter.Value = value;
				}

				parameter.Changed.SetDirty();

				// Partition parameter list and set the end of range of changed parameters
				m_ChangedRangeEnd = (Parameter*)std::partition(begin(), end(), [](const Parameter& parameter) { return parameter.Changed.IsDirty(); });

				// Reset dirty flags
				std::for_each(begin(), m_ChangedRangeEnd, [](Parameter& param) { param.Changed.CheckAndResetIfDirty(); });

				Updated.SetDirty();
				return true;
			}
		}

		return false;
	}
	bool SoundGraphPatchPreset::SetParameter(uint32_t handle, const choc::value::ValueView& value)
	{
		const bool isArray = value.isArray();

		// For now we don't support sending array of values to SoundGraph patch
		if (isArray && Utils::IsWaveAsset(value[0]) || value.isObject())
		{
			ZONG_CORE_ASSERT(false, "Dynamically changing SoundGraph Wave asset arrays is not implemented yet!");
			return false;
		}

		for (auto& parameter : m_Parameters)
		{
			if (parameter.Handle == handle)
			{
				if (isArray)
				{
					ZONG_CORE_ASSERT(value.size() <= 32);
					parameter.Value = value;
				}
				else
				{
					parameter.Value = value;
				}

				parameter.Changed.SetDirty();

				// Partition parameter list and set the end of range of changed parameters
				m_ChangedRangeEnd = (Parameter*)std::partition(begin(), end(), [](const Parameter& parameter) { return parameter.Changed.IsDirty(); });

				// Reset dirty flags
				std::for_each(begin(), m_ChangedRangeEnd, [](Parameter& param) { param.Changed.CheckAndResetIfDirty(); });


				Updated.SetDirty();
				return true;
			}
		}

		return false;
	}
	bool SoundGraphPatchPreset::HasParameter(std::string_view name) const
	{
		for (const auto& parameter : m_Parameters)
		{
			if (parameter.Name == name)
				return true;
		}

		return false;
	}
	bool SoundGraphPatchPreset::HasParameter(uint32_t handle) const
	{
		for (const auto& parameter : m_Parameters)
		{
			if (parameter.Handle == handle)
				return true;
		}

		return false;
	}
	choc::value::ValueView SoundGraphPatchPreset::GetParameter(std::string_view name) const
	{
		for (const auto& parameter : m_Parameters)
		{
			if (parameter.Name == name)
				return parameter.Value;
		}

		return {};
	}
	choc::value::ValueView SoundGraphPatchPreset::GetParameter(std::string_view name, const choc::value::Value& defaultReturnValue) const
	{
		for (const auto& parameter : m_Parameters)
		{
			if (parameter.Name == name)
				return parameter.Value;
		}

		return defaultReturnValue;
	}

	const SoundGraphPatchPreset::Parameter* SoundGraphPatchPreset::GetLastChangedParameter() const
	{
		return m_ChangedRangeEnd;
	}

	void SoundGraphPatchPreset::AddParameter(Parameter&& newParameter)
	{
		m_Parameters.push_back(std::move(newParameter));
	}

} // namespace Engine
