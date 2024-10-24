#pragma once
#include "miniaudio/include/miniaudio_incl.h"
#include "farbot/RealtimeObject.hpp"

namespace Engine::Audio::DSP
{
    using ChannelGains = std::array<float, MA_MAX_CHANNELS>;

    // Thread-safe object to update VBAP gains from non-realtime thread and read from realtime thread.
    using RealtimeGains = farbot::RealtimeObject<ChannelGains, farbot::RealtimeObjectOptions::nonRealtimeMutatable>;

    struct VBAPData;

    class VBAP
    {
    public:
        struct PositionUpdateData
        {
            float PanAngle;
            float Spread;
            float Focus;
            float GainAttenuation;
        };

        struct VirtualSource
        {
            float Angle;                                                // Absolute angle of this virtual source in radians.
            uint32_t Channel;                                           // Index of the channel this virtual source is associated to. Mainly for debugging.
            uint32_t NumOutputChannels;                 //? number of output channels, could probably use this later to initialize the Array of gains to this number
            ChannelGains Gains;                                         // Gains this virtual source contributing to each output channel.

            VirtualSource();
            VirtualSource(uint32_t numberOfOutputChannels, float angle = 0.0f);
        };

        struct ChannelGroup
        {
            float Angle;                                                // Angle of the output channel this group is associated to. In radians.
            uint32_t Channel;                                           // Index of the channel this group is associated to. Mainly for debugging.
            std::vector<int> VirtualSourceIDs;                          // Virtual sources associated to this channel group

            RealtimeGains Gains;                                        // Accumulated and normalized gains of virtual sources associated to this channel group.
            ma_gainer Gainer;                                           // Interpolating changes in gain for the virtual sources of the group.

            ChannelGroup();
            ChannelGroup(uint32_t numberOfOutputChannels, uint32_t channel, float angle = 0.0f);
            ChannelGroup(const ChannelGroup& other);
            ChannelGroup& operator= (const ChannelGroup& other);
        };

    public:
        static bool InitVBAP(VBAPData* vbap, const uint32_t numOfInputs, const uint32_t numOfOutputs, const ma_channel* sourceChannelMap, const ma_channel* outPutChannelMap);
        static void ClearVBAP(VBAPData* vbap);

        // Update gains each Virtual Source contributing to the output channels based on the new positional data
        static void UpdateVBAP(VBAPData* vbap, const PositionUpdateData& positionData, const ma_channel_converter& converter, bool isInitialPosition = false);

    private:
        // Sort speaker vectors in ascending order preparing for FindActiveArch()
        static void SortChannelLayout(const std::vector<glm::vec2>& speakerVectors, std::vector<std::pair<glm::vec2, uint32_t>>& sorted);

        /*  Calculate channel gains based on source direction
			@param azimuthRadians - direction of the source in speaker plane
			@param gains - output gains
		 */
        static void ProcessVBAP(const VBAPData* vbap, const float azimuthRadians, ChannelGains& gains);
        
        // Find speaker pair for the source direction
        static bool FindActiveArch(const VBAPData* vbap, const float azimuthRadians, std::pair<int, int>& outSpekerIndexes, std::pair<float, float>& outGains);

        // Convert speaker gains from internal format to the output format. Foramat and weights specified in converter
        static ChannelGains ConvertChannelGains(const ChannelGains& channelGainsIn, const ma_channel_converter& converter);
    };

    // Data needed to calculate and apply VBAP gains
    struct VBAPData
    {
        std::vector<glm::vec2> spPos;                               // Intermediary output speaker positions in channel map order
        std::vector<std::pair<glm::vec2, uint32_t>> spPosSorted;    // Intermediary output speaker positions sorted by vector angle

        std::vector<glm::mat2> InverseMats;                         // Matrices to find active pair of Intermediary output speakers for a given source vector

        std::vector<VBAP::VirtualSource> VirtualSources;            // VBAP sources distributed in 360 degrees and assigned to input channel groups
        std::vector<VBAP::ChannelGroup> ChannelGroups;              // Groups of virtual sources associated with input channels
    };


    // Calculate angle in XZ where -Z is forward direction
    static inline float VectorAngle(const glm::vec3& vec)
    {
        glm::vec3 norm = glm::normalize(vec);
        float x = norm.x;
        float y = norm.z;

        // Special cases
        if (x == 0.0f)
            return glm::radians((y < 0.0f) ? 0.0f
                : (y == 0.0f) ? 0.0f
                : 180.0f);
        else if (y == 0.0f)
            return glm::radians((x > 0.0f) ? 90.0f
                : x < 0.0f ? -90.0f
                : 0.0f);

        float ret = glm::degrees(atanf((float)x / y));

        if (x < 0.0f && y < 0.0f)   // IV
            ret *= -1.0f;
        else if (x < 0.0f)          // III
            ret = (180.0f + ret) * -1.0f;
        else if (y < 0)             // I
            ret *= -1.0f;
        else
            ret = 180.0f - ret;

        return glm::radians(ret);
    }

    static inline float VectorAngle(const glm::vec2& vec)
    {
        glm::vec2 norm = glm::normalize(vec);
        float x = norm.x;
        float y = norm.y;

        // Spacial cases
        if (x == 0.0f)
            return glm::radians((y < 0.0f) ? 0.0f
                : (y == 0.0f) ? 0.0f
                : 180.0f);
        else if (y == 0.0f)
            return glm::radians((x > 0.0f) ? 90.0f
                : x < 0.0f ? -90.0f
                : 0.0f);

        float ret = glm::degrees(atanf((float)x / y));

        if (x < 0.0f && y < 0.0f)   // IV
            ret *= -1.0f;
        else if (x < 0.0f)          // III
            ret = (180.0f + ret) * -1.0f;
        else if (y < 0)             // I
            ret *= -1.0f;
        else
            ret = 180.0f - ret;

        return glm::radians(ret);
    }


} // Engine::Audio::DSP
