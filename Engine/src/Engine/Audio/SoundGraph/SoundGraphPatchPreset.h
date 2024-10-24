#pragma once

#include "Engine/Editor/NodeGraphEditor/PropertySet.h"

#include "choc/containers/choc_Span.h"
#include "choc/containers/choc_SmallVector.h"

namespace Engine
{ 
	class SoundGraphPatchPreset
	{
	public:
		SoundGraphPatchPreset() = default;
		SoundGraphPatchPreset(const Utils::PropertySet& propertySet, const std::vector<uint32_t>& handles);

		void InitializeFromPropertySet(const Utils::PropertySet& propertySet, const std::vector<uint32_t>& handles);

		bool SetParameter(std::string_view name, const choc::value::ValueView& value);
		bool SetParameter(uint32_t handle, const choc::value::ValueView& value);
		bool HasParameter(std::string_view name) const;
		bool HasParameter(uint32_t handle) const;
		choc::value::ValueView GetParameter(std::string_view name) const;
		choc::value::ValueView GetParameter(std::string_view name, const choc::value::Value& defaultReturnValue) const;

		struct Parameter
		{
			uint32_t Handle;
			std::string Name;
			choc::value::Value Value;
			Flag Changed;
		};

		const Parameter* begin() const;
		const Parameter* end() const;
		size_t size() const;

		/** Retrurn pointer to the next after last changed parameter.
			Parameters are sorted by 'changed' flag when value changed.
		*/
		const Parameter* GetLastChangedParameter() const;

		/** Check from real-time thread if any of the parameters have changed. */
		mutable AtomicFlag Updated;

	private:
		void AddParameter(Parameter&& newParameter);

		/** Translate parameters that came from a Graph into a our custom data
			structures that we use in our SoundGraph.
			
			Parsing PropertySet that came from SoundGraph Editor Graph through this ensures
			that we don't have to do this from audio render thread before
			sending input events to SoundGraph.
		*/
		void TranslateParameters();

		Parameter* begin();
		Parameter* end();
		
	private:
		choc::SmallVector<Parameter, 32> m_Parameters;

		Parameter* m_ChangedRangeEnd = nullptr;
	};
} // namespace Engine
