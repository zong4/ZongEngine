#pragma once

#include "miniaudio_incl.h"

namespace Engine::Audio::DSP
{
    struct LowPassFilter
    {
    public:
        LowPassFilter();
        ~LowPassFilter();

        enum ELPFParameters : uint8_t
        {
            CutOffFrequency
        };

        bool Initialize(ma_engine* engine, ma_node_base* nodeToInsertAfter);
        void Uninitialize();
		void SetCutoffFrequency(double frequency);
        void SetCutoffValue(double cutoffMultiplier);

        ma_node_base* GetNode() { return &m_Node.base; }

        void SetParameter(uint8_t parameterIdx, float value);
        float GetParameter(uint8_t parameterIdx) const;
        const char* GetParameterLabel(uint8_t parameterIdx) const;
        std::string GetParameterDisplay(uint8_t parameterIdx) const;
        const char* GetParameterName(uint8_t parameterIdx) const;
        uint8_t GetNumberOfParameters() const;

    private:
        // --- Internal members
        bool m_Initialized = false;

        friend void lpf_node_process_pcm_frames(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn,
                                                                        float** ppFramesOut, ma_uint32* pFrameCountOut);
        struct lpf_node
        {
            ma_node_base base; // <-- Make sure this is always the first member.
            ma_lpf1 filter;
        };

        lpf_node m_Node;
        double m_SampleRate = 0.0;
        // ~ End of internal members

        std::atomic<double> m_CutoffMultiplier = 22000.0;
    };

} // namespace Engine::Audio::DSP
