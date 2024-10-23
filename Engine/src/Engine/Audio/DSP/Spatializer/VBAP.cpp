#include <hzpch.h>
#include "VBAP.h"

#include "Engine/Audio/Audio.h"

namespace Hazel::Audio::DSP
{
    constexpr glm::vec3 g_maChannelDirections[MA_CHANNEL_POSITION_COUNT] = {
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_NONE */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_MONO */
                        {-0.7071f,  0.0f,    -0.7071f },  /* MA_CHANNEL_FRONT_LEFT */
                        {+0.7071f,  0.0f,    -0.7071f },  /* MA_CHANNEL_FRONT_RIGHT */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_FRONT_CENTER */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_LFE */
                        {-0.7071f,  0.0f,    +0.7071f },  /* MA_CHANNEL_BACK_LEFT */
                        {+0.7071f,  0.0f,    +0.7071f },  /* MA_CHANNEL_BACK_RIGHT */
                        {-0.3162f,  0.0f,    -0.9487f },  /* MA_CHANNEL_FRONT_LEFT_CENTER */
                        {+0.3162f,  0.0f,    -0.9487f },  /* MA_CHANNEL_FRONT_RIGHT_CENTER */
                        { 0.0f,     0.0f,    +1.0f    },  /* MA_CHANNEL_BACK_CENTER */
                        {-1.0f,     0.0f,     0.0f    },  /* MA_CHANNEL_SIDE_LEFT */
                        {+1.0f,     0.0f,     0.0f    },  /* MA_CHANNEL_SIDE_RIGHT */
                        { 0.0f,    +1.0f,     0.0f    },  /* MA_CHANNEL_TOP_CENTER */
                        {-0.5774f, +0.5774f, -0.5774f },  /* MA_CHANNEL_TOP_FRONT_LEFT */
                        { 0.0f,    +0.7071f, -0.7071f },  /* MA_CHANNEL_TOP_FRONT_CENTER */
                        {+0.5774f, +0.5774f, -0.5774f },  /* MA_CHANNEL_TOP_FRONT_RIGHT */
                        {-0.5774f, +0.5774f, +0.5774f },  /* MA_CHANNEL_TOP_BACK_LEFT */
                        { 0.0f,    +0.7071f, +0.7071f },  /* MA_CHANNEL_TOP_BACK_CENTER */
                        {+0.5774f, +0.5774f, +0.5774f },  /* MA_CHANNEL_TOP_BACK_RIGHT */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_0 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_1 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_2 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_3 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_4 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_5 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_6 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_7 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_8 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_9 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_10 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_11 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_12 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_13 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_14 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_15 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_16 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_17 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_18 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_19 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_20 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_21 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_22 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_23 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_24 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_25 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_26 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_27 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_28 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_29 */
                        { 0.0f,     0.0f,    -1.0f    },  /* MA_CHANNEL_AUX_30 */
                        { 0.0f,     0.0f,    -1.0f    }   /* MA_CHANNEL_AUX_31 */
    };

//==============================================================================
#pragma region VBAP_STRUCTURE_CONSTRUCTORS

    VBAP::VirtualSource::VirtualSource()
        : Angle(0.0f)
        , Channel(0)
        , NumOutputChannels(0)
        , Gains{ 0.0f }
    {
    }

    VBAP::VirtualSource::VirtualSource(uint32_t numberOfOutputChannels, float angle)
        : Angle(angle)
        , Channel(0)
        , NumOutputChannels(numberOfOutputChannels)
        , Gains{ 0.0f }
    {
    }

    VBAP::ChannelGroup::ChannelGroup()
        : Angle(0.0f)
        , Channel(0)
    {
    }

    VBAP::ChannelGroup::ChannelGroup(uint32_t numberOfOutputChannels, uint32_t channel, float angle)
        : Angle(angle)
        , Channel(channel)
    {
        ma_result result;
        uint32_t gainSmoothTimeInFrames = 360; // ~7ms with 48kHz sampling rate
        ma_gainer_config gainerConfig = ma_gainer_config_init(numberOfOutputChannels, gainSmoothTimeInFrames);
        result = ma_gainer_init(&gainerConfig, NULL, &Gainer);
        HZ_CORE_ASSERT(result == MA_SUCCESS);

        // Setting gains to 0.0 to prevent loud volume jump in case initial position was not set.
        // Technically not necessary because we don't start the node until the initial position has been set.
        // But just in case.
        for (uint32_t iChannel = 0; iChannel < Gainer.config.channels; ++iChannel)
        {
            Gainer.pOldGains[iChannel] = 0.0f;
            Gainer.pNewGains[iChannel] = 0.0f;
        }
    }
    VBAP::ChannelGroup::ChannelGroup(const VBAP::ChannelGroup& other)
    {
        Angle = other.Angle;
        Channel = other.Channel;
        VirtualSourceIDs = other.VirtualSourceIDs;
        Gainer = other.Gainer;
    }

    VBAP::ChannelGroup& VBAP::ChannelGroup::operator=(const VBAP::ChannelGroup& other)
    {
        Angle = other.Angle;
        Channel = other.Channel;
        VirtualSourceIDs = other.VirtualSourceIDs;
        Gainer = other.Gainer;

        return *this;
    }

#pragma endregion


    //==============================================================================

    bool VBAP::InitVBAP(VBAPData* vbap, const uint32_t numOfInputs, const uint32_t numOfOutputs, const ma_channel* sourceChannelMap, const ma_channel* outPutChannelMap)
    {
        HZ_CORE_ASSERT(numOfInputs > 0 && numOfOutputs > 0);

        ClearVBAP(vbap);

        std::vector<float> angles;
        angles.resize(numOfOutputs);

        vbap->spPos.resize(numOfOutputs); // this might have to be indices

        // Store speaker vectors
        for (int i = 0; i < (int)numOfOutputs; i++)
        {
            const auto vector = g_maChannelDirections[outPutChannelMap[i]];

            const float angle = VectorAngle(vector);
            angles[i] = angle;
            //HZ_CORE_TRACE("Angle degrees: {}", glm::degrees(angle));

            vbap->spPos[i] = glm::vec2{ vector.x, vector.z };
        }

        // Create sorted list of speaker vectors
        SortChannelLayout(vbap->spPos, vbap->spPosSorted);

        // Store inverse matrices of sorted speaker pairs
        for (int i = 0; i < vbap->spPosSorted.size(); i++)
        {
            auto& [p, idx] = vbap->spPosSorted.at(i);
            auto& [p2, idx2] = vbap->spPosSorted.at((i + 1) % vbap->spPosSorted.size());

            //HZ_CORE_TRACE("Sorted degrees: {}", glm::degrees(VectorAngle({ p.x, 0.0f, p.y })));

            const glm::mat2 L(glm::vec2(p.x, p.y), glm::vec2(p2.x, p2.y));
            vbap->InverseMats.push_back(glm::inverse(L));
        }

        //----------- Virtual Sources ------------

        //? We need to use at least 4 virtual sources, so that there's no less than 90 degrees between them
        const uint32_t numOfVirtualSources = numOfOutputs * 2u;
        const uint32_t vsPerChannel = numOfVirtualSources / numOfInputs;

        const float vsAngle = 360.0f / (float)numOfVirtualSources;
        const float inputPlaneSection = 360.0f / (float)numOfInputs;

        vbap->ChannelGroups.resize(numOfInputs);
        vbap->VirtualSources.reserve(numOfVirtualSources);
        int vsID = 0;

        // Sort input channels by their rotation vectors

        std::vector<glm::vec2> inputChannelsUnsorted;
        inputChannelsUnsorted.reserve(numOfInputs);
        for (uint32_t i = 0; i < numOfInputs; i++)
        {
            const auto vec = g_maChannelDirections[sourceChannelMap[i]];
            inputChannelsUnsorted.push_back(glm::vec2{ vec.x, vec.z });
        }

        std::vector<std::pair<glm::vec2, uint32_t>> inputChannelsSorted;
        inputChannelsSorted.reserve(numOfInputs);
        SortChannelLayout(inputChannelsUnsorted, inputChannelsSorted);

        // If channel count is odd, we need to offset virtual sources so that the first one desn't end up on top of the centre channel
        const bool evenChannelCount = numOfInputs % 2 == 0;
        float sourceAngle = -0.5f * vsAngle;

        // 1. Find equal input channel positions for 100% spread
        // 2. Lay out virtual sources for input channel group evenly withing its equal section

        for (uint32_t i = 0; i < numOfInputs; i++)
        {
            auto& [pos, id] = inputChannelsSorted[i];

            const glm::vec2& channelVector = inputChannelsUnsorted[id];

            // Assign a centre angle of the next equal section of the input plane
            float channelAngle = inputPlaneSection * i;
            if (evenChannelCount)
                channelAngle += 0.5f * inputPlaneSection;

            // We are using 0 to 180 and 0 to -180 angles
            if (channelAngle > 180.0f)
                channelAngle -= 360.0f;

            // Create channel group for each input channel
            ChannelGroup channelGroup(numOfOutputs, id, channelAngle);
            channelGroup.VirtualSourceIDs.reserve(vsPerChannel);

            //? DBG. Input channel angle
            /*const auto an = g_maChannelDirections[m_SurrChannelMap[i]];
              const float originalAngle = glm::degrees(VectorAngle(g_maChannelDirections[m_SurrChannelMap[id]]));
              HZ_CORE_TRACE("-----------------------------");
              HZ_CORE_TRACE("Channel GRP ID {0}, angle: {1}, original angle {2}", id, channelAngle, originalAngle);*/

            for (uint32_t vi = 0; vi < vsPerChannel; vi++)
            {
                sourceAngle += vsAngle;
                float sa = sourceAngle;

                // We are using 0 to 180 and 0 to -180 angles
                if (sa > 180.0f)
                    sa = -(sa - 180.0f); // -= 360.0f;

                VirtualSource virtualSource(numOfOutputs, sa);
                virtualSource.Channel = id;

                vbap->VirtualSources.push_back(virtualSource);


                channelGroup.VirtualSourceIDs.push_back(vsID);
                vsID++;

                //? DBG. Virtual Source angle
                /*if (channelAngle < 0.0f)
                        HZ_CORE_TRACE("Left Channel VS: channelAngle {0}, vsAngle {1}", channelAngle, sa);
                    else
                        HZ_CORE_TRACE("Right Channel VS: channelAngle {0}, vsAngle {1}", channelAngle, sa);*/

            }

            vbap->ChannelGroups[id] = channelGroup;
        }

        return true;
    }

    void VBAP::ClearVBAP(VBAPData* vbap)
    {
        HZ_CORE_ASSERT(vbap != nullptr);
        *vbap = VBAPData();

        //vbap.spPos.clear();
        //vbap.spPosSorted.clear();
        //vbap.InverseMats.clear();
        //vbap.VirtualSources.clear();
        //vbap.ChannelGroups.clear();
    }

    void VBAP::UpdateVBAP(VBAPData* vbap, const PositionUpdateData& positionData, const ma_channel_converter& converter, bool isInitialPosition /*= false*/)
    {
        HZ_CORE_ASSERT(!vbap->VirtualSources.empty());

        const float panAngle = positionData.PanAngle;
        const float spread = positionData.Spread;
        const float focus = positionData.Focus;
        const float gainAttenuation = positionData.GainAttenuation;
        

        //===== Calculate Channel Panning Gains ======

        for (auto& vs : vbap->VirtualSources)
        {
            float channelGroupAngle = vbap->ChannelGroups[vs.Channel].Angle;

            // Calculate angle based on Focus and Spread values
            const float vsAngle = glm::radians(Lerp(vs.Angle, channelGroupAngle, focus) * spread);

            // Calculate VBAP channel gains based on source direction + virtual source offset angle
            ProcessVBAP(vbap, panAngle + vsAngle, vs.Gains);

            // TODO: handle Source Orientation somehow
            //       Probably should add a user options to chose spatialization base on just position, or position + orientation
        }


        //===== Normalize and Apply Gains ======

        for (auto& chg : vbap->ChannelGroups)
        {
            ChannelGains gainsLocal;//
            ma_silence_pcm_frames(gainsLocal.data(), MA_MAX_CHANNELS, ma_format_f32, 1);

            for (auto& vsID : chg.VirtualSourceIDs)
            {
                auto& vsGains = vbap->VirtualSources[vsID].Gains;

                size_t i = 0;
                for (auto& g : gainsLocal)
                {
                    g += vsGains[i] * vsGains[i];
                    i++;
                }
            }

            // Applying gain compensation for the number of contributing virtual sources
            const uint32_t numVirtualSources = (uint32_t)chg.VirtualSourceIDs.size();
            std::for_each(gainsLocal.begin(), gainsLocal.end(),
                [numVirtualSources, gainAttenuation](float& g) { g = std::sqrt(g / numVirtualSources) * gainAttenuation; });

            // Convert intermediate Surround channel gains to Stereo output
            ChannelGains gainsOut = ConvertChannelGains(gainsLocal, converter);
            farbot::RealtimeObject<ChannelGains, farbot::RealtimeObjectOptions::nonRealtimeMutatable>::ScopedAccess<farbot::ThreadType::nonRealtime> gains(chg.Gains);
            *gains = gainsOut;

            // If initial position hasn't been set yet, no need for interpolation, set "old" gains to the current ones
            if (isInitialPosition)
            {
                memcpy(chg.Gainer.pOldGains, gainsOut.data(), sizeof(float) * chg.Gainer.config.channels);
            }
        }
    }

    void VBAP::SortChannelLayout(const std::vector<glm::vec2>& speakerVectors, std::vector<std::pair<glm::vec2, uint32_t>>& sorted)
    {
        for (int i = 0; i < speakerVectors.size(); i++)
        {
            sorted.push_back({ speakerVectors.at(i), i });
        }

        std::sort(sorted.begin(), sorted.end(), [](const auto& p1, const auto& p2)
            {
                float ang1 = VectorAngle({ p1.first.x, p1.first.y });
                float ang2 = VectorAngle({ p2.first.x, p2.first.y });
                constexpr float circle = glm::radians(360.0f);
                if (ang1 < 0.0f) ang1 = circle + ang1;
                if (ang2 < 0.0f) ang2 = circle + ang2;

                return ang1 < ang2;
            });
    }

    ChannelGains VBAP::ConvertChannelGains(const ChannelGains& channelGainsIn, const ma_channel_converter& converter)
    {
        ChannelGains channelGainsOut{ 0.0f };

        float* pFramesOutF32 = channelGainsOut.data();
        const float* pFramesInF32 = channelGainsIn.data();

        if (converter.isPassthrough)
        {
            memcpy(pFramesOutF32, pFramesInF32, sizeof(float) * channelGainsIn.size());
        }
        else if (converter.isSimpleShuffle)
        {
            for (ma_uint32 iChannelIn = 0; iChannelIn < converter.channelsIn; ++iChannelIn)
                pFramesOutF32[converter.shuffleTable[iChannelIn]] = pFramesInF32[iChannelIn];
        }
        else if (converter.isSimpleMonoExpansion) {
            if (converter.channelsOut == 2)
            {
                pFramesOutF32[0] = pFramesInF32[0];
                pFramesOutF32[1] = pFramesInF32[0];
            }
            else
            {
                for (ma_uint32 iChannel = 0; iChannel < converter.channelsOut; ++iChannel)
                    pFramesOutF32[iChannel] = pFramesInF32[0];
            }
        }
        else if (converter.isStereoToMono)
        {
            pFramesOutF32[0] = (pFramesInF32[0] + pFramesInF32[1]) * 0.5f;
        }
        else // Weights
        {
            for (ma_uint32 iChannelIn = 0; iChannelIn < converter.channelsIn; ++iChannelIn)
            {
                for (ma_uint32 iChannelOut = 0; iChannelOut < converter.channelsOut; ++iChannelOut)
                {
                    const auto w = converter.weights.f32[iChannelIn][iChannelOut];
                    pFramesOutF32[iChannelOut] += pFramesInF32[iChannelIn] * w;
                }
            }
        }

        return channelGainsOut;
    }

    void VBAP::ProcessVBAP(const VBAPData* vbap, const float azimuthRadians, ChannelGains& gains)
    {
        std::pair<int, int> speakers;
        std::pair<float, float> speakerGains;
        if (FindActiveArch(vbap, azimuthRadians, speakers, speakerGains))
        {
            std::fill(gains.begin(), gains.end(), 0.0f);
            gains[speakers.first] = speakerGains.first;
            gains[speakers.second] = speakerGains.second;
        }
        else
        {
            //HZ_CONSOLE_LOG_ERROR("Faild to find Active Arch.");
        }
    }

    bool VBAP::FindActiveArch(const VBAPData* vbap, const float azimuthRadians, std::pair<int, int>& outSpekerIndexes, std::pair<float, float>& outGains)
    {
        float powr, ref = 0.0f;
        float g1 = 0.0f, g2 = 0.0f;
        int idx1 = -1, idx2 = -1;

        const float x = sinf(azimuthRadians);
        const float y = -cosf(azimuthRadians);

        const auto& sortedPositions = vbap->spPosSorted;
        const auto& inverseMats = vbap->InverseMats;
        const auto size = sortedPositions.size();

        for (size_t i = 0; i < size; i++)
        {
            const auto& [pos, ind] = sortedPositions[i];
            const auto& [pos2, ind2] = sortedPositions[(i + 1) % size];

            // Speaker gains
            //glm::vec2 g;
            float gi1, gi2;

            // This inverse matrix already contains vectors of current and next sorted position
            const glm::mat2& Li = inverseMats[i];
            //g = Li * glm::vec2(x, y);

            gi1 = Li[0][0] * x + Li[1][0] * y;
            gi2 = Li[0][1] * x + Li[1][1] * y;


            // Check if all gains are positive -> that's the speaker pair we need
            if (gi1 > -1e-6f && gi2 > -1e-6f)
            {
                powr = sqrtf(gi1 * gi1 + gi2 * gi2);
                if (powr > ref)
                {
                    ref = powr;
                    idx1 = ind;
                    idx2 = ind2;

                    gi1 = std::max(gi1, 0.0f);
                    gi2 = std::max(gi2, 0.0f);

                    // Normilize gains to RMS
                    g1 = gi1 / powr;
                    g2 = gi2 / powr;
                }
            }
        }

        outSpekerIndexes = { idx1, idx2 };
        outGains = { g1, g2 };

        return ref != 0.0f;
    }

} // namespace Hazel::Audio::DSP
