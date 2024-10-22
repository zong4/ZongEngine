#pragma once

#include "Hazel/Audio/AudioCallback.h"

#include "Hazel/Asset/Asset.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Editor/NodeGraphEditor/PropertySet.h"
#include "Hazel/Audio/SoundGraph/SoundGraphPatchPreset.h"
#include "Hazel/Audio/AudioReader.h"

#include "SoundGraph.h"

#include "farbot/RealtimeObject.hpp"

#include <filesystem>

namespace Hazel
{
    // TODO: move "Suspendable" into a CRTP Skill, as well as callback itself. 
    // TODO: Playback interface should be separate from SoundGraph reading source that processes SoundGraph (?)
    //==============================================================================
    /** Data source interface between SoundGraphSound and SoundGraph instance.
    */
    class SoundGraphSource : public Audio::AudioCallbackInterleaved<SoundGraphSource>
    {
    public:
        explicit SoundGraphSource();

        /** Get output node of this Audio Callback interface */
        const ma_engine_node* GetEngineNode() const { return &m_EngineNode; };
        ma_engine_node* GetEngineNode() { return &m_EngineNode; };

        //==============================================================================
        /// Audio Callback Interface
        bool Init(uint32_t sampleRate, uint32_t maxBlockSize, const BusConfig& config);
        void ProcessBlock(const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut);
        void ReleaseResources() final;

        void SuspendProcessing(bool shouldBeSuspended) override;
        bool IsSuspended() override { return m_Suspended.load(); }

		bool IsFinished() const noexcept { return bIsFinished && !m_IsPlaying; }

        //==============================================================================
        /// Audio Update thread
        void Update(float deltaTime);

        //==============================================================================
        /// SoundGraph Interface
        bool InitializeDataSources(const std::vector<AssetHandle>& dataSources);
        void UninitializeDataSources();
        
        /** Replace current graph with new one. This is called when a new graph has
            been constructed to assign it to this interface.
        */
        void ReplacePlayer(const Ref<SoundGraph::SoundGraph>& newGraph);
        Ref<SoundGraph::SoundGraph> GetGraph() { return m_Graph; }

        //==============================================================================
        /// Patch Parameter Interface
        
        /** Set graph parameter value. This will first hash the 'parameter' name,
            for better performance prefer using overload which takes 'parameterID',
            which is a prehashed parameter name.
        
            @param parameter    - parameter to change the value of.
            @param value        - must be of suppoerted value type.
            @return             'true' if parameter of supported type has been found
                                and set to new value.
        */
        bool SetParameter(std::string_view parameter, choc::value::ValueView value);

        /** Set graph parameter value.
        
            @param parameter    - parameter to change the value of.
            @param value        - must be of suppoerted value type.
            @return             'true' if parameter of supported type has been found
                                and set to new value.
        */
        bool SetParameter(uint32_t parameterID, choc::value::ValueView value);

        /** Apply new preset to SoundGraph. This will first parse PropertySet
            to SoundGraphPatchPreset and then apply each parameter to SoundGraph
            on audio thread as input events.
        
            @param preset       - PropertySet supplied in the form used by SoundGraph Editor graph (?)
            @return             'false' if patch player is not initialized and the
                                preset wasn't applied
        */
        bool ApplyParameterPreset(const Utils::PropertySet& preset);


        //==============================================================================
        /// SoundGraph outgoing events
        static void HandleEvent(void* context, uint64_t frameIndex, Identifier endpointID, const choc::value::ValueView& eventData)
        {
			//! This is still in audio render thread!

            /*if (auto& fn = static_cast<SoundGraphSource*> (context)->m_HandleOutgoingEvent)
                fn(frameIndex, endpointID, eventData);*/

			auto* owner = static_cast<SoundGraphSource*>(context);
			owner->m_GraphOutgoingEvents.push({ [=]					//? this is bad, this is going probably to allocate and is called inside of audio Process Block
			{
				owner->m_HandleOutgoingEvent(frameIndex, endpointID, eventData);
			} });
        }
        static void HandleConsole(void* context, uint64_t frameIndex, const char* message)
        {
			//! This is still in audio render thread!

            /*if (auto& fn = static_cast<SoundGraphSource*> (context)->m_HandleConsoleMessage)
                fn(frameIndex, message);*/

			// TODO: mabe have preallocated buffer to copy the message into from render thread?
			//		otherwise the message might get invalidated by the time Update thread gets
			//		to ouput it
			auto* owner = static_cast<SoundGraphSource*>(context);
			owner->m_GraphOutgoingMessages.push({ [=]
			{
				owner->m_HandleConsoleMessage(frameIndex, message);
			} });
        }

        //==============================================================================
        /// Playback Interface
		int GetNumDataSources() const;
        bool AreAllDataSourcesAtEnd();
        bool IsAnyDataSourceReading() { return !AreAllDataSourcesAtEnd(); }

        bool SendPlayEvent();

        uint64_t GetCurrentFrame() { return m_CurrentFrame.load(); }

    private:
        /** Called after SoundGraph has been reset to collect endpoint handles
            and map them to hashed parameter names.
        */
        void UpdateParameterSet();

        /** Called from audio render thread to update preset if changed. */
        bool ApplyParameterPresetInternal();

        /** Called from audio render thread to send updated parameters
            to SoundGraph as input events.
        */
        void UpdateChangedParameters();

    private:
        //============================================
        /// Audio Callback data
        std::atomic<bool> m_Suspended = false;
		AtomicFlag flag_SuspendProcessing;
        uint32_t m_SampleRate = 0;
        uint32_t m_BlockSize = 0;

        std::atomic<bool> m_IsPlaying = false;
        std::atomic<uint64_t> m_CurrentFrame = 0;

		// Has played and finished
		bool bIsFinished = false;

        //============================================
        /// Audio Rounting and Source
        
        // Output node.
        ma_engine_node m_EngineNode;
        Ref<SoundGraph::SoundGraph> m_Graph = nullptr;

        //============================================
        /// Parameter mapping
        struct PrameterInfo
        {
            uint32_t Handle; // TODO: use Identifier, or store Identifiers in a contiguous array with Handle being index in this array?
             // TODO std::string Name;

            PrameterInfo(uint32_t handle/*, std::string_view name*/)
                : Handle(handle)/*, Name(name) */{}
        };
        std::unordered_map<uint32_t, PrameterInfo> m_ParameterHandles;

        //============================================
        /// Communication with audio render thread
		using PresetSafe = farbot::RealtimeObject<SoundGraphPatchPreset, farbot::RealtimeObjectOptions::nonRealtimeMutatable>;
        using PresetWrite = PresetSafe::ScopedAccess<farbot::ThreadType::nonRealtime>;
        using PresetRead = PresetSafe::ScopedAccess<farbot::ThreadType::realtime>;

        PresetSafe m_Preset;

		AtomicFlag flag_PresetUpToDate;
        AtomicFlag flag_PlayRequested;

        //============================================
        /// Audio render thread player specific data
        
        // Must be accessed only from audio thread. Set once when the
        // initial preset has been applied to SoundGraph.
        bool m_PresetIsInitialized = false;
        Audio::DataSourceContext m_DataSourceMap;

        //============================================
        /// SoundGraph outgoing event callbacks
		using OnGraphMessageCb = std::function<void(uint64_t frameIndex, const char* message)>;
		using OnGraphEventCb = std::function<void(uint64_t frameIndex, Identifier endpointID, const choc::value::ValueView& eventData)>;
		OnGraphMessageCb m_HandleConsoleMessage = nullptr;
		OnGraphEventCb m_HandleOutgoingEvent = nullptr;

		choc::fifo::SingleReaderSingleWriterFIFO<std::function<void()>> m_GraphOutgoingEvents;
		choc::fifo::SingleReaderSingleWriterFIFO<std::function<void()>> m_GraphOutgoingMessages;
    };
} // namespace Hazel
