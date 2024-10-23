#include <hzpch.h>
#include "Spatializer.h"

#include "Engine/Audio/Sound.h"

#include "Engine/Debug/Profiler.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <execution>


namespace Hazel::Audio::DSP
{
    
    using ChannelGains = std::array<float, MA_MAX_CHANNELS>;


    //==============================================================================
    /// PROCESS SPATIALIZATION PARAMETERS

    static float ProcessDistanceAttenuation(AttenuationModel attenuationModel, float distance, float minDistance, float maxDistance, float rolloff)
    {
        switch (attenuationModel)
        {
        case AttenuationModel::Inverse:
        {
            if (minDistance >= maxDistance)
                return 1.0f;   /* To avoid division by zero. Do not attenuate. */

            return minDistance / (minDistance + rolloff * (std::clamp(distance, minDistance, maxDistance) - minDistance));
        } break;
        case AttenuationModel::Linear:
        {
            if (minDistance >= maxDistance)
                return 1.0f;   /* To avoid division by zero. Do not attenuate. */

            return 1.0f - rolloff * (std::clamp(distance, minDistance, maxDistance) - minDistance) / (maxDistance - minDistance);
        } break;
        case AttenuationModel::Exponential:
        {
            if (minDistance >= maxDistance)
                return 1.0f;   /* To avoid division by zero. Do not attenuate. */

            return (float)std::pow(std::clamp(distance, minDistance, maxDistance) / minDistance, -rolloff);
        } break;
        case AttenuationModel::None:
        default:
        {
            return 1.0f;
        } break;
        }
    }

    static float ProcessAngularAttenuation(glm::vec3 dirSourceToListener, glm::vec3 dirSource, float coneInnerAngleInRadians, float coneOuterAngleInRadians, float coneOuterGain)
    {
        if (coneInnerAngleInRadians < 6.283185f)
        {
            float angularGain = 1.0f;
            float cutoffInner = (float)cos(coneInnerAngleInRadians * 0.5f);
            float cutoffOuter = (float)cos(coneOuterAngleInRadians * 0.5f);
            float d;

            d = glm::dot(dirSourceToListener, dirSource);

            if (d > cutoffInner)
            {
                // It's inside the inner angle.
                angularGain = 1;
            }
            else
            {
                // It's outside the inner angle.
                if (d > cutoffOuter)
                {
                    // It's between the inner and outer angle. We need to linearly interpolate between 1 and coneOuterGain.
                    angularGain = Lerp(coneOuterGain, 1, (d - cutoffOuter) / (cutoffInner - cutoffOuter));
                }
                else
                {
                    // It's outside the outer angle.
                    angularGain = coneOuterGain;
                }
            }

            return angularGain;
        }
        else
        {
            // Inner angle is 360 degrees so no need to do any attenuation.
            return 1.0f;
        }
    }

    static float ProcessDopplerPitch(glm::vec3 relativePosition, glm::vec3 sourceVelocity, glm::vec3 listenVelocity, float speedOfSound, float dopplerFactor)
    {
        float len;
        float vls;
        float vss;

        len = glm::length(relativePosition);

        if (len == 0)
            return 1.0;

        vls = glm::dot(relativePosition, listenVelocity) / len;
        vss = glm::dot(relativePosition, sourceVelocity) / len;

        vls = std::min(vls, speedOfSound / dopplerFactor);
        vss = std::min(vss, speedOfSound / dopplerFactor);

        return (speedOfSound - dopplerFactor * vls) / (speedOfSound - dopplerFactor * vss);
    }


    //==============================================================================
    /// AUDIO CALLBACK

    void spatializer_node_process_pcm_frames(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
    {
        //===== Audio Buffer data ======

        const float* pFramesIn_0 = ppFramesIn[0];   // Input bus @ index 0.
        float* pFramesOut_0 = ppFramesOut[0];       // Output bus @ index 0.
        uint32_t frameCount = *pFrameCountIn;

        //===== User data ======

        Spatializer::spatializer_node* node = static_cast<Spatializer::spatializer_node*>(pNode);
        VBAPData& vbap = *node->vbap;
        auto* pEngineNode = node->targetEngineNode;

        const uint32_t channelsIn = node->channelsIn;
        const uint32_t channelsOut = node->channelsOut;

        //===== Retrieve updated panning gain values ======

        if (node->IsDirty())
        {

            for (auto& chg : vbap.ChannelGroups)
            {
                farbot::RealtimeObject<ChannelGains, farbot::RealtimeObjectOptions::nonRealtimeMutatable>::ScopedAccess<farbot::ThreadType::realtime> gains(chg.Gains);

                // Set interpolation target values for the gainer
                if (!SampleBufferOperations::ContentMatches(chg.Gainer.pNewGains, gains->data(), chg.Gainer.config.channels, 1))
                    memcpy(chg.Gainer.pNewGains, gains->data(), sizeof(float) * chg.Gainer.config.channels);
            }

            pEngineNode->spatializer.dopplerPitch = node->DopplerPitch;
        }

        //===== Apply Panning Effect to the Output ======

        ma_silence_pcm_frames(pFramesOut_0, frameCount, ma_format_f32, channelsOut);

        // Push data from all input channels to all output channels applying Vecrtor Based Amplitude Panning gains

        // Should use the actual number of channels of the source, not what miniaudio outputs from ma_sound node into here
        // as we intitialized ChannelGroups per source channel.
        const uint32_t numSourceInputs = (uint32_t)vbap.ChannelGroups.size();
        for (uint32_t iChannelIn = 0; iChannelIn < numSourceInputs; ++iChannelIn)
        {
            auto& gainer = vbap.ChannelGroups[iChannelIn].Gainer;

            for (uint32_t iChannelOut = 0; iChannelOut < channelsOut; ++iChannelOut)
            {
                const float startGain = gainer.pOldGains[iChannelOut];
                const float endGain = gainer.pNewGains[iChannelOut];

                SampleBufferOperations::AddAndApplyGainRamp(pFramesOut_0, pFramesIn_0, iChannelOut, iChannelIn, channelsOut, channelsIn, frameCount, startGain, endGain);
                gainer.pOldGains[iChannelOut] = endGain;
            }
        }
    }


    //==============================================================================
    /// SPATIALIZER DEFINITIONS

    static ma_node_vtable spatializer_node_vtable = {
    spatializer_node_process_pcm_frames,
    nullptr,
    1, // 1 input bus.
    1, // 1 output bus.
    0 // Default flags.
    };

    Spatializer::spatializer_node::spatializer_node(const spatializer_node& other)
        : base(other.base)
        , channelsIn(other.channelsIn)
        , channelsOut(other.channelsOut)
        , DopplerPitch(other.DopplerPitch.load())
        , targetEngineNode(other.targetEngineNode)
    {
        vbap.reset(other.vbap.get());
    }

    Spatializer::~Spatializer()
    {
        Uninitialize();
    }

    bool Spatializer::Initialize(ma_engine* engine_)
    {
        m_Engine = engine_;

        return true;
    }

    void Spatializer::Uninitialize()
    {
		while(!m_Sources.empty())
		{
			auto it = m_Sources.begin();
			if (!ReleaseSource(it->first))
			{
				HZ_CORE_ASSERT(false, "Failed to uninitialize Spatializer Source while uninitializing Spatializer! Source ID: {0}", it->first);
				break;
			}
		}

		m_Engine = nullptr;
    }

    bool Spatializer::IsInitialized(uint32_t sourceID) const
    {
        return m_Sources.find(sourceID) != m_Sources.end() && m_Sources.at(sourceID).bInitialized;
    }

    float Spatializer::GetCurrentDistanceAttenuation(uint32_t sourceID) const
    {
        HZ_CORE_ASSERT(IsInitialized(sourceID))
        return m_Sources.at(sourceID).DistanceAttenuationFactor;
    }

    float Spatializer::GetCurrentConeAngleAttenuation(uint32_t sourceID) const
    {
        HZ_CORE_ASSERT(IsInitialized(sourceID))
        return m_Sources.at(sourceID).AngleAttenuationFactor;
    }

    float Spatializer::GetCurrentDistance(uint32_t sourceID) const
    {
        HZ_CORE_ASSERT(IsInitialized(sourceID));
        return m_Sources.at(sourceID).Distance;
    }


    //==============================================================================
    /// MANUAL UPDATE

    void Spatializer::SetSpread(uint32_t sourceID, float newSpread)
    {
        if (!IsInitialized(sourceID))
        {
            HZ_CORE_ERROR_TAG("Sound Spatializer", "SetSpread({0}, {1}) - invalid sourseID.", sourceID, newSpread);
            return;
        }

        Source& source = m_Sources.at(sourceID);

        if (source.SpatializationConfig->bSpreadFromSourceSize)
        {
            HZ_CORE_WARN_TAG("Sound Spatializer", "Trying to update sound Spread value while using SpreadFromSourceSize.");
            return;
        }

        source.Spread = std::clamp(newSpread, 0.0f, 1.0f);
        UpdateVBAP(source);
        FlagRealtimeForUpdate(source);
    }

    void Spatializer::SetFocus(uint32_t sourceID, float newFocus)
    {
        if (!IsInitialized(sourceID))
        {
            HZ_CORE_ERROR_TAG("Sound Spatializer", "SetFocus({0}, {1}) - invalid sourseID.", sourceID, newFocus);
            return;
        }

        Source& source = m_Sources.at(sourceID);

        source.Focus = std::clamp(newFocus, 0.0f, 1.0f);
        UpdateVBAP(source);
        FlagRealtimeForUpdate(source);
    }


    //==============================================================================
    /// UPDATE  PARAMETERS AFTER A POSITION CHANGE

    float Spatializer::GetSpreadFromSourceSize(float sourceSize, float distance)
    {
        if (distance <= 0.0f)
            return 1.0f;

        float degreeSpread = glm::degrees(atanf((0.5f * sourceSize) / distance)) * 2.0f;
        return degreeSpread / 180.0f;
    }

    void Spatializer::UpdatePositionalData(Source& source, const ma_spatializer_listener* listener)
    {
        auto* pEngineNode = source.SpatializerNode.targetEngineNode;
        const float distance = source.Distance;
        const glm::vec3 positionRelative = source.PositionRelative;   // Source to listener position
        const glm::vec3 position = source.Transform.Position;         // Source absolute position
        const glm::vec3 relativeDir = source.RelativeDir;             // Source to listener direction
        const glm::vec3 velocity = source.Velocity;

        // TODO: rewrite this to use my listener, instead of ma_listener
        
        //const int channels = node->channels;

        //===== Distance Attenuation ======

        float gainAttenuation = ProcessDistanceAttenuation(source.SpatializationConfig->AttenuationMod,
                                                           source.Distance,
                                                           source.SpatializationConfig->MinDistance,
                                                           source.SpatializationConfig->MaxDistance,
                                                           source.SpatializationConfig->Rolloff);

        // Store value to be used elsewehere
        source.DistanceAttenuationFactor = gainAttenuation;

        // TODO: expose Cone Angle and Distance based attenuation factors separately to use with filters and user parameters

        //===== Cone Angle Attenuation ======

        // TODO: rewrite this to use stored relativeTransform matrix
        if (distance > 0)
        {
            float angleAttenuation = 1.0f;

            // Source anglular gain.
            angleAttenuation *= ProcessAngularAttenuation(glm::vec3(0.0f, 0.0f, -1.0f), relativeDir, source.SpatializationConfig->ConeInnerAngleInRadians,
                                                                                                     source.SpatializationConfig->ConeOuterAngleInRadians,
                                                                                                     source.SpatializationConfig->ConeOuterGain);
            
            /** Listener angular gain, to reduce the volume of sounds that are position behind the listener.
                  On default settings, this will have no effect.
              */
            if (listener != nullptr && listener->config.coneInnerAngleInRadians < 6.283185f)
            {
                glm::vec3 listenerDirection;
                float listenerInnerAngle;
                float listenerOuterAngle;
                float listenerOuterGain;

                if (listener->config.handedness == ma_handedness_right)
                {
                    listenerDirection = glm::vec3(0, 0, -1);
                }
                else
                {
                    listenerDirection = glm::vec3(0, 0, +1);
                }

                listenerInnerAngle = listener->config.coneInnerAngleInRadians;
                listenerOuterAngle = listener->config.coneOuterAngleInRadians;
                listenerOuterGain = listener->config.coneOuterGain;

                angleAttenuation *= ProcessAngularAttenuation(listenerDirection, glm::normalize(positionRelative), listenerInnerAngle, listenerOuterAngle, listenerOuterGain);

                //HZ_CORE_TRACE_TAG("Sound Spatializer", "Angle Attenuation: {}", angleAttenuation);
            }

            gainAttenuation *= angleAttenuation;

            // Store value to be used elsewehere
            source.AngleAttenuationFactor = angleAttenuation;
        }
        else
        {
            /* The sound is right on top of the listener. Don't do any angular attenuation. */
        }

        // TODO: make this affect where the curve starts and where it hits 0.0f instead of just clamping the multiplier
        gainAttenuation = std::clamp(gainAttenuation, source.SpatializationConfig->MinGain, source.SpatializationConfig->MaxGain);

        //HZ_CORE_TRACE_TAG("Sound Spatializer", "Gain attenuation: {}", gainAttenuation);

        glm::vec3 listenerVel(0.0f);
        if (listener)
        {
            listenerVel = glm::vec3{ listener->velocity.x, listener->velocity.y, listener->velocity.z };
        }


        //===== Doppler Effect ======

        /*
            Pitch shifting applied externally by Miniaudio. Note that we need to negate the relative positionRelative here
            because the doppler calculation needs to be source-to-listener, but ours is listener-to-
            source.
         */

        if (source.SpatializationConfig->DopplerFactor > 0.0f)
        {
            if (listener != nullptr)
            {
                glm::vec3 lp(listener->position.x, listener->position.y, listener->position.z);

                source.SpatializerNode.DopplerPitch = ProcessDopplerPitch(lp - position, velocity, listenerVel, SPEED_OF_SOUND, source.SpatializationConfig->DopplerFactor);

                //? A bit of a hack to let Miniaudio know that we need to update pitch and handle all of the resapling boilerplate
                pEngineNode->spatializer.dopplerPitch = source.SpatializerNode.DopplerPitch;
            }
        }
        else
        {
            source.SpatializerNode.DopplerPitch = 1.0f;
        }
    }

    void Spatializer::UpdateVBAP(Source& source, bool isInitialPosition /*= false*/)
    {
        VBAP::PositionUpdateData updateData{ source.vbapAzimuth,
                                             source.Spread,
                                             source.Focus,
                                             source.DistanceAttenuationFactor * source.AngleAttenuationFactor };

        VBAP::UpdateVBAP(source.SpatializerNode.vbap.get(), updateData, source.Converter, isInitialPosition);
    }

    void Spatializer::FlagRealtimeForUpdate(Source& source)
    {
        source.SpatializerNode.fGainsUpToDate.clear();
    }


    //==============================================================================
    /// SPATIALIZED SOURCES

    bool Spatializer::InitSource(uint32_t sourceID, ma_engine_node* nodeToInsertAfter, const Ref<SpatializationConfig>& config)
    {
        HZ_CORE_ASSERT(m_Sources.find(sourceID) == m_Sources.end());
        
        //Source source;
        //source.sourceID = sourceID;
        const auto& [it, success] = m_Sources.emplace(sourceID, Source());
        auto& source = it->second;


        auto abortIfFailed = [&](ma_result result, const char* errorMessage)
        {
            if (result != MA_SUCCESS)
            {
                HZ_CORE_ASSERT(false && errorMessage);
                ReleaseSource(sourceID);
                return true;
            }

            return false;
        };

        ma_allocation_callbacks* allocationCallbacks = &m_Engine->pResourceManager->config.allocationCallbacks;

        //------------- Base Node -------------
        ma_result result;

        // Using Quad virtual speaker setup for panning
        source.InternalChannelCount = 4;

        const uint32_t sourceChannels = nodeToInsertAfter->resampler.config.channels;
        const uint32_t sourceNodeChannels = ma_node_get_output_channels(nodeToInsertAfter, 0);

        const uint32_t numInputChannels[1]{ sourceNodeChannels };
        const uint32_t numOutputChannels[1]{ sourceNodeChannels };
        const uint32_t numIntermChannels[1]{ source.InternalChannelCount };

        ma_node_config nodeConfig = ma_node_config_init();
        nodeConfig.vtable = &spatializer_node_vtable;
        nodeConfig.pInputChannels = numInputChannels;
        nodeConfig.pOutputChannels = numOutputChannels;
        nodeConfig.initialState = ma_node_state_stopped;

        result = ma_node_init(&m_Engine->nodeGraph, &nodeConfig, allocationCallbacks, &source.SpatializerNode);
        if (abortIfFailed(result, "Node Init failed"))
            return false;

        source.SpatializerNode.channelsIn = numInputChannels[0];
        source.SpatializerNode.channelsOut = numOutputChannels[0];
        source.SpatializerNode.targetEngineNode = nodeToInsertAfter;


        //----------------- VBAP -----------------
         
        // Surround channel map for MDAP
        ma_get_standard_channel_map(ma_standard_channel_map_default, source.InternalChannelMap, sizeof(source.InternalChannelMap) / sizeof(source.InternalChannelMap[0]), source.InternalChannelCount);

        // Initialize channel converter
        ma_channel_converter_config channelMapConfig = ma_channel_converter_config_init(
            ma_format_f32,                      // Sample format
            source.InternalChannelCount,        // Input channels
            source.InternalChannelMap,          // Input channel map
            numOutputChannels[0],               // Output channels
            NULL,                               // Output channel map
            ma_channel_mix_mode_rectangular);   // The mixing algorithm to use when combining channels.

        result = ma_channel_converter_init(&channelMapConfig, &source.Converter);
        if (abortIfFailed(result, "Channel converter Init failed"))
            return false;

        //? For now only using 4 channels intermediate layout, so no need to initialize converter for each Source
        // Store channel map for MDAP
        ma_get_standard_channel_map(ma_standard_channel_map_default, source.SourceChannelMap, sizeof(source.SourceChannelMap) / sizeof(source.SourceChannelMap[0]), sourceChannels);


        //-------- Initialize Spatializer ----------

        source.SpatializationConfig = config;

        source.Spread = config->Spread;
        source.Focus = config->Focus;

        // Need to make sure to initialize VBAP to the number of channels of the input source file
        // otherwise the panning is weird
        source.SpatializerNode.vbap = CreateScope<VBAPData>();
        bool res = VBAP::InitVBAP(source.SpatializerNode.vbap.get(), sourceChannels, source.InternalChannelCount, source.SourceChannelMap, source.InternalChannelMap);
        if (abortIfFailed(res ? MA_SUCCESS : MA_INVALID_ARGS, "Channel converter Init failed"))
            return false;

        //-------- Routing ----------

        auto* output = nodeToInsertAfter->baseNode.pOutputBuses->pInputNode;

        // attach spatializer output to the same destination as the node it's connecting to
        result = ma_node_attach_output_bus(&source.SpatializerNode, 0, output, 0);
        if (abortIfFailed(result, "Node attach failed"))
            return false;

        // attach passed in node to spatializer
        result = ma_node_attach_output_bus(nodeToInsertAfter, 0, &source.SpatializerNode, 0);
        if (abortIfFailed(result, "Node attach failed"))
            return false;

        source.bInitialized = true;

        return true;
    }

    bool Spatializer::ReleaseSource(uint32_t sourceID)
    {
        auto sIt = m_Sources.find(sourceID);
        if (sIt == m_Sources.end())
        {
			//? Disabled the error massage to prevent the spam. Neen to not update position on audio objects with disabled spatializer.
			//HZ_CORE_ERROR_TAG("Sound Spatializer", "Invalid sourceID {}.", sourceID);
            return false;
        }

        auto abortIfFailed = [&](ma_result result, const char* errorMessage)
        {
            if (result != MA_SUCCESS)
            {
                HZ_CORE_ASSERT(false && errorMessage);
                Uninitialize();
                return true;
            }

            return false;
        };

        Source& source = sIt->second;

        // Reattach input to output
        auto* output = source.SpatializerNode.base.pOutputBuses->pInputNode;
        auto* input = source.SpatializerNode.targetEngineNode;

        ma_result result = ma_node_attach_output_bus(input, 0, output, 0); //? might want to stop the spatializer node before doing this
        if (abortIfFailed(result, "Node attach failed"))
            return false;

        // Uninitialize miniaudio nodes
        if (source.bInitialized)
        {
            if (((ma_node_base*)&source.SpatializerNode)->vtable != nullptr)
            {
                ma_allocation_callbacks* allocationCallbacks = &m_Engine->pResourceManager->config.allocationCallbacks;
                // ma_engine could have been uninitialized by this point, in which case allocation callbacs would've been invalidated
                if (!allocationCallbacks->onFree)
                    allocationCallbacks = nullptr;

                ma_node_uninit(&source.SpatializerNode, allocationCallbacks);
                source.SpatializerNode.targetEngineNode = nullptr;
            }
        }

        // Clear VBAP
        VBAP::ClearVBAP(source.SpatializerNode.vbap.get());

        source.bInitialPositionSet = false;
        source.bInitialized = false;

        m_Sources.erase(sourceID);

        return true;
    }

    void Spatializer::UpdateSourcePosition(uint32_t sourceID, const Audio::Transform& transform, glm::vec3 velocity)
    {
        auto sIt = m_Sources.find(sourceID);
        if (sIt == m_Sources.end())
        {
            //? For now this is going to be firing for all of the entities with disabled spatialization.
            //? Disabled the error massage to prevent the spam. Neen to not update position on audio objects with disabled spatializer.
            //HZ_CORE_ERROR_TAG("Sound Spatializer", "UpdateSourcePosition() Invalid sourceID {}.", sourceID);
            return;
        }

        Source& source = sIt->second;

        if (!source.bInitialized)
        {
            HZ_CORE_ASSERT(false);
            return;
        }

        auto& position = transform.Position;
        auto& orientation = transform.Orientation;
        auto& sourceUp = transform.Up;

        const auto& lp = m_ListenerTransform.Position;
        const auto& lr = m_ListenerTransform.Orientation;
        const auto& lup = m_ListenerTransform.Up;

        // Position of the source ralative to listener plane
        const glm::mat4 lookatM = glm::lookAt(lp, lp + lr, lup);
        glm::vec3 relativePos = lookatM * glm::vec4(position, 1.0f);

        // Direction from the source to listener in source's plane
        const glm::mat4 lookatMR = glm::lookAt(position, position + orientation, sourceUp);
        glm::vec3 relativeDir = glm::normalize(lookatMR * glm::vec4(lp, 1.0f));

        // TODO: pass in transform matrix as function argument instead of doing this double conversion

        const float distance = glm::length(relativePos);

        // When the sound is on top of us, we can't do vector math properly
        if (distance < 1e-6f)   //? this might still backfire when the source is above the listener but the distance is more than 0
        {
            //pos.z = -0.001f;
            relativePos.z = -0.001f;
            return;
        }

        // TODO: elevation and height spread?

        // Angle of source relative to listener in XZ plane
        float azimuth = VectorAngle(glm::normalize(relativePos));

        //HZ_CORE_INFO_TAG("Sound Spatializer", "Relative Pos: X {0} | Y {1} | Z {2}", relativePos.x, relativePos.y, relativePos.z);

        //HZ_CORE_TRACE_TAG("Sound Spatializer", "Angle degrees: {}", glm::degrees(azimuth));

        source.Distance = distance;
        source.vbapAzimuth = azimuth;
        source.PositionRelative = relativePos;
        source.Transform.Position = position;
        source.Transform.Orientation = orientation;
        source.Transform.Up = sourceUp;
        source.RelativeDir = relativeDir;
        source.Velocity = velocity;

        if (source.SpatializationConfig->bSpreadFromSourceSize)
            source.Spread = GetSpreadFromSourceSize(source.SpatializationConfig->SourceSize, distance);

        UpdatePositionalData(source, &m_Engine->listeners[0]);
        UpdateVBAP(source, !source.bInitialPositionSet);

        // Now that the initial gain values have been set, we can start the audio callback
        if (!source.bInitialPositionSet)
        {
            ma_node_set_state(&source.SpatializerNode, ma_node_state_started);
            source.bInitialPositionSet = true;
        }

        FlagRealtimeForUpdate(source); // TODO: mark dirty in bulk?
    }

    void Spatializer::UpdateListener(const Audio::Transform& transform, glm::vec3 velocity)
    {
        m_ListenerTransform = transform;
        m_ListenerVelocity = velocity;

        const auto& lp = m_ListenerTransform.Position;
        const auto& lr = m_ListenerTransform.Orientation;
        const auto& lup = m_ListenerTransform.Up;

        // Need to update sources when the listener position changed

        for (auto& [sourceID, source] : m_Sources)
        {
            const auto& sp = source.Transform.Position;
            const auto& sr = source.Transform.Orientation;
            const auto& sup = source.Transform.Up;

            // Position of the source ralative to listener
            const glm::mat4 lookatM = glm::lookAt(lp, lp + lr, lup);
            glm::vec3 relativePos = lookatM * glm::vec4(sp, 1.0f);

            // Direction from the source to listener
            const glm::mat4 lookatMR = glm::lookAt(sp, sp + sr, sup);
            glm::vec3 relativeDir = glm::normalize(lookatMR * glm::vec4(lp, 1.0f));

            const float distance = glm::length(relativePos);

            // TODO: elevation and height spread?

            // Angle of source relative to listener in XZ plane
            float azimuth = VectorAngle(glm::normalize(relativePos));

            //HZ_CORE_TRACE_TAG("Sound Spatializer", "Angle degrees: {}", glm::degrees(azimuth));

            source.Distance = distance;
            source.vbapAzimuth = azimuth;
            source.PositionRelative = relativePos;
            source.RelativeDir = relativeDir;

            if (source.SpatializationConfig->bSpreadFromSourceSize)
                source.Spread = GetSpreadFromSourceSize(source.SpatializationConfig->SourceSize, distance);
        }

        std::for_each(std::execution::par_unseq, m_Sources.begin(), m_Sources.end(), [&](std::pair<const uint32_t, Source>& IDSourcePair) 
            {
                auto& source = IDSourcePair.second;
                UpdatePositionalData(source, &m_Engine->listeners[0]);
                UpdateVBAP(source);
                FlagRealtimeForUpdate(source);
            });
    }
}
