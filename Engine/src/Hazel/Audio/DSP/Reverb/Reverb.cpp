#include <hzpch.h>

#include "Reverb.h"

#include "Hazel/Audio/DSP/Components/revmodel.hpp"
#include "Hazel/Audio/DSP/Components/DelayLine.h"

namespace Hazel::Audio::DSP
{

void reverb_node_process_pcm_frames(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
{    
    Reverb::reverb_node* node = static_cast<Reverb::reverb_node*>(pNode);
    ma_node_base* pNodeBase = &node->base;
    ma_uint32 outBuses = ma_node_get_output_bus_count(pNodeBase);

    // If we don't have any input connected we need too clear the output buffer from garbage and to no do any processing
    if (ppFramesIn == nullptr)
    {
        for (ma_uint32 oBus = 0; oBus < outBuses; ++oBus)
            ma_silence_pcm_frames(ppFramesOut[oBus], *pFrameCountOut, ma_format_f32, ma_node_get_output_channels(pNodeBase, oBus));
        return;
    }


    const float* pFramesIn_0 = ppFramesIn[0];   // Input bus @ index 0.
    const float* pFramesIn_1 = ppFramesIn[1];   // Input bus @ index 1.
    float* pFramesOut_0 = ppFramesOut[0];       // Output bus @ index 0.
 
    ma_uint32 channels;

    /*? NOTE: This assumes the same number of channels for all inputs and outputs. */
    channels = ma_node_get_input_channels(pNodeBase, 0);
    HZ_CORE_ASSERT(channels == 2);

    // 1. Feed Pre-Delay
    // TODO: move this stuff to DelayLine class
    auto* delay = node->delayLine;

    const float* channelDataL = &pFramesIn_0[0];
    const float* channelDataR = &pFramesIn_0[1];
    float* outputL = &pFramesOut_0[0];
    float* outputR = &pFramesOut_0[1];

    const float wetMix = 1.0f;
    const float dryMix = 1.0f - wetMix;
    const float feedback = 0.0f;

    int numsamples = *pFrameCountIn;

    while (numsamples-- > 0)
    {
        float delayedSampleL = delay->PopSample(0);
        float delayedSampleR = delay->PopSample(1);
        float outputSampleL = (*channelDataL * dryMix + (wetMix * delayedSampleL));
        float outputSampleR = (*channelDataR * dryMix + (wetMix * delayedSampleR));
        delay->PushSample(0, *channelDataL + (delayedSampleL * feedback));
        delay->PushSample(1, *channelDataR + (delayedSampleR * feedback));

        // Assign output sample computed above to the output buffer
        *outputL = outputSampleL;
        *outputR = outputSampleR;
        channelDataL += channels;
        channelDataR += channels;
        outputL += channels;
        outputR += channels;
    }

    // 2. Process delayed signal with reverb
    node->reverb->processreplace(pFramesOut_0, &pFramesOut_0[1], pFramesOut_0, &pFramesOut_0[1], *pFrameCountIn, channels);
}

static ma_node_vtable reverb_vtable = {
    reverb_node_process_pcm_frames,
    nullptr,
    2, // 1 input bus.
    1, // 1 output bus.
    MA_NODE_FLAG_CONTINUOUS_PROCESSING | MA_NODE_FLAG_ALLOW_NULL_INPUT
};


//===========================================================================
static ma_allocation_callbacks allocation_callbacks;

Reverb::Reverb() {}

Reverb::~Reverb()
{
    Uninitialize();
}

void Reverb::Uninitialize()
{
    if (m_Initialized)
    {
        if (((ma_node_base*)&m_Node)->vtable != nullptr)
            ma_node_uninit(&m_Node, &allocation_callbacks);
    }
    m_Initialized = false;
}

bool Reverb::Initialize(ma_engine* engine, ma_node_base* nodeToAttachTo)
{
    HZ_CORE_ASSERT(!m_Initialized);
    
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

    double sampleRate = ma_engine_get_sample_rate(engine);
    uint8_t numChannels = ma_node_get_output_channels(nodeToAttachTo, 0);

    // Setting max pre-delay time to 1 second
    m_DelayLine = std::make_unique<DelayLine>((int)sampleRate + 1);
    m_RevModel = std::make_unique<revmodel>(sampleRate);

    ma_uint32 inChannels[2]{ numChannels, numChannels };
    ma_uint32 outChannels[1]{ numChannels };
    ma_node_config nodeConfig = ma_node_config_init();
    nodeConfig.vtable = &reverb_vtable;
    nodeConfig.pInputChannels = inChannels;
    nodeConfig.pOutputChannels = outChannels;
    nodeConfig.initialState = ma_node_state_started;

    ma_result result;

    allocation_callbacks = engine->pResourceManager->config.allocationCallbacks;

    result = ma_node_init(&engine->nodeGraph, &nodeConfig, &allocation_callbacks, &m_Node);
    if (abortIfFailed(result, "Node Init failed"))
        return false;

    m_DelayLine->SetConfig(ma_node_get_input_channels(&m_Node, 0), sampleRate);

    // Set default pre-delay time to 50ms
    m_DelayLine->SetDelayMs(50);

    m_Node.delayLine = m_DelayLine.get();
    m_Node.reverb = m_RevModel.get();

    result = ma_node_attach_output_bus(&m_Node, 0, nodeToAttachTo, 0);
    if (abortIfFailed(result, "Node attach failed"))
        return false;

    m_Initialized = true;

    return m_Initialized;
}

void Reverb::Suspend()
{
    m_RevModel->mute();
}

void Reverb::Resume()
{
    m_RevModel->mute();
}


void Reverb::SetParameter(EReverbParameters parameter, float value)
{
    switch (parameter)
    {
    case PreDelay:
        HZ_CORE_ASSERT(value <= m_MaxPreDelay);
        m_DelayLine->SetDelayMs((uint32_t)value);
        break;
    case Mode:
        m_RevModel->setmode(value);
        break;
    case RoomSize:
        m_RevModel->setroomsize(value); //the roomsize must be less than 1.0714, and so the GUI slider max should be 1 (f=0.98)
        break;
    case Damp:
        m_RevModel->setdamp(value);
        break;
    case Wet:
        m_RevModel->setwet(value);
        break;
    case Dry:
        m_RevModel->setdry(value);
        break;
    case Width:
        m_RevModel->setwidth(value); //! This could be incredibly useful for setting room portals and such
        break;
    }
}

float Reverb::GetParameter(EReverbParameters parameter) const
{
    float ret;

    switch (parameter)
    {
    case PreDelay:
        ret = (float)m_DelayLine->GetDelayMs();
        break;
    case Mode:
        ret = m_RevModel->getmode();
        break;
    case RoomSize:
        ret = m_RevModel->getroomsize();
        break;
    case Damp:
        ret = m_RevModel->getdamp();
        break;
    case Wet:
        ret = m_RevModel->getwet();
        break;
    case Dry:
        ret = m_RevModel->getdry();
        break;
    case Width:
        ret = m_RevModel->getwidth();
        break;
    }
    return ret;
}

const char* Reverb::GetParameterName(EReverbParameters parameter) const
{
    switch (parameter)
    {
    case PreDelay:
        return "Pre-delay";
        break;
    case Mode:
        return "Mode";
        break;
    case RoomSize:
        return "Room size";
        break;
    case Damp:
        return "Damping";
        break;
    case Wet:
        return "Wet level";
        break;
    case Dry:
        return "Dry level";
        break;
    case Width:
        return "Width";
        break;
    default:
        return "";
    }
}

std::string Reverb::GetParameterDisplay(EReverbParameters parameter) const
{
    switch (parameter)
    {
    case PreDelay:
        return std::to_string(m_DelayLine->GetDelayMs()).c_str();
        break;
    case Mode:
        if (m_RevModel->getmode() >= freezemode)
            return "Freeze";
        else
            return "Normal";
        break;
    case RoomSize:
        return std::to_string(m_RevModel->getroomsize() * scaleroom + offsetroom);
        break;
    case Damp:
        return std::to_string((long)(m_RevModel->getdamp() * 100));
        break;
    case Wet:
        return std::to_string(m_RevModel->getwet() * scalewet);
        break;
    case Dry:
        return std::to_string(m_RevModel->getdry() * scaledry);
        break;
    case Width:
        return std::to_string((long)(m_RevModel->getwidth() * 100));
        break;
    default:
        return "";
    }
}

const char* Reverb::GetParameterLabel(EReverbParameters parameter) const
{
    switch (parameter)
    {
    case PreDelay:
        return "ms";
        break;
    case Mode:
        return "mode";
        break;
    case RoomSize:
        return "size";
        break;
    case Damp:
    case Width:
        return "%";
        break;
    case Wet:
    case Dry:
        return "dB";
        break;
    default:
        return "";
    }
}

} // namespace Hazel::Audio::DSP