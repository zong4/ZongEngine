#pragma once

#include "Hazel/Audio/SoundObject.h"
#include "miniaudio_incl.h"

#include <glm/glm.hpp>

#include "Hazel/Audio/DSP/Spatializer/Spatializer.h"
#include "Hazel/Audio/DSP/Filters/FilterLowPass.h"
#include "Hazel/Audio/DSP/Filters/FilterHighPass.h"

// TODO: move PropertySet out of Editor project
#include "Hazel/Editor/NodeGraphEditor/PropertySet.h"

//#include "DSP/FilterAirAbsorption.h"

namespace Hazel
{
    class MiniAudioEngine;
    class SoundGraphSource;

    namespace SoundGraph
    {
        struct SoundGraph;
    }

    /* =====================================
        SoundGraph Sound, represents playing voice

        -------------------------------------
    */
    class SoundGraphSound : public ISound
    {
    public:
        explicit SoundGraphSound();
        ~SoundGraphSound();

        //--- Sound Source Interface
        virtual bool Play() override;
        virtual bool Stop() override;
        virtual bool Pause() override;
        virtual bool IsPlaying() const override;
        // ~ End of Sound Source Interface

        virtual void SetVolume(float newVolume) override;
        virtual void SetPitch(float newPitch) override;
        void SetLooping(bool looping);

        virtual float GetVolume() override;
        virtual float GetPitch() override;

		virtual void SetLowPassFilter(float value) override { m_LowPass.SetCutoffFrequency(Audio::NormalizedToFrequency(value)); }
		virtual void SetHighPassFilter(float value) override { m_HighPass.SetCutoffFrequency(Audio::NormalizedToFrequency(value)); }

        //==============================================================================
        /// Sound Parameter Interface
        void SetParameter(uint32_t pareterID, float value) final;
        void SetParameter(uint32_t pareterID, int value) final;
        void SetParameter(uint32_t pareterID, bool value) final;

        virtual bool FadeIn(const float duration, const float targetVolume) override;
        virtual bool FadeOut(const float duration, const float targetVolume) override;

        /* Initialize data source from sound configuration.
            @param soundConfig - configuration to initialized data source with
            @param audioEngine - reference to AudioEngine
            
            @returns true - if successfully initialized data source
         */
        bool InitializeDataSource(const Ref<SoundConfig>& soundConfig, MiniAudioEngine* audioEngine, const Ref<SoundGraph::SoundGraph>& source);

        virtual void ReleaseResources() override;

        void SetLocation(const glm::vec3& location, const glm::vec3& orientation);
        void SetVelocity(const glm::vec3& velocity = { 0.0f, 0.0f, 0.0f });

        /* @return true - if has a valid data source */
        bool IsReadyToPlay() const { return bIsReadyToPlay; }

        void Update(Timestep ts) override;

        /* @returns true - if playback complete. E.g. reached the end of data, or has been hard-stopped. */
        virtual bool IsFinished() const override;

        /* @returns true - if currently "stop-fading". */
        virtual bool IsStopping() const override;
        
        virtual bool IsLooping() const override { return bLooping; };

		/* Get current level of fader performing fade operations.
		   Such operations performed when FadeIn(), or FadeOut() are called,
		   as well as "stop-fade" and "resume from pause" fade.
		 
		   @returns current fader level
		*/
        float GetCurrentFadeVolume();

        /* Get current priority level based on current fade volume 
		   and priority value set for this sound source.
		 
		   @returns normalized priority value
		*/
        virtual float GetPriority() override;

        /* @returns current playback percentage (read position) whithin data source */
        virtual float GetPlaybackPercentage() override;

        ESoundPlayState GetPlayState() const override { return m_PlayState; }

    private:
        /* Stop playback with short fade-out to prevent click.
		   @param numSamples - length of the fade-out in PCM frames
		 
		   @returns true - if successfully initialized fade
		*/
        bool StopFade(uint64_t numSamples);

        /* Stop playback with short fade-out to prevent click.
		   @param milliseconds - length of the fade-out in milliseconds
		 
		   @returns true - if successfully initialized fade
		*/
        bool StopFade(int milliseconds);

        enum EStopNowOptions : uint16_t
        {
            None = 0,
            NotifyPlaybackComplete  = BIT(0),
            ResetPlaybackPosition   = BIT(1)
        };

		/* "Hard-stop" playback without fade. This is called to immediatelly stop playback, has ended,
		   as well as to reset the play state when "stop-fade" has ended.
		   @param notifyPlaybackComplete - used when the sound has finished its playback
		   @param resetPlaybackPosition - set to 'false' when pausing
		 
		   @returns 1 ////ID of the sound source in pool
		*/
        int StopNow(bool notifyPlaybackComplete, bool resetPlaybackPosition);
        int StopNow(uint16_t options = EStopNowOptions::None);

        void InitializeEffects(const Ref<SoundConfig>& config);

		bool InitializeAudioCallback();

    private:
        friend class MiniAudioEngine;
        friend class SourceManager;

        std::function<void()> onPlaybackComplete;
        std::string m_DebugName;

        ESoundPlayState m_PlayState{ ESoundPlayState::Stopped };
        ESoundPlayState m_NextPlayState{ ESoundPlayState::Stopped };

        /* Data source. For now all we use is Miniaudio, 
            in the future SoundObject will access data source via SoundSource class.
         */
        Scope<SoundGraphSource> m_Source = nullptr;
		Utils::PropertySet m_SoundGraphPreset;

        ma_splitter_node m_MasterSplitter;

        bool bIsReadyToPlay = false;

        // TODO
        uint8_t m_Priority = 64;

        bool bLooping = false;
        bool bFinished = false;

        /* Stored Fader "resting" value. Used to restore Fader before restarting playback if a fade has occured. */
        float m_StoredFaderValue = 1.0f;
        float m_LastFadeOutDuration = 0.0f;

        double m_Volume = 1.0f;
        double m_Pitch = 1.0f;

        /* Stop-fade counter. Used to stop the sound after "stopping-fade" has finished. */
        double m_StopFadeTime = 0.0;

        Audio::DSP::LowPassFilter m_LowPass;
        Audio::DSP::HighPassFilter m_HighPass;

        //? TESTING
        //DSP::AirAbsorptionFilter m_AirAbsorptionFilter;
    };

} // namespace Hazel
