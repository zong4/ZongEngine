#pragma once

#include "Hazel/Core/Timer.h"
#include "Hazel/Core/TimeStep.h"
#include "Hazel/Core/Delegate.h"

#include "choc/audio/choc_SampleBuffers.h"
#include "choc/audio/choc_SampleBufferUtilities.h"

#include <thread>
#include <atomic>
#include <queue>

/*  --------------------------
    Audio specific definitions
    
    Core concepts
    --------------------------
*/

namespace Hazel
{
    class MiniAudioEngine;
	
	namespace Audio
	{
		class AudioThreadFence;
	}
}

namespace Hazel::Audio
{
    //static constexpr auto PCM_FRAME_CHUNK_SIZE = 1024;                                            //? for now unable to set engine block size
    static constexpr auto PCM_FRAME_CHUNK_SIZE = 480;                                               //  default block size with WASAPI backend
    static constexpr auto PCM_FRAME_CHUNK_MS = uint32_t(PCM_FRAME_CHUNK_SIZE / 48000.0f * 1000);    //  arbitrary value for sample rate, only need aproximate value ms
    static constexpr auto STOPPING_FADE_MS = 28;
    static constexpr float SPEED_OF_SOUND = 343.3f;

    //==============================================================================
    using AudioThreadCallbackFunction = std::function<void()>;

    class AudioFunctionCallback
    {
    public:
        AudioFunctionCallback(AudioThreadCallbackFunction f, const char* jobID = "NONE")
            : m_Func(std::move(f)), m_JobID(jobID)
        {
        }

        void Execute()
        {
            m_Func();
        }

        const char* GetID() { return m_JobID; }

    private:
        AudioThreadCallbackFunction const m_Func;
        const char* m_JobID;
    };

    //==============================================================================
	///	Audio Update Thread
    class AudioThread // TODO: inherit from Hazel::Thread
    {
    public:
        static bool Start();
        static bool Stop();
        static bool IsRunning();
        static bool IsAudioThread();
        static std::thread::id GetThreadID();

    private:
        friend class Hazel::MiniAudioEngine;
        friend class AudioThreadFence;

        static void AddTask(AudioFunctionCallback*&& funcCb);
        static void OnUpdate();
        static float GetFrameTime() { return s_LastFrameTime.load(); }

        template<auto TFunction, class TClass>
        static void BindUpdateFunction(TClass* object)
        {
			using TUpdateFunc = void(TClass::*)(Timestep);
			static_assert(std::is_same_v<decltype(TFunction), TUpdateFunc>, "Invalid Update function signature.");

			onUpdateCallback.Bind<TFunction>(object);
        }

    private:
        static inline std::thread* s_AudioThread{ nullptr };
        static inline std::atomic<bool > s_ThreadActive{ false };
		static inline std::atomic<std::thread::id> s_AudioThreadID{ std::thread::id() };
        
        //? std::queue may not be the best option here 
        static inline std::queue<AudioFunctionCallback*> s_AudioThreadJobs;
        static inline std::mutex s_AudioThreadJobsLock;

		static inline Delegate<void(Timestep)> onUpdateCallback;
        static inline Timer s_Timer;
		static inline Timestep s_TimeStep{ 0.0f };
		static inline std::atomic<float> s_LastFrameTime{ 0.0f };
    };


    static inline bool IsAudioThread()
    {
        /*if (sizeof(std::thread::id) == sizeof(uint64_t))
        {
            unsigned int audioThreadID = s_AudioThreadID.load();
            auto id = std::this_thread::get_id();
            uint64_t* ptr = (uint64_t*) &id;
            uint64_t thisThreadID = *ptr;

            return thisThreadID == audioThreadID;

        }
        else if (sizeof(std::thread::id) == sizeof(unsigned int))
        {
            unsigned int audioThreadID = s_AudioThreadID.load();
            auto id = std::this_thread::get_id();
            uint64_t* ptr = (uint64_t*) &id;
            unsigned int thisThreadID = *ptr;

            return thisThreadID == audioThreadID;
        }*/

        return std::this_thread::get_id() == AudioThread::GetThreadID();
    }

	//==============================================================================
	/// Used to wait for all pending audio thread tasks to complete
	class AudioThreadFence
	{
	public:
		AudioThreadFence();
		~AudioThreadFence();

		void Begin();
		void BeginAndWait();
		inline bool IsReady() const { return !m_Counter->GetRefCount(); }
		void Wait() const;

	private:
		struct Counter : RefCounted {};
		Counter* m_Counter = new Counter();
	};

    struct Transform
    {
        glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Orientation{ 0.0f, 0.0f, -1.0f };
        glm::vec3 Up{ 0.0f, 1.0f, 0.0f };

        bool operator==(const Transform& other) const
        {
            return Position == other.Position && Orientation == other.Orientation && Up == other.Up;
        }

        bool operator!=(const Transform& other) const
        {
            return !(*this == other);
        }

    };

    //==============================================================================

    static inline float Lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

	static inline float NormalizedToFrequency(float value)
	{
		const float octaveRange = glm::log2(22000.0f / 20.0f);
		return glm::exp2(value * octaveRange) * (20.0f);
	}

	//==============================================================================
    class SampleBufferOperations
    {
    public:
        static inline void ApplyGainRamp(float* data, uint32_t numSamples, uint32_t numChannels, float gainStart, float gainEnd)
        {
            const float delta = (gainEnd - gainStart) / (float)numSamples;
            for (uint32_t i = 0; i < numSamples; ++i)
            {
                for (uint32_t ch = 0; ch < numChannels; ++ch)
                    data[i * numChannels + ch] *= gainStart + delta * i;
            }
        }

        static inline void ApplyGainRampToSingleChannel(float* data, uint32_t numSamples, uint32_t numChannels, uint32_t channel, float gainStart, float gainEnd)
        {
            const float delta = (gainEnd - gainStart) / (float)numSamples;
            for (uint32_t i = 0; i < numSamples; ++i)
            {
                data[i * numChannels + channel] *= gainStart + delta * i;
            }
        }

        static inline void AddAndApplyGainRamp(float* dest, const float* source, uint32_t destChannel, uint32_t sourceChannel,
                                                uint32_t destNumChannels, uint32_t sourceNumChannels, uint32_t numSamples, float gainStart, float gainEnd)
        {
            if (gainEnd == gainStart)
            {
                for (uint32_t i = 0; i < numSamples; ++i)
                    dest[i * destNumChannels + destChannel] += source[i * sourceNumChannels + sourceChannel] * gainStart;
            }
            else
            {
                const float delta = (gainEnd - gainStart) / (float)numSamples;
                for (uint32_t i = 0; i < numSamples; ++i)
                {
                    dest[i * destNumChannels + destChannel] += source[i * sourceNumChannels + sourceChannel] * gainStart;
                    gainStart += delta;
                }
            }
        }

        static inline void AddAndApplyGain(float* dest, const float* source, uint32_t destChannel, uint32_t sourceChannel,
                                            uint32_t destNumChannels, uint32_t sourceNumChannels, uint32_t numSamples, float gain)
        {
            for (uint32_t i = 0; i < numSamples; ++i)
                dest[i * destNumChannels + destChannel] += source[i * sourceNumChannels + sourceChannel] * gain;
        }

        static bool ContentMatches(const float* buffer1, const float* buffer2, uint32_t frameCount, uint32_t numChannels)
        {
            for (uint32_t frame = 0; frame < frameCount; ++frame)
            {
                for (uint32_t chan = 0; chan < numChannels; ++chan)
                {
                    const auto pos = frame * numChannels + chan;
                    if (buffer1[pos] != buffer2[pos])
                        return false;
                }
            }

            return true;
        }

        static inline void AddDeinterleaved(float* dest, const float* source, uint32_t numSamples)
        {
            for (uint32_t i = 0; i < numSamples; ++i)
                dest[i] += source[i];
        }

        template<typename SampleType>
        static void Deinterleave(choc::buffer::ChannelArrayBuffer<SampleType>& dest, const float* source)
        {
            auto numChannels = dest.getNumChannels();
            auto destData = dest.getView().data.channels;

            for (uint32_t ch = 0; ch < dest.getNumChannels(); ++ch)
            {
                for (uint32_t s = 0; s < dest.getNumFrames(); ++s)
                {
                    destData[ch][s] = source[s * numChannels + ch];
                }
            }
        }

        template <typename SampleType>
        static void Interleave(float* dest, const choc::buffer::ChannelArrayBuffer<SampleType>& source)
        {
            auto numChannels = source.getNumChannels();
            auto sourceData = source.getView().data.channels;

            for (uint32_t ch = 0; ch < source.getNumChannels(); ++ch)
            {
                for (uint32_t s = 0; s < source.getNumFrames(); ++s)
                {
                    dest[s * numChannels + ch] = sourceData[ch][s];
                }
            }
        }

        static void Deinterleave(float* const* dest, const float* source, uint32_t numChannels, uint32_t numSamples)
        {
            for (uint32_t ch = 0; ch < numChannels; ++ch)
            {
                for (uint32_t s = 0; s < numSamples; ++s)
                {
                    dest[ch][s] = source[s * numChannels + ch];
                }
            }
        }

        static void Interleave(float* dest, const float* const* source, uint32_t numChannels, uint32_t numSamples)
        {
            for (uint32_t ch = 0; ch < numChannels; ++ch)
            {
                for (uint32_t s = 0; s < numSamples; ++s)
                {
                    dest[s * numChannels + ch] = source[ch][s];
                }
            }
        }

        template<typename T, template<typename> typename LayoutTypet>
        static T GetMagnitude(const choc::buffer::BufferView<T, LayoutTypet>& buffer, int startSample, int numSamples)
        {
            T mag(0);
            for (int i = 0; i < buffer.getNumChannels(); ++i)
            {
                for (int s = 0; s < numSamples; ++s)
                    mag = std::max(std::abs(buffer.getChannel(i).data.data[s]), mag);

                // TODO: test which way is faster
                //choc::span channelData(buffer.getChannel(i).fromFrame(startSample).getIterator(i).sample, numSamples);
                //auto minMax = std::minmax_element(channelData.begin(), channelData.end());
                //mag = std::max(mag, std::max(std::abs(*minMax.first), *minMax.second));
            }

            return mag;
        }

        template<typename T>
        static T GetRMSLevel(const choc::buffer::ChannelArrayBuffer<T>& buffer, int channel, int startSample, int numSamples) noexcept
        {
            assert(channel >= 0 && channel < buffer.getNumChannels());
            assert(startSample >= 0 && numSamples >= 0 && startSample + numSamples <= buffer.getNumFrames());

            if (numSamples <= 0 || channel < 0 || channel >= buffer.getNumChannels() || buffer.getNumFrames() == 0)
                return T(0);

            T* data = &buffer.getSample(channel, startSample);
            double sum = 0.0;

            for (int i = 0; i < numSamples; ++i)
            {
                auto sample = (double)data[i];
                sum += sample * sample;
            }

            return static_cast<T> (std::sqrt(sum / numSamples));
        }

    };

} // namespace Hazel::Audio
