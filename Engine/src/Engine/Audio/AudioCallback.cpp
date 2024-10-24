#include <pch.h>
#include "AudioCallback.h"

namespace Engine::Audio
{
	void processing_node_process_pcm_frames(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
	{
		AudioCallback::processing_node* node = (AudioCallback::processing_node*)pNode;

		AudioCallback* callback = node->callback;
		if (!callback->IsSuspended())
		{
			ma_silence_pcm_frames(ppFramesOut[0], *pFrameCountOut, ma_format_f32, 2);
			// TODO: the issue with soul player and miniaudio doppler effect is that miniaudio requests number of samples less than default block size to pitch down,
			//?		but on my end I don't handle it in any way.
			callback->ProcessBlockBase(ppFramesIn, pFrameCountIn, ppFramesOut, pFrameCountOut);
		}
		else
		{
			ma_silence_pcm_frames(ppFramesOut[0], *pFrameCountOut, ma_format_f32, 2);
		}
	}

	bool AudioCallback::Initialize(ma_engine* engine, const BusConfig& busConfig)
	{
		m_Engine = engine;
		m_BusConfig = busConfig;

		uint32_t sampleRate = engine->pDevice->sampleRate;
		uint32_t blockSize = engine->pDevice->playback.internalPeriodSizeInFrames;

		ZONG_CORE_ASSERT(!busConfig.InputBuses.empty() && !busConfig.OutputBuses.empty()); //? input might want to allow 0 buses
		for (auto& b : busConfig.InputBuses)  ZONG_CORE_ASSERT(b > 0);
		for (auto& b : busConfig.OutputBuses) ZONG_CORE_ASSERT(b > 0);

		if (m_Node.bInitialized)
		{
			ma_node_set_state(&m_Node, ma_node_state_stopped);
			ma_node_uninit(&m_Node, nullptr);
			m_Node.bInitialized = false;
		}

		m_Node.callback = this;
		m_Node.pEngine = m_Engine;

		// TODO: initialize node to required layout
		processing_node_vtable.onProcess = processing_node_process_pcm_frames;
		processing_node_vtable.inputBusCount = (ma_uint8)busConfig.InputBuses.size();
		processing_node_vtable.outputBusCount = (ma_uint8)busConfig.OutputBuses.size();
		
		ma_result result;

		ma_node_config nodeConfig = ma_node_config_init();
		nodeConfig.initialState = ma_node_state_stopped;

		//const uint32_t numInChannels[1]{ 2 };
		//const uint32_t numOutChannels[1]{ 2 }; //? must match endpoint input channel count, or use channel converter

		nodeConfig.pInputChannels = busConfig.InputBuses.data();
		nodeConfig.pOutputChannels = busConfig.OutputBuses.data();
		nodeConfig.vtable = &processing_node_vtable;
		result = ma_node_init(&engine->nodeGraph, &nodeConfig, nullptr, &m_Node);

		ZONG_CORE_ASSERT(result == MA_SUCCESS);
		m_Node.bInitialized = true;

		result = ma_node_attach_output_bus(&m_Node, 0u, &engine->nodeGraph.endpoint, 0u);
		ZONG_CORE_ASSERT(result == MA_SUCCESS);

		return InitBase(sampleRate, blockSize, busConfig);
	}

	void AudioCallback::Uninitialize()
	{
		if (m_Node.bInitialized)
		{
			ma_node_set_state(&m_Node, ma_node_state_stopped);
			ma_node_uninit(&m_Node, nullptr);
			m_Node.bInitialized = false;
			m_Node.pEngine = nullptr;
			m_Node.callback = nullptr;
		}

		ReleaseResources();
	}

	bool AudioCallback::StartNode()
	{
		if (m_Node.bInitialized)
			ma_node_set_state(&m_Node, ma_node_state_started);

		return m_Node.bInitialized;
	}
} // namespace Engine::Audio
