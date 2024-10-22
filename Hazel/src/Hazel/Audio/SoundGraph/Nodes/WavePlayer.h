#pragma once
#include "Hazel/Audio/SoundGraph/NodeProcessor.h"
#include "Hazel/Audio/SoundGraph/WaveSource.h"

#include <random>
#include <chrono>
#include <array>
#include <type_traits>

#define LOG_DBG_MESSAGES 0

#if LOG_DBG_MESSAGES
#define DBG(...) HZ_CORE_WARN(__VA_ARGS__)
#else
#define DBG(...)
#endif

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Hazel::SoundGraph
{
	//==============================================================================
	struct WavePlayer : public NodeProcessor
	{
		struct IDs // TODO: could replace this with simple enum
		{
			DECLARE_ID(Play);
			DECLARE_ID(Stop);
		private:
			IDs() = delete;
		};

		explicit WavePlayer(const char* dbgName, UUID id) : NodeProcessor(dbgName, id)
		{
			// Inputs
			AddInEvent(IDs::Play, [this](float v) { Play(v); });
			AddInEvent(IDs::Stop, [this](float v) { Stop(v); });

			RegisterEndpoints();
		}

		// TODO: maybe non-pointer variables for these could be updated at the beginning of every buffer callback
		//		to avoid constant access for pointers
		int64_t* in_WaveAsset = nullptr; // This is pointing to some external data
		float* in_StartTime = nullptr;
		bool* in_Loop = nullptr;
		int* in_NumberOfLoops = nullptr;

		// these must be located together,
		// we directly memcpy into them
		float out_OutLeft;
		float out_OutRight;

		float out_PlaybackPosition{ 0.0f };

		OutputEvent out_OnPlay{ *this };
		OutputEvent out_OnFinish{ *this };
		OutputEvent out_OnLooped{ *this };

		Audio::WaveSource buffer; //? Should this be a pointer? This needs to be filled up externally, though we do access it locally most of the time
	private:
		//==============================================================================
		// Internal

		double sampleRate = 48000.0;
		uint64_t waveAsset = 0;

		// State variables
		bool isInitialized = false;
		bool swapOnInit = false;

		bool playing = false;

		// Dynamic members
		int64_t totalFrames = 0;
		int64_t frameNumber = 0;

		int64_t startSample = 0;
		// current loop count since playback started
		int loopCount = 0;

		double framePlaybackFraction = 0.0;

		Flag fPlay;
		Flag fStop;

	public:
		void Play(float)
		{
			DBG("PLAY EVENT");
			fPlay.SetDirty();
		}

		void Stop(float)
		{
			DBG("STOP EVENT");
			fStop.SetDirty();
		}

	private:
		//=============================================================================
		/// Internal
		void RegisterEndpoints();
		void InitializeInputs();

		void StartPlayback()
		{
			if (!isInitialized)
			{
				HZ_CORE_WARN("!WavePlayer::Play() - Wave Player is not initialized.");
				return;
			}

			startSample = int64_t(*in_StartTime * sampleRate /*currentBuffer.sampleRate*/);
			HZ_CORE_ASSERT(startSample >= 0);

			loopCount = 0; // TODO: might not want to reset loopCount in some cases
			frameNumber = startSample;

			// Getting updated wave asset to play when starting playback
			/// When Play is triggered, it should be able play new sound immediately
			// If Wave Asset was changed while playing, need to refill the buffer now
			UpdateWaveSourceIfNeeded();

			// It is valid to request to play 0 handle,
			// in such case we just ignore the playback request and stop
			if (!buffer.WaveHandle)
			{
				// TODO: proper console out
				// TODO: log macros should be sending to output message queue, for thread safety
				//HZ_CORE_WARN("!WavePlayer::Play() - invalid wave source id.");

				StopPlayback(true);
				
				return;
			}

			// If was already playing, need to fill the buffer from starting position
			if (playing)
				ForceRefillBuffer();

			playing = true;
			out_OnPlay(2.0f);
		}

		void StopPlayback(bool notifyOnFinish)
		{
			playing = false;
			loopCount = 0;
			frameNumber = startSample;
			buffer.ReadPosition = frameNumber;

			UpdateWaveSourceIfNeeded();

			if (notifyOnFinish)
				out_OnFinish(2.0f);
		}

	public:
		void Init() final
		{
			InitializeInputs();

			// TODO: take a snapshot of the initial values and restore it when new graph is initialized, or copied (?)

			// State variables
			isInitialized = false;
			swapOnInit = false;
			playing = false;

			// Dynamic members
			totalFrames = 0;
			frameNumber = 0;
			//readPosition = 0;

			startSample = 0;
			loopCount = 0;

			buffer.Clear();
			UpdateWaveSourceIfNeeded();

			isInitialized = true;
		}

		void Process() final
		{
			if (fPlay.CheckAndResetIfDirty())
				StartPlayback();

			if (fStop.CheckAndResetIfDirty())
				StopPlayback(true);

			if (playing) //? this is quite hot check, need to optimize it, possibly remove it
			{
				// Need to do this check on the next frame after the last to read 
				// to allow seamless chaining of sound waves
				if (frameNumber >= buffer.TotalFrames)
				{
					if (*in_Loop)
					{
						++loopCount;
						out_OnLooped(2.0f);//++loopCount;

						// Stop playback if previous sample was the last one of the last loop
						if (*in_NumberOfLoops >= 0 && loopCount > *in_NumberOfLoops)
						{
							StopPlayback(true);
							OutputSilence();
						}
						else
						{
							// If we still have loops to play, read the next frame of data
							// and increment the counters from the 'startSample'

							buffer.Channels.GetMultiple<2>(&out_OutLeft);

							frameNumber = startSample;
							buffer.ReadPosition = ++frameNumber;
						}
					}
					else
					{
						// This is the next frame after the last one to read,
						// now we can send OnFinsh event and proceed the execution
						// on this current frame.
						StopPlayback(true);
						OutputSilence();
					}
				}
				else
				{
					// make sure we read in the right order of interleaved channels
					buffer.Channels.GetMultiple<2>(&out_OutLeft);

					buffer.ReadPosition = ++frameNumber;
				}

				out_PlaybackPosition = float(frameNumber * framePlaybackFraction);
			}
			else
			{
				OutputSilence();
			}
			//advance();
		}

	private:
		inline void PreinitBuffer()
		{
			buffer.Clear();
			buffer.StartPosition = startSample;
			buffer.ReadPosition = frameNumber;
			buffer.WaveHandle = waveAsset;
		}

		void UpdateWaveSourceIfNeeded()
		{
			waveAsset = /*(uint64_t)*/ (*in_WaveAsset);

			if (buffer.WaveHandle != waveAsset)
			{
				PreinitBuffer();

				if (buffer.WaveHandle)
				{
					const bool refilled = buffer.Refill();
					HZ_CORE_ASSERT(refilled);

					totalFrames = buffer.TotalFrames;
					framePlaybackFraction = double(1.0 / totalFrames);
					isInitialized = (bool)waveAsset && refilled;
				}
				else // It is valid to request to play 0 handle
				{
					totalFrames = 0;
					framePlaybackFraction = 0.0f;
				}
			}
		}

		void ForceRefillBuffer()
		{
			HZ_CORE_ASSERT(waveAsset)
			
			buffer.ReadPosition = frameNumber;
			const bool refilled = buffer.Refill();
			HZ_CORE_ASSERT(refilled);
		}

		inline void OutputSilence() noexcept
		{
			memset(&out_OutLeft, 0, sizeof(float) * 2);
		}
	};

} // Hazel::SoundGraph

#undef DECLARE_ID
#undef LOG_DBG_MESSAGES
#undef DBG
