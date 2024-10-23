#pragma once

#include "miniaudio/include/miniaudio_incl.h"
#include "Engine/Audio/Audio.h"
#include "VBAP.h"

#include <glm/glm.hpp>

namespace Hazel
{
    struct SpatializationConfig;
}

namespace Hazel::Audio::DSP
{
    struct VBAPData;

    /*  ====================
        3D Sound Spatializer
        ---------------------
    */
    class Spatializer
    {
    public:
        Spatializer() = default;
        ~Spatializer();

		/*  Audio callback is going to be stopped until initial position is set with the next SetPositionRotation() call.
			This is done to prevent volume spike at the beginning of playback if it's started before initial panning gains have been calculated.
		*/
        bool Initialize(ma_engine* engine);
        void Uninitialize();

        bool IsInitialized(uint32_t sourceID) const;
        float GetCurrentDistanceAttenuation(uint32_t sourceID) const;
        float GetCurrentConeAngleAttenuation(uint32_t sourceID) const;
        float GetCurrentDistance(uint32_t sourceID) const;

        //  Spatialied Sources
        //============================================================================
        bool InitSource(uint32_t sourceID, ma_engine_node* nodeToInsertAfter, const Ref<SpatializationConfig>& config);
        bool ReleaseSource(uint32_t sourceID);
        void UpdateSourcePosition(uint32_t sourceID, const Audio::Transform& position, glm::vec3 velocity = { 0.0f, 0.0f, 0.0f });
        void SetSpread(uint32_t sourceID, float newSpread);
        void SetFocus(uint32_t sourceID, float newFocus);

        void UpdateListener(const Audio::Transform& transform, glm::vec3 velocity = { 0.0f, 0.0f, 0.0f });

    private:
        struct Source;

        static float GetSpreadFromSourceSize(float sourceSize, float distance);

        // Updates attenuation values
        static void UpdatePositionalData(Source& source, const ma_spatializer_listener* listener);

        // Update VBAP channel gains for changed source directions
        static void UpdateVBAP(Source& source, bool isInitialPosition = false);

        // Flag channel gains dirty so that realtime thread can grab new values
        static void FlagRealtimeForUpdate(Source& source);

    private:
        friend void spatializer_node_process_pcm_frames(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn,
                                                                              float** ppFramesOut, ma_uint32* pFrameCountOut);
        
        ma_engine* m_Engine = nullptr;

        struct spatializer_node
        {
            ma_node_base base;
            uint32_t channelsIn;
            uint32_t channelsOut;
            ma_engine_node* targetEngineNode = nullptr;

            Scope<VBAPData> vbap = nullptr;

            // This accessed from audio callback to set Miniaudio's spatializer doppler pitch
            std::atomic<float> DopplerPitch;

            std::atomic_flag fGainsUpToDate;
            bool IsDirty() { return !fGainsUpToDate.test_and_set(); }

            spatializer_node() = default;
            spatializer_node(const spatializer_node&);
        };

		struct Source
		{
            // Static data
            //------------
            uint32_t SourceID;                  // ID of the sound source this Source is associated to
            bool bInitialized = false;
            bool bInitialPositionSet = false;
            uint32_t InternalChannelCount = 4;  // Number of virtual speakers used to calculate VBAP gains

            spatializer_node SpatializerNode;
            ma_channel_converter Converter;
            ma_channel InternalChannelMap[MA_MAX_CHANNELS];
            ma_channel SourceChannelMap[MA_MAX_CHANNELS];
            Ref<SpatializationConfig> SpatializationConfig;

            // Dynamic data
            //-------------
            float Spread;
            float Focus;

            float vbapAzimuth;                // Direction angle in XZ plane used to calculate VBAP gains
            float Distance;                   // Distance from the source to Listener
            float DistanceAttenuationFactor;  // Current distance attenuation factor
            float AngleAttenuationFactor;     // Current cone angle attenuation factor
            glm::vec3 PositionRelative;       // Position relative to Listener
            Audio::Transform Transform;       // Absolute position, orientation and up vector
            glm::vec3 RelativeDir;            // Direction of Listener relative to the Source

            glm::vec3 Velocity = glm::vec3(0.0f, 0.0f, 0.0f);   // For doppler pitch calculation.
		};

		std::unordered_map<uint32_t, Source> m_Sources;

        // Listener
        Audio::Transform m_ListenerTransform;
        glm::vec3 m_ListenerVelocity;
	};

} // namespace Hazel::Audio::DSP

