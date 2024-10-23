#include <pch.h>
#include "FilterHighPass.h"

namespace Hazel::Audio::DSP
{

    void hpf_node_process_pcm_frames(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
    {
        const float* pFramesIn_0 = ppFramesIn[0]; // Input bus @ index 0.
        const float* pFramesIn_1 = ppFramesIn[1]; // Input bus @ index 1.
        float* pFramesOut_0 = ppFramesOut[0];     // Output bus @ index 0.

        auto* node = static_cast<HighPassFilter::hpf_node*>(pNode);

        ma_hpf2_process_pcm_frames(&node->filter, pFramesOut_0, pFramesIn_0, *pFrameCountOut);
    }
        
    // TODO: use second input bus for Bypass functionality?
    //       Or it can be used for a modulator signal!
    static ma_node_vtable high_pass_filter_vtable = {
        hpf_node_process_pcm_frames,
        nullptr,
        2, // 2 input buses.
        1, // 1 output bus.
        0 // Default flags.
    };


    //===========================================================================
    static ma_allocation_callbacks allocation_callbacks;

    HighPassFilter::HighPassFilter()
    {
    }

    HighPassFilter::~HighPassFilter()
    {
        Uninitialize();
    }

    void HighPassFilter::Uninitialize()
    {
        if (m_Initialized)
        {
            if (((ma_node_base*)&m_Node)->vtable != nullptr)
                ma_node_uninit(&m_Node, &allocation_callbacks);
        }
     
        m_Initialized = false;
    }

    bool HighPassFilter::Initialize(ma_engine* engine, ma_node_base* nodeToInsertAfter)
    {
        ZONG_CORE_ASSERT(!m_Initialized);

        auto abortIfFailed = [&](ma_result result, const char* errorMessage) 
        {
            if (result != MA_SUCCESS)
            {
                ZONG_CORE_ASSERT(false && errorMessage);
                Uninitialize();
                return true;
            }

            return false;
        };

        m_SampleRate = ma_engine_get_sample_rate(engine);;
        uint8_t numChannels = ma_node_get_output_channels(nodeToInsertAfter, 0);
            
        ma_uint32 inChannels[2]{ numChannels, numChannels };
        ma_uint32 outChannels[1]{ numChannels };
        ma_node_config nodeConfig = ma_node_config_init();
        nodeConfig.vtable = &high_pass_filter_vtable;
        nodeConfig.pInputChannels = inChannels;
        nodeConfig.pOutputChannels = outChannels;
        nodeConfig.initialState = ma_node_state_started;

        ma_result result;
        allocation_callbacks = engine->pResourceManager->config.allocationCallbacks;
        result = ma_node_init(&engine->nodeGraph, &nodeConfig, &allocation_callbacks, &m_Node);
        if(abortIfFailed(result,"Node Init failed"))
            return false;

        ma_hpf2_config config = ma_hpf2_config_init(ma_format_f32, numChannels, (ma_uint32)m_SampleRate, m_CutoffMultiplier.load(), 0.707);
        result = ma_hpf2_init(&config, &m_Node.filter);
        if(abortIfFailed(result,"Filter Init failed"))
            return false;

        auto* output = nodeToInsertAfter->pOutputBuses[0].pInputNode;

        // attach to the output of the node that this filter is connected to
        result = ma_node_attach_output_bus(&m_Node, 0, output, 0);
        ZONG_CORE_ASSERT(result == MA_SUCCESS);
        if (abortIfFailed(result, "Node attach failed"))
            return false;

        // attach passed in node to the filter
        result = ma_node_attach_output_bus(nodeToInsertAfter, 0, &m_Node, 0);
        if (abortIfFailed(result, "Node attach failed"))
            return false;

        m_Initialized = true;

        return m_Initialized;
    }


	void HighPassFilter::SetCutoffFrequency(double frequency)
	{
		if (m_CutoffMultiplier == frequency)
			return;

		m_CutoffMultiplier = frequency;

		ma_hpf2_config config = ma_hpf2_config_init(m_Node.filter.bq.format, m_Node.filter.bq.channels, (ma_uint32)m_SampleRate, frequency, 0.707);

		// TODO: make sure this thread safe, e.i. using atomic operation to update the values
		ma_hpf2_reinit(&config, &m_Node.filter);
	}

    // TODO: check for the "zipper" noise artefacts
    void HighPassFilter::SetCutoffValue(double cutoffMultiplier)
    {
        double cutoffFrequency = cutoffMultiplier * 20000.0;
		if (m_CutoffMultiplier == cutoffFrequency)
			return;

		m_CutoffMultiplier = cutoffFrequency;

        ma_hpf2_config config = ma_hpf2_config_init(m_Node.filter.bq.format, m_Node.filter.bq.channels, (ma_uint32)m_SampleRate, cutoffFrequency, 0.707);

        // TODO: make sure this thread safe, e.i. using atomic operation to update the values
        ma_hpf2_reinit(&config, &m_Node.filter);
    }

    void HighPassFilter::SetParameter(uint8_t parameterIdx, float value)
    {
        if (parameterIdx == EHPFParameters::CutOffFrequency)
        {
            m_CutoffMultiplier = (double)value;
            SetCutoffValue(m_CutoffMultiplier.load()); // TODO: call this from update loop
        }
    }

    float HighPassFilter::GetParameter(uint8_t parameterIdx) const
    {
        if (parameterIdx == EHPFParameters::CutOffFrequency)
            return (float)m_CutoffMultiplier.load();
        else
            return -1.0f;
    }

    const char* HighPassFilter::GetParameterLabel(uint8_t parameterIdx) const
    {
        if (parameterIdx == EHPFParameters::CutOffFrequency)
            return "Hz";
        else
            return "Unkonw parameter index";
    }

    std::string HighPassFilter::GetParameterDisplay(uint8_t parameterIdx) const
    {
        if (parameterIdx == EHPFParameters::CutOffFrequency)
            return std::to_string(m_CutoffMultiplier.load());
        else
            return "Unkonw parameter index";
    }

    const char* HighPassFilter::GetParameterName(uint8_t parameterIdx) const
    {
        if (parameterIdx == EHPFParameters::CutOffFrequency)
            return "Cut-Off Frequency";
        else
            return "Unkonw parameter index";
    }

    uint8_t HighPassFilter::GetNumberOfParameters() const
    {
        return 1;
    }

} // namespace Hazel::Audio::DSP
