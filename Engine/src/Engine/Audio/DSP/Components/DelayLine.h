#pragma once

#include <vector>
#include <cassert>
#include <algorithm>
#include <cmath>

#include "Engine/Audio/SourceManager.h"


namespace Engine::Audio::DSP
{

class DelayLine
{
public:
    explicit DelayLine(int maximumDelayInSamples = 0)
    {
        assert(maximumDelayInSamples >= 0);

        m_TotalSize = std::max(4, maximumDelayInSamples + 1);
        m_SampleRate = 44100.0;
    }

    void SetDelay(float newDelayInSamples)
    {
        auto upperLimit = (float)(m_TotalSize - 1);

        //? for now just setting delay to 1 sample, in the future might make sense to completely bypass it
        //? if delay time set to 0 by the user
        if (newDelayInSamples <= 0) newDelayInSamples = 1;
        assert(newDelayInSamples > 0 && newDelayInSamples < upperLimit);
        
        m_Delay = std::clamp(newDelayInSamples, (float)0, upperLimit);
        m_DelayInt = (int)std::floor(m_Delay);
    }
    void SetDelayMs(uint32_t milliseconds)
    { 
        SetDelay(float((double)milliseconds / 1000.0 * m_SampleRate));
    }

    float GetDelay() const { return m_Delay; }
    uint32_t GetDelayMs() const { return uint32_t(m_Delay / m_SampleRate * 1000.0); }

    double GetSampleRate() { return m_SampleRate; }

    void SetConfig(uint32_t numChannels, double sampleRate)
    {
        assert(numChannels > 0);

        m_BufferData.resize(numChannels);
        for (auto& ch : m_BufferData)
        {
            ch.resize(m_TotalSize);
            std::fill(ch.begin(), ch.end(), 0.0f);
        }

        m_WritePos.resize(numChannels);
        m_ReadPos.resize(numChannels);

        m_SampleRate = sampleRate;

        Reset();
    }

    void Reset()
    {
        for (auto vec : { &m_WritePos, &m_ReadPos })
            std::fill(vec->begin(), vec->end(), 0);

        for(auto& ch : m_BufferData)
            std::fill(ch.begin(), ch.end(), 0.0f);
    }

    void PushSample(int channel, float sample)
    {
        m_BufferData.at(channel).at(m_WritePos[(size_t)channel]) = sample;
        m_WritePos[(size_t)channel] = (m_WritePos[(size_t)channel] + m_TotalSize - 1) % m_TotalSize;
    }
    float PopSample(int channel, float delayInSamples = -1, bool updateReadPointer = true)
    {
        if (delayInSamples >= 0)
            SetDelay(delayInSamples);

        float result = [&, channel]() { auto index = (m_ReadPos[(size_t)channel] + m_DelayInt) % m_TotalSize;
                                        return m_BufferData.at(channel).at(index); }();

        if (updateReadPointer)
            m_ReadPos[(size_t)channel] = (m_ReadPos[(size_t)channel] + m_TotalSize - 1) % m_TotalSize;

        return result;
    }

private:
    double m_SampleRate;
    std::vector<std::vector<float, SourceManager::Allocator<float>>> m_BufferData;
    std::vector<int> m_WritePos, m_ReadPos;
    float m_Delay = 0.0;
    int m_DelayInt = 0, m_TotalSize = 4;
};

} // namespace Engine::Audio::DSP