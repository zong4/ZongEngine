#pragma once

#include "Hazel/Asset/Asset.h"
#include "Hazel/Core/TimeStep.h"

#include <type_traits>

namespace Hazel
{
    enum class AttenuationModel
    {
        None,          // No distance attenuation and no spatialization.
        Inverse,       // Equivalent to OpenAL's AL_INVERSE_DISTANCE_CLAMPED.
        Linear,        // Linear attenuation. Equivalent to OpenAL's AL_LINEAR_DISTANCE_CLAMPED.
        Exponential    // Exponential attenuation. Equivalent to OpenAL's AL_EXPONENT_DISTANCE_CLAMPED.

        // TODO: CusomCurve
    };

    /* ==============================================
        Configuration for 3D spatialization behavior
        ---------------------------------------------
     */
    struct SpatializationConfig : public Asset
    {
        AttenuationModel AttenuationMod{ AttenuationModel::Inverse };   // Distance attenuation function
        float MinGain{ 0.0f };                                            // Minumum volume muliplier
        float MaxGain{ 1.0f };                                            // Maximum volume multiplier
        float MinDistance{ 1.0f };                                        // Distance where to start attenuation
        float MaxDistance{ 1000.0f };                                     // Distance where to end attenuation
        float ConeInnerAngleInRadians{ 6.283185f };                     // Defines the angle where no directional attenuation occurs 
        float ConeOuterAngleInRadians{ 6.283185f };                     // Defines the angle where directional attenuation is at max value (lowest multiplier)
        float ConeOuterGain{ 0.0f };                                      // Attenuation multiplier when direction of the emmiter falls outside of the ConeOuterAngle
        float DopplerFactor{ 1.0f };                                      // The amount of doppler effect to apply. Set to 0 to disables doppler effect. 
        float Rolloff{ 0.6f };                                            // Affects steepness of the attenuation curve. At 1.0 Inverse model is the same as Exponential

        bool bAirAbsorptionEnabled{ true };                            // Enable Air Absorption filter (TODO)

        bool bSpreadFromSourceSize{ true };                             // If this option is enabled, spread of a sound source automatically calculated from the source size.
        float SourceSize{ 1.0f };                                       // Diameter of the sound source in game world.
        float Spread{ 1.0f };
        float Focus{ 1.0f };

        static AssetType GetStaticType() { return AssetType::SpatializationConfig; }
        virtual AssetType GetAssetType() const override { return GetStaticType(); }
    };

    /* ==============================================
        Configuration to initialize sound source

        Can be passed to an AudioComponent              (TODO: or to directly initialize Sound to play)
        ----------------------------------------------
    */
    struct SoundConfig : public Asset
    {
        AssetHandle DataSourceAsset = 0;     // Audio data source

        bool bLooping = false;
        float VolumeMultiplier = 1.0f;
        float PitchMultiplier = 1.0f;

        bool bSpatializationEnabled = false;
        Ref<SpatializationConfig> Spatialization{ new SpatializationConfig() };        // Configuration for 3D spatialization behavior

        float MasterReverbSend = 0.0f;
        float LPFilterValue = 1.0f;
        float HPFilterValue = 0.0f;

        static AssetType GetStaticType() { return AssetType::SoundConfig; }
        virtual AssetType GetAssetType() const override { return GetStaticType(); }
    };

    enum class ESoundPlayState
    {
        Stopped,
        Starting,
        Playing,
        Pausing,
        Paused,
        Stopping,
        FadingOut,
        FadingIn
    };

    class ISoundParameters
    {
    public:
        virtual void SetParameter(uint32_t pareterID, float value) {}
        virtual void SetParameter(uint32_t pareterID, int32_t value) {}
        virtual void SetParameter(uint32_t pareterID, bool value) {}
    };

    class ISound : public ISoundParameters
    {
    public:
        virtual ~ISound() = default;

        virtual bool Play() = 0;
        virtual bool Stop() = 0;
        virtual int StopNow(bool notifyPlaybackComplete /*= true*/, bool resetPlaybackPosition /*= true*/) = 0;
        virtual bool Pause() = 0;
        virtual bool IsPlaying() const = 0;
        virtual bool IsFinished() const = 0;
        virtual bool IsStopping() const = 0;
        virtual bool IsLooping() const = 0;

        virtual void SetVolume(float newVolume) = 0;
        virtual void SetPitch(float newPitch) = 0;

		virtual void SetLowPassFilter(float value) = 0;
		virtual void SetHighPassFilter(float value) = 0;

        virtual float GetVolume() = 0;
        virtual float GetPitch() = 0;

        virtual bool FadeIn(const float duration, const float targetVolume) = 0;
        virtual bool FadeOut(const float duration, const float targetVolume) = 0;

        virtual void Update(Timestep ts) = 0;

        virtual ESoundPlayState GetPlayState() const = 0;
        virtual float GetPriority() = 0;
        virtual float GetPlaybackPercentage() = 0;

        virtual void ReleaseResources() = 0;
    };

    /* ==================================
        Bridge Interface for Sound Object

        Represents playing voice
        ---------------------------------
     */
    class SoundObject
    {
    public:
        SoundObject();
        virtual ~SoundObject();

        ISound* SetSound(Scope<ISound>&& sound);

        bool Play() { return m_Sound->Play(); }
        bool Stop() { return m_Sound->Stop(); }
        int StopNow(bool notifyPlaybackComplete = true, bool resetPlaybackPosition = true) { return m_Sound->StopNow(notifyPlaybackComplete, resetPlaybackPosition); }

        bool Pause()            { return m_Sound->Pause(); }
        bool IsPlaying() const  { return m_Sound->IsPlaying(); }
        bool IsFinished() const { return m_Sound->IsFinished(); }
        bool IsStopping() const { return m_Sound->IsStopping(); }
        bool IsLooping() const  { return m_Sound->IsLooping(); };

        void SetVolume(float newVolume) { m_Sound->SetVolume(newVolume); }
        void SetPitch(float newPitch)   { m_Sound->SetPitch(newPitch); }

		void SetLowPassFilter(float value) { m_Sound->SetLowPassFilter(value); };
		void SetHighPassFilter(float value) { m_Sound->SetHighPassFilter(value); };

        float GetVolume()   { return m_Sound->GetVolume(); }
        float GetPitch()    { return m_Sound->GetPitch(); }

        bool FadeIn(const float duration, const float targetVolume)     { return m_Sound->FadeIn(duration, targetVolume); }
        bool FadeOut(const float duration, const float targetVolume)    { return m_Sound->FadeOut(duration, targetVolume); }

        void Update(Timestep ts) { m_Sound->Update(ts); }

        static std::string StringFromState(ESoundPlayState state);

        ESoundPlayState GetPlayState() const { return m_Sound->GetPlayState(); }

        void ReleaseResources() { if (m_Sound) m_Sound->ReleaseResources(); }

        int GetSourceID() const         { return m_SoundSourceID; }
        float GetPriority()             { return m_Sound->GetPriority(); }
        float GetPlaybackPercentage()   { return m_Sound->GetPlaybackPercentage(); }

        // Parameter Interface
        void SetParameter(uint32_t parameterID, float value){ m_Sound->SetParameter(parameterID, value); }
        void SetParameter(uint32_t parameterID, int value)  { m_Sound->SetParameter(parameterID, value); }
        void SetParameter(uint32_t parameterID, bool value) { m_Sound->SetParameter(parameterID, value); }


    private:
        friend class MiniAudioEngine;

        Scope<ISound> m_Sound = nullptr;

        // Voice data
        // ----------
        Ref<SoundConfig> m_SoundConfig = nullptr;
        int m_SoundSourceID = -1;

        uint64_t m_AudioObjectID = 0;
        uint32_t m_EventID = 0;
        /* ID of the scene this voice belongs to. */
        uint64_t m_SceneID = 0;
    };

} // namespace Hazel
