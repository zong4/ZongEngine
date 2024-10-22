#pragma once

#include "Hazel/Audio/Buffer/CircularBuffer.h"

#include <functional>

namespace Hazel::Audio
{
    //==============================================================================
    /** Request from readers for new data when close to empty.
        Or check at the end/beginning of the audio callback
        from the wrapper.
    */
    struct WaveSource
    {
        CircularBuffer<float, 1920> Channels;   // Interleaved sample data

        int64_t TotalFrames = 0;                // Total frames in the source to be set by the reader on the first read, used by Wave Player
        int64_t StartPosition = 0;              // Frame position in source to wrap around when reached the end of the source
        int64_t ReadPosition = 0;               // Frame position in source to read next time from (this is where this source is being read by a NodeProcessor)
        uint64_t WaveHandle = 0;                // Source Wave Asset handle
        const char* WaveName = nullptr;         // Wave Asset name for debugging purposes

        using RefillCallback = std::function<bool(WaveSource&)>;
        RefillCallback onRefill = nullptr;

        inline bool Refill() { return onRefill(*this); }

        inline void Clear() noexcept
        {
            Channels.Clear();

            TotalFrames = 0;
            ReadPosition = 0;
            WaveHandle = 0;
            WaveName = nullptr;
        }
    };

} // namespace Hazel::Audio
