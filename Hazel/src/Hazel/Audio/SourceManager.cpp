#include "hzpch.h"
#include "SourceManager.h"
#include "ResourceManager.h"
#include "AudioEngine.h"
#include "DSP/Reverb/Reverb.h"
#include "DSP/Spatializer/Spatializer.h"

#include "Hazel/Serialization/FileStream.h"

#include "SoundGraph/SoundGraphSound.h"
#include "SoundGraph/SoundGraphSource.h"
#include "SoundGraph/GraphGeneration.h"
#include "SoundGraph/Utils/SoundGraphCache.h"

namespace Hazel
{
    void SourceManager::AllocationCallback(uint64_t size)
    {
        auto& stats = MiniAudioEngine::Get().s_Stats;
        std::scoped_lock lock{ stats.mutex };
        stats.MemResManager += size;
    }

    void SourceManager::DeallocationCallback(uint64_t size)
    {
        auto& stats = MiniAudioEngine::Get().s_Stats;
        std::scoped_lock lock{ stats.mutex };
        stats.MemResManager -= size;
    }

	static constexpr auto MAX_NUM_CACHED_GRAPHS = 256; // TODO: get this value from somewhere reasonable

    //==================================================================================

    SourceManager::SourceManager(MiniAudioEngine& audioEngine, Audio::ResourceManager& resourceManager)
        : m_AudioEngine(audioEngine), m_ResourceManager(resourceManager)
    {
        m_Spatializer = CreateScope<Audio::DSP::Spatializer>();

        m_SoundGraphCache = CreateScope<SoundGraphCache>(MAX_NUM_CACHED_GRAPHS);
    }

    SourceManager::~SourceManager()
    {
        // TODO: release all of the sources
    }

    void SourceManager::Initialize()
    {
        m_Spatializer->Initialize(&m_AudioEngine.m_Engine);
	}

    void SourceManager::UninitializeEffects()
    {
        for (auto* source : m_AudioEngine.m_SoundSources)
            ReleaseSource(source->GetSourceID());

		m_Spatializer->Uninitialize();
    }

    bool SourceManager::InitializeSource(uint32_t sourceID, const Ref<SoundConfig>& sourceConfig)
    {
        // TODO: refactor this to use the same, or more common initialization interface

        // If sourceConfig is SoundGraphAsset, initialize SoundGraphSource
        AssetType type = AssetManager::GetAssetType(sourceConfig->DataSourceAsset);
        if (type == AssetType::SoundGraphSound) // TODO: !!! Replace this with the new Sound Graph
        {
			//? If asset is not loaded, it's goint to attempt to load it here, this must be avoided!
            auto graphAsset				= AssetManager::GetAsset<SoundGraphAsset>(sourceConfig->DataSourceAsset);

			// Try to get prototype from cache if it's not loaded yet to this asset
			//? This is wat too SLOW! Even if loaded when asset loads because first time it loads when it's needed to be used
			//? unless used by Graph Editor, it's going to load it from disk and compile into an instance now,
			//? when it's needed to be played!
			Ref<SoundGraph::Prototype>& prototype = graphAsset->Prototype;
			if (!prototype)
			{
				// TODO: need to set new cache directory when new project is loaded
				const std::filesystem::path cacheDirectory = Project::GetCacheDirectory() / "SoundGraph";
				m_SoundGraphCache->SetCacheDirectory(cacheDirectory);

				// TODO: this might be not the fastest thing ever, reading from disk at this stage might not be the best idea
				//		Special "Runtime" Cache could solve this by preloading into memory / asset packs
				std::string name = graphAsset->CachedPrototype.stem().string();
				if (name.find("sound_graph_cache_") != std::string::npos)
					name = name.substr(18);

				graphAsset->Prototype = m_SoundGraphCache->ReadPtototype(name.c_str());
			}

			// TODO: get prototype from Asset Pack / runtime cache
			//			Load from file first and then store in memory asset pack?
			//			Or preload asset pack when startign scene?

            Ref<SoundGraph::SoundGraph> graphInstance;
            if (graphAsset->Prototype)
            {
                graphInstance = SoundGraph::CreateInstance(graphAsset->Prototype);
            }
            else
            {
				HZ_CONSOLE_LOG_ERROR("SourceManager::InitializeSource. Failed to initialize source, SoundGraph has to be built.");
                return false;
            }

			SoundGraphSound* soundSource = static_cast<SoundGraphSound*>(m_AudioEngine.m_SoundSources.at(sourceID)->SetSound(CreateScope<SoundGraphSound>()));
            if (soundSource->InitializeDataSource(sourceConfig, &m_AudioEngine, graphInstance))
            {
                auto* reverbNode = m_AudioEngine.GetMasterReverb()->GetNode();

                ma_engine_node* node = soundSource->m_Source->GetEngineNode();

                if (!node)
                {
                    HZ_CORE_ASSERT(false, "Failed to initialize audio data soruce.");
                    return false;
                }

                // Initialize spatialization source
                if (sourceConfig->bSpatializationEnabled)
                    m_Spatializer->InitSource(sourceID, node, sourceConfig->Spatialization);


                //--- Set up Master Reverb send for the new source ---
                //----------------------------------------------------
                // TODO: Temp. In the future move this to Effects interface
                auto& splitterNode = soundSource->m_MasterSplitter;

                // Attach "FX send" output of the splitter to the Master Reverb node
                ma_node_attach_output_bus(&splitterNode, 1, reverbNode, 0);
                // Set send level to the Master Reverb
                ma_node_set_output_bus_volume(&splitterNode, 1, sourceConfig->MasterReverbSend);

                return true;
            }
            else
            {
                HZ_CORE_ASSERT(false, "Failed to initialize audio data soruce.");
            }
        }
        else
        {
			// TODO: JP. maybe completely remove this old non-SoundGraph API

            Sound* soundSource = static_cast<Sound*>(m_AudioEngine.m_SoundSources.at(sourceID)->SetSound(CreateScope<Sound>()));

            if (soundSource->InitializeDataSource(sourceConfig, &m_AudioEngine))
            {
                auto* reverbNode = m_AudioEngine.GetMasterReverb()->GetNode();

                // Initialize spatialization source
                if (sourceConfig->bSpatializationEnabled)
                    m_Spatializer->InitSource(sourceID, &soundSource->m_Sound.engineNode, sourceConfig->Spatialization);

                //--- Set up Master Reverb send for the new source ---
                //----------------------------------------------------
                // TODO: Temp. In the future move this to Effects interface
                auto& splitterNode = soundSource->m_MasterSplitter;

                // Attach "FX send" output of the splitter to the Master Reverb node
                ma_node_attach_output_bus(&splitterNode, 1, reverbNode, 0);
                // Set send level to the Master Reverb
                ma_node_set_output_bus_volume(&splitterNode, 1, sourceConfig->MasterReverbSend);

                return true;
            }
            else
            {
                HZ_CORE_ASSERT(false, "Failed to initialize audio data soruce.");
            }
        }

        return false;
    }

    void SourceManager::ReleaseSource(uint32_t sourceID)
    {
        m_Spatializer->ReleaseSource(sourceID);
        m_AudioEngine.m_SoundSources.at(sourceID)->ReleaseResources();

        m_FreeSourcIDs.push(sourceID);
    }

    bool SourceManager::GetFreeSourceId(int& sourceIdOut)
    {
        if (m_FreeSourcIDs.empty())
            return false;

        sourceIdOut = m_FreeSourcIDs.front();
        m_FreeSourcIDs.pop();
        
        return true;
    }

    void SourceManager::SetMasterReverbSendForSource(uint32_t sourceID, float sendLevel)
    {
        // TODO: Temp. In the future move this to Effects interface

        auto& engine = MiniAudioEngine::Get();
        //auto* reverbNode = engine.GetMasterReverb()->GetNode();

        auto* sound = dynamic_cast<Sound*>(engine.m_SoundSources.at(sourceID));
        
        // TODO: SoundGraph Sounds not going to use master reverb for now.
        if (!sound)
            return;

        auto& splitterNode = sound->m_MasterSplitter;

        // Set send level to the Master Reverb
        ma_node_set_output_bus_volume(&splitterNode, 1, sendLevel);
    }

    void SourceManager::SetMasterReverbSendForSource(SoundObject* source, float sendLevel)
    {
        SetMasterReverbSendForSource(source->GetSourceID(), sendLevel);
    }

    float SourceManager::GetMasterReverbSendForSource(SoundObject* source)
    {
        auto& engine = MiniAudioEngine::Get();

        auto* sound = dynamic_cast<Sound*>(source);

        // TODO: SoundGraph Sounds not going to use master reverb for now.
        if (!sound)
            return -1.0f;

        auto& splitterNode = sound->m_MasterSplitter;

        return ma_node_get_output_bus_volume(&splitterNode, 1);
    }

} // namespace Hazel
