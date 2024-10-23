#pragma once
#include "Engine/Editor/NodeGraphEditor/SoundGraph/SoundGraphAsset.h"
#include "Engine/Editor/NodeGraphEditor/SoundGraph/SoundGraphNodeEditorModel.h"
#include "SoundGraph.h"

#include "Utils/SoundGraphCache.h"

namespace Hazel::SoundGraph
{
	struct GraphGeneratorOptions
	{
		std::string Name;
		uint32_t NumInChannels;
		uint32_t NumOutChannels;
		Ref<SoundGraphAsset> Graph;
		SoundGraphNodeEditorModel* Model = nullptr;

		SoundGraphCache* Cache = nullptr;
	};

	Ref<Prototype> ConstructPrototype(GraphGeneratorOptions options, std::vector<UUID>& waveAssetsToLoad);

	/** Create instance of SoundGraph from Prototype for playback */
	Ref<SoundGraph> CreateInstance(const Ref<Prototype>& prototype);

} // namespace Hazel::SoundGraph
