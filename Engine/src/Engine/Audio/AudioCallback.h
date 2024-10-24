#pragma once

#include "Audio.h"

#include "miniaudio_incl.h"

#include "choc/audio/choc_SampleBuffers.h"
#include "choc/audio/choc_SampleBufferUtilities.h"

namespace Engine::Audio
{
	class AudioCallback
	{
	public:
		AudioCallback() = default;
		virtual ~AudioCallback() = default;
		AudioCallback(AudioCallback&&) = default;
		AudioCallback& operator=(AudioCallback&&) = default;

		AudioCallback(const AudioCallback&) = delete;
		AudioCallback& operator=(const AudioCallback&) = delete;

		struct BusConfig
		{
			std::vector<uint32_t> InputBuses;
			std::vector<uint32_t> OutputBuses;
		};

		ma_node* GetNode() { return &m_Node; }

		bool Initialize(ma_engine* engine, const BusConfig& busConfig);
		void Uninitialize();
		bool StartNode();

		virtual void SuspendProcessing(bool shouldBeSuspended) = 0;
		virtual bool IsSuspended() = 0;

	protected:
		virtual bool InitBase(uint32_t sampleRate, uint32_t maxBlockSize, const BusConfig& busConfig) = 0;
		virtual void ProcessBlockBase(const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut) = 0;
		virtual void ReleaseResources() = 0;

		BusConfig m_BusConfig;
		ma_engine* m_Engine = nullptr;

	private:
		bool bInitialized = false;

		friend void processing_node_process_pcm_frames(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn,
			float** ppFramesOut, ma_uint32* pFrameCountOut);


		struct processing_node
		{
			ma_node_base base;
			ma_engine* pEngine;
			bool bInitialized = false;
			AudioCallback* callback;
		} m_Node;

		ma_node_vtable processing_node_vtable =
		{
			nullptr,
			nullptr,
			1, // 1 input bus.
			1, // 1 output bus.
			MA_NODE_FLAG_CONTINUOUS_PROCESSING | MA_NODE_FLAG_ALLOW_NULL_INPUT// Default flags.
		};
	};

	template<typename T>
	class AudioCallbackInterleaved : public AudioCallback
	{
	public:
		virtual ~AudioCallbackInterleaved() = default;

	protected:
		bool InitBase(uint32_t sampleRate, uint32_t maxBlockSize, const BusConfig& busConfig) final { return Underlying().Init(sampleRate, maxBlockSize, busConfig); }

	private:
		// CRTP interface
		AudioCallbackInterleaved() = default;
		friend T;
		T& Underlying() { return static_cast<T&>(*this); }
		const T& Underlying() const { return static_cast<T const&>(*this); }

		void ProcessBlockBase(const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut) final
		{
			Underlying().ProcessBlock(ppFramesIn, pFrameCountIn, ppFramesOut, pFrameCountOut);
		}
	};


	template<typename T>
	class AudioCallbackDeinterleaved : public AudioCallback
	{
	public:
		virtual ~AudioCallbackDeinterleaved() = default;

	protected:
		bool InitBase(uint32_t sampleRate, uint32_t maxBlockSize, const BusConfig& busConfig) final
		{
			m_InDeinterleavedBuses.resize(busConfig.InputBuses.size());
			for (int i = 0; i < m_InDeinterleavedBuses.size(); ++i)
			{
				m_InDeinterleavedBuses[i].resize({ busConfig.InputBuses[i], maxBlockSize });
				m_InDeinterleavedBuses[i].clear();
			}

			m_OutDeinterleavedBuses.resize(busConfig.OutputBuses.size());
			for (int i = 0; i < m_OutDeinterleavedBuses.size(); ++i)
			{
				m_OutDeinterleavedBuses[i].resize({ busConfig.OutputBuses[i], maxBlockSize });
				m_OutDeinterleavedBuses[i].clear();
			}


			return Underlying().Init(sampleRate, maxBlockSize, busConfig);
		}

		void ClearBuffer()
		{
			m_InDeinterleavedBuses.clear();
			m_OutDeinterleavedBuses.clear();
		}

	private:
		// CRTP interface
		AudioCallbackDeinterleaved() = default;
		friend T;
		T& Underlying() { return static_cast<T&>(*this); }
		const T& Underlying() const { return static_cast<T const&>(*this); }

		void ProcessBlockBase(const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut) final
		{
			if (ppFramesIn != NULL)
			{
				for (int i = 0; i < m_BusConfig.InputBuses.size(); ++i)
					SampleBufferOperations::Deinterleave(m_InDeinterleavedBuses[i], ppFramesIn[0]);
			}
			else
			{
				for (int i = 0; i < m_BusConfig.InputBuses.size(); ++i)
					m_InDeinterleavedBuses[i].clear();
			}

			Underlying().ProcessBlock(m_InDeinterleavedBuses, m_OutDeinterleavedBuses, *pFrameCountIn);

			for (int i = 0; i < m_BusConfig.OutputBuses.size(); ++i)
			{
				SampleBufferOperations::Interleave(ppFramesOut[0], m_OutDeinterleavedBuses[i]);
			}
		}

	private:
		std::vector<choc::buffer::ChannelArrayBuffer<float>> m_InDeinterleavedBuses;
		std::vector<choc::buffer::ChannelArrayBuffer<float>> m_OutDeinterleavedBuses;
	};


	class CallbackBindedDeinterleaved : public AudioCallbackDeinterleaved<CallbackBindedDeinterleaved>
	{
	public:
		CallbackBindedDeinterleaved() = default;
		virtual ~CallbackBindedDeinterleaved() = default;

		void SuspendProcessing(bool shouldBeSuspended) final { m_Suspended.store(shouldBeSuspended); }
		bool IsSuspended() final { return m_Suspended.load(); }

		std::function<void(const std::vector <choc::buffer::ChannelArrayBuffer<float>>& inBuffer,
								std::vector<choc::buffer::ChannelArrayBuffer<float>>& outBuffer)> onAudioCallback;
	private:
		friend AudioCallbackDeinterleaved<CallbackBindedDeinterleaved>;

		virtual void ReleaseResources() override {}
		bool Init(uint32_t sampleRate, uint32_t maxBlockSize, const BusConfig& config) { return true; }
		void ProcessBlock(const std::vector<choc::buffer::ChannelArrayBuffer<float>>& inBuffer, std::vector<choc::buffer::ChannelArrayBuffer<float>>& outBuffer, uint32_t numFramesRequested)
		{
			if (onAudioCallback && !m_Suspended.load())
				onAudioCallback(inBuffer, outBuffer);
		}

	private:
		std::atomic<bool> m_Suspended = false;
	};

	class CallbackBindedInterleaved : public AudioCallbackInterleaved<CallbackBindedInterleaved>
	{
	public:
		CallbackBindedInterleaved() = default;
		virtual ~CallbackBindedInterleaved() = default;

		void SuspendProcessing(bool shouldBeSuspended) final { m_Suspended.store(shouldBeSuspended); }
		bool IsSuspended() final { return m_Suspended.load(); }

		std::function<void(const float** ppFramesIn, ma_uint32* pFrameCountIn,
			float** ppFramesOut, const BusConfig& busConfig)> onAudioCallback;
	private:
		friend AudioCallbackInterleaved<CallbackBindedInterleaved>;

		virtual void ReleaseResources() override {}
		bool Init(uint32_t sampleRate, uint32_t maxBlockSize, const BusConfig& config) { return true; }
		void ProcessBlock(const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
		{
			if (onAudioCallback && !m_Suspended.load())
				onAudioCallback(ppFramesIn, pFrameCountIn, ppFramesOut, m_BusConfig);
		}

	private:
		std::atomic<bool> m_Suspended = false;
	};

	class CallbackTest_Passthrough : public AudioCallbackInterleaved<CallbackTest_Passthrough>
	{
	public:
		CallbackTest_Passthrough() = default;

		void SuspendProcessing(bool shouldBeSuspended) override { m_Suspended.store(shouldBeSuspended); }
		bool IsSuspended() override { return m_Suspended.load(); }


	private:
		friend AudioCallbackInterleaved<CallbackTest_Passthrough>;

		void ReleaseResources() override {}
		bool Init(uint32_t sampleRate, uint32_t maxBlockSize, const BusConfig& config) /*override*/ { return true; }
		void ProcessBlock(const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut) /*override*/
		{
			if (!m_Suspended.load())
				memcpy(ppFramesOut[0], ppFramesIn[0], sizeof(float) * (*pFrameCountIn) * m_BusConfig.OutputBuses[0]);
		}

	private:
		std::atomic<bool> m_Suspended = false;
	};

	class CallbackTest_PassthroughD : public AudioCallbackDeinterleaved<CallbackTest_PassthroughD>
	{
	public:
		CallbackTest_PassthroughD() = default;

		void SuspendProcessing(bool shouldBeSuspended) override { m_Suspended.store(shouldBeSuspended); }
		bool IsSuspended() override { return m_Suspended.load(); }


	private:
		friend AudioCallbackDeinterleaved<CallbackTest_PassthroughD>;

		void ReleaseResources() override {}
		bool Init(uint32_t sampleRate, uint32_t maxBlockSize, const BusConfig& config) /*override*/ { return true; }
		void ProcessBlock(const std::vector <choc::buffer::ChannelArrayBuffer<float>>& inBuffer, std::vector<choc::buffer::ChannelArrayBuffer<float>>& outBuffer, uint32_t numFramesRequested) /*override*/
		{
			if (!m_Suspended.load())
				choc::buffer::copy(outBuffer[0], inBuffer[0]);
		}

	private:
		std::atomic<bool> m_Suspended = false;
	};

} // namespace Engine::Audio
