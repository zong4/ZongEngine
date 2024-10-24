#include <pch.h>
#include "SoundGraphSource.h"
#include "WaveSource.h"

#include "Engine/Core/Hash.h"
#include "Engine/Debug/Profiler.h"

#include "text/choc_JSON.h"

namespace Engine
{
    bool RefillWavePlayerBuffer(Audio::WaveSource& waveSource, void* userData, uint32_t numFrames)
    {
        auto& buffer = waveSource;
        if (!buffer.WaveHandle)
            return false;

        auto* context = (Audio::DataSourceContext*)(userData);

        const uint32_t numChannels = 2; // TODO: get from somewhere. Initialize with the WaveSource struct?

        // TODO: try adding an option to read directly into CircularBuffer and advance the counters
        float readBuffer[1920]{ 0.0f }; //! Interleaved

        auto& reader = context->Readers.at(buffer.WaveHandle);
        buffer.TotalFrames = reader.TotalFrames;

        // Check how many samples we need to refill
		const int32_t available = buffer.Channels.Available() / numChannels;
        const int32_t samplesToRefill = numFrames - available;
        ZONG_CORE_ASSERT(samplesToRefill <= 960 / 2);

        if (samplesToRefill > 0 /*buffer.Channels[0].available() < numFrames*/)
        {
            // Read sample data from data source
            const int samplesRead = (int)reader.Read(readBuffer, samplesToRefill, buffer.ReadPosition + available, buffer.StartPosition);

            // Fill WaveSource with interleaved sample data
            buffer.Channels.PushMultiple(readBuffer, samplesRead * 2);
        }

        return true;
    }


    SoundGraphSource::SoundGraphSource()
    {
        // Set up callback to recieve messages from SoundGraph
        m_HandleConsoleMessage = [&](uint64_t frameIndex, const char* message)
        {
            if (message[0] == '!')
            {
                ZONG_CORE_ERROR(R"sgMessage([Sound Graph] Frame index: {0}
    Message: {1}", message, frameIndex)sgMessage", frameIndex, message);
            }
            else if (message[0] == '*')
            {
                ZONG_CORE_WARN(R"sgMessage([Sound Graph] Frame index: {0}
    Message: {1}", message, frameIndex)sgMessage", frameIndex, message);
            }
            else
            {
                ZONG_CORE_TRACE(R"sgMessage([Sound Graph] Frame index: {0}
    Message: {1}", message, frameIndex)sgMessage", frameIndex, message);
            }
        };

        // Set up callback to recieve outgoing events from SoundGraph
        m_HandleOutgoingEvent = [&](uint64_t frameIndex, Identifier endpointID, const choc::value::ValueView& eventData)
        {
			const auto endpointName = !endpointID.GetDBGName().empty() ? endpointID.GetDBGName() : "unknown";
            
			//? DBG
			//ZONG_CORE_INFO("[SoundGraph Patch Event] Endpoint name: {0} | frame index: {1} | event data: {2}", endpointName, frameIndex, choc::json::toString(eventData));

			if (endpointID == SoundGraph::SoundGraph::IDs::OnFinished)
			{
				bIsFinished = true;
			}
        };
    }


    //==============================================================================
    /// SoundGraph Interface
    bool SoundGraphSource::InitializeDataSources(const std::vector<AssetHandle>& dataSources)
    {
        bool result = true;

        for (auto& waveAsset : dataSources)
        {
            if (!m_DataSourceMap.InitializeReader(m_Engine->pResourceManager, waveAsset))
                result = false;
        }

        return result;
    }

    void SoundGraphSource::UninitializeDataSources()
    {
        m_DataSourceMap.Uninitialize();
    }

    //==============================================================================
    /// Audio Callback Interface
    bool SoundGraphSource::Init(uint32_t sampleRate, uint32_t maxBlockSize, const BusConfig& config)
    {
        m_SampleRate = sampleRate;
        m_BlockSize = maxBlockSize;

        //! Set up base node of inherited AudioCallback as a data source for an engine node
        ma_engine_node_config nodeConfig = ma_engine_node_config_init(m_Engine, ma_engine_node_type_group, MA_SOUND_FLAG_NO_SPATIALIZATION);

        const uint32_t numInChannels[1]{ 2 };
        //const uint32_t numOutChannels[1]{ 2 }; //? must match endpoint input channel count, or use channel converter
        nodeConfig.channelsIn = *config.InputBuses.data();
        nodeConfig.channelsOut = *config.OutputBuses.data();

        ma_result result;
        result = ma_engine_node_init(&nodeConfig, nullptr, &m_EngineNode);
        ZONG_CORE_ASSERT(result == MA_SUCCESS);

        result = ma_node_attach_output_bus(GetNode(), 0u, &m_EngineNode, 0u);
        ZONG_CORE_ASSERT(result == MA_SUCCESS);

        result = ma_node_attach_output_bus(&m_EngineNode, 0u, &m_Engine->nodeGraph.endpoint, 0u);

        return result == MA_SUCCESS;
    }

    void SoundGraphSource::ProcessBlock(const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
    {
		if (flag_SuspendProcessing.CheckAndResetIfDirty())
		{
			m_Suspended.store(true);
			m_IsPlaying.store(false);
		}

        if (m_Graph != nullptr && m_Graph->IsPlayable() && !IsSuspended())
        {

            // If parameters changed, send update events
            // 
            // Before we send Play event we need to update parameters, set initial preset values,
            // otherwise Play event will fail because Wave Source ID might be invalid / not set yet.
            if (flag_PresetUpToDate.CheckAndResetIfDirty())
            {
                const bool allParametersSet = ApplyParameterPresetInternal();
                if (allParametersSet)
                    m_PresetIsInitialized = true;
                else
				{
					HandleConsole(this, m_CurrentFrame.load(), "!SoundGraphSource::{} - failed to set all parameters of a preset.");
				}
            }

            if (m_PresetIsInitialized)
            {
                UpdateChangedParameters();

                if (flag_PlayRequested.CheckAndResetIfDirty())
                {
					m_Graph->SendInputEvent(SoundGraph::SoundGraph::IDs::Play, choc::value::Value(2.0f));

                    //ZONG_CORE_WARN("Play request sent");
                    m_CurrentFrame.store(0);
                    m_IsPlaying.store(true);
                }

                // TODO: consider not even rendering SoundGraph if Play event hasn't been set.
                //      Although in that case it won't be able to recieve any events.

                // miniaudio might request different amount of frames based on pitch settings 
                //ZONG_CORE_ASSERT(numFramesRequested <= outBuffer[0].getNumFrames());
                const uint32_t numFrames = *pFrameCountOut;
                const uint32_t numOutChannels = 2; // TODO: ???
                float* mainOutputBus = ppFramesOut[0];

                // TODO: wrap all render parameters into a render context struct

                // Refill buffers of WavePlayers
                m_Graph->BeginProcessBlock();
                
                for (uint32_t i = 0; i < numFrames; ++i)
                {
                    m_Graph->Process();

                    for (uint32_t ch = 0; ch < numOutChannels; ++ch)
                    {
                        // read data from the graph's endpoint to the output of this audio callback
                        const Identifier& id = m_Graph->OutputChannelIDs[ch];
                        const float sample = *(float*)(m_Graph->EndpointOutputStreams.InValue(id).getRawData());
                        mainOutputBus[i * numOutChannels + ch] = sample;
                    }
                }

                m_Graph->HandleOutgoingEvents(this, HandleEvent, HandleConsole);

                //if (m_IsPlaying) //? this might be 'false' but we still have processed this block of samples ?
                    m_CurrentFrame.fetch_add(numFrames, std::memory_order_relaxed);

                //m_RMSMeter.ProcessBlock(outBuffer[0]);
            }
            else
            {
				//? assuming we have 2 channels
				ma_silence_pcm_frames(ppFramesOut[0], *pFrameCountOut, ma_format_f32, 2);
            }
        }
        else
        {
			//? assuming we have 2 channels
			ma_silence_pcm_frames(ppFramesOut[0], *pFrameCountOut, ma_format_f32, 2);
        }
    }

    void SoundGraphSource::ReleaseResources()
    {
        //? This might backfire later if need to reinitialize and start playback
        //? If you forget to resume processing
        SuspendProcessing(true);
        UninitializeDataSources();
        ma_engine_node_uninit(&m_EngineNode, nullptr);
    }

    void SoundGraphSource::SuspendProcessing(bool shouldBeSuspended)
    {
		if (shouldBeSuspended)
		{
			flag_SuspendProcessing.SetDirty();
		}
		else
        {
			// Unpausing (unsuspending)
            m_IsPlaying.store(false);
            m_CurrentFrame.store(0);

			bIsFinished = false;
			m_GraphOutgoingEvents.reset();
			m_GraphOutgoingMessages.reset();

			// In case the flag was just set and not yet processed
			flag_SuspendProcessing.CheckAndResetIfDirty();
			m_Suspended.store(false);
        }
    }

    //==============================================================================
    /// Audio Update thread
    void SoundGraphSource::Update(float deltaTime)
    {
		const bool wasFinished = bIsFinished;

		// Handle outgoing events from audio render thread
		std::function<void()> outEvent;
		while (m_GraphOutgoingEvents.pop(outEvent))
			outEvent();

		// Handle outgoing messages from audio render thread
		std::function<void()> outMessage;
		while (m_GraphOutgoingMessages.pop(outMessage))
			outMessage();

		if (!wasFinished && bIsFinished)
		{
			SuspendProcessing(true);
		}
    }

    //==============================================================================
    /// SoundGraph Interface
    void SoundGraphSource::ReplacePlayer(const Ref<SoundGraph::SoundGraph>& newGraph)
    {
		if (newGraph == m_Graph)
			return;

        m_Graph = newGraph;

        m_Graph->SetRefillWavePlayerBufferCallback(&RefillWavePlayerBuffer, &m_DataSourceMap, m_BlockSize);

        m_Graph->Init();

        UpdateParameterSet();
    }

    //==============================================================================
    /// Patch Parameter Interface
    bool SoundGraphSource::SetParameter(std::string_view parameter, choc::value::ValueView value)
    {
        ZONG_PROFILE_FUNC();

        if (!m_Graph || value.isVoid())
        {
            ZONG_CORE_ASSERT(false);
            return false;
        }

        return SetParameter(Hash::GenerateFNVHash(parameter.data()), value);
    }

    bool SoundGraphSource::SetParameter(uint32_t parameterID, choc::value::ValueView value)
    {
        ZONG_PROFILE_FUNC();

        const auto& parameterInfo = m_ParameterHandles.find(parameterID);
        if (parameterInfo != m_ParameterHandles.end())
        {
			//ZONG_CONSOLE_LOG_INFO("Parameter: id {0}, value {1}", parameterInfo->first, choc::json::toString(value));

            PresetWrite presetSafe(m_Preset);
            return presetSafe->SetParameter(parameterInfo->second.Handle, value);
        }
        else
        {
            ZONG_CONSOLE_LOG_WARN("SetParameter(parameterID {0}, value {1}) - Parameter with ID {0} doesn't exist in current context."
                                , parameterID, choc::json::toString(value));
            return false;
        }
    }

    bool SoundGraphSource::ApplyParameterPreset(const Utils::PropertySet& preset)
    {
        if (!m_Graph)
        {
            ZONG_CORE_ASSERT(false);
            return false;
        }

        ZONG_CORE_ASSERT(!m_ParameterHandles.empty());

        const std::vector<std::string>& names = preset.GetNames();
        std::vector<uint32_t> handles;
        handles.reserve(names.size());

        //? might need to make a "safe identifier" before hashing
        //? to make sure property name would be the same as endpoint
        for (const auto& name : names)
		{
			const uint32_t parameterId = Hash::GenerateFNVHash(choc::text::replace(name, " ", ""));
			if (m_ParameterHandles.count(parameterId))
			{
				handles.emplace_back(m_ParameterHandles.at(parameterId).Handle);
			}
			else
			{
				ZONG_CORE_WARN_TAG("SoundGraphSource", "ApplyParameterPreset - SoundGraphSource doesn't have parameter '{}'", name);
				return false;
			}
		}

        PresetWrite presetSafe(m_Preset);
        presetSafe->InitializeFromPropertySet(preset, handles);
        flag_PresetUpToDate.SetDirty();

        return true;
    }

    void SoundGraphSource::UpdateParameterSet()
    {
        // TODO: move all this hashing and stuff to SoundGraphPatchPreset class?

        // TODO: add a method to ValidateParameterSet to make sure all names and handles and IDs are valid between SoundGraphSource and SoundGraph instance

        // Exposed input events of the nested processors and custom structures
        auto endpoints = m_Graph->GetInputEventEndpoints();

        // Events of the Main Graph, other input event types
        auto parameters = m_Graph->GetParameters();

        const size_t totalNumberOfParameters = endpoints.size() + parameters.size();

        m_ParameterHandles.clear();
        m_ParameterHandles.reserve(totalNumberOfParameters);

        uint32_t numParametersSet = 0;
        
        // TODO: endpoint names

        for (const auto& e : endpoints)
        {
            const auto& [element, inserted] = m_ParameterHandles.try_emplace(e, e);
            numParametersSet += inserted;
        }

        for (const auto& p : parameters)
        {
            const auto& [element, inserted] = m_ParameterHandles.try_emplace(p, p);
            numParametersSet += inserted;
        }

        ZONG_CORE_ASSERT(numParametersSet == totalNumberOfParameters);
    }

    bool SoundGraphSource::ApplyParameterPresetInternal()
    {
        ZONG_CORE_ASSERT(m_Graph);

        PresetRead presetSafe(m_Preset);
        uint32_t numParametersSet = 0;

        for (const SoundGraphPatchPreset::Parameter& parameter : *presetSafe)
        {
			// Set parameters without interpolation
            if (m_Graph->SendInputValue(parameter.Handle, parameter.Value, false))
            {
                ++numParametersSet;
                //? DBG. Careful, too many calls can caus audio glitches.
               /* ZONG_CORE_INFO("parameter set: {}", param->Name);
                if (value.isFloat())
                    ZONG_CORE_INFO("value: {}", value.get<float>());*/
            }
            else
            {
                ZONG_CORE_ASSERT(false, "Failed to send input event to SoundGraph instance. Most likely endpoint handle was invalid.")
            }
        }

        return numParametersSet == presetSafe->size();
    }

    void SoundGraphSource::UpdateChangedParameters()
    {
        ZONG_CORE_ASSERT(m_Graph);

        PresetRead presetSafe(m_Preset);
        const auto* changedEnd = presetSafe->GetLastChangedParameter();
        if (!presetSafe->Updated.CheckAndResetIfDirty() || !changedEnd || changedEnd == presetSafe->begin())
            return;

        for (const auto* param = presetSafe->begin(); param != changedEnd; ++param)
        {
            //? DBG
            //auto endpoint = m_Player->getEndpointDetails(param->Name.c_str());

            if (m_Graph->SendInputValue(param->Handle, param->Value, true))
            {
                //? DBG. Careful, too many calls can caus audio glitches.
               /* ZONG_CORE_INFO("parameter set: {}", param->Name);
                if (value.isFloat())
                    ZONG_CORE_INFO("value: {}", value.get<float>());*/
            }
            else
            {
                ZONG_CORE_ASSERT(false, "Failed to send input event to SoundGraph instance. Most likely endpoint handle was invalid.")
            }
        }

		
    }


	int SoundGraphSource::GetNumDataSources() const
	{
		return (int)m_DataSourceMap.Readers.size();
	}

	//==============================================================================
    /// Playback Interface
    bool SoundGraphSource::AreAllDataSourcesAtEnd()
    {
        return m_DataSourceMap.AreAllDataSourcesAtEnd();
    }

    bool SoundGraphSource::SendPlayEvent()
    {
        if (!m_Graph)
            return false;

        flag_PlayRequested.SetDirty();
        return true;
    }
} // namespace Engine
