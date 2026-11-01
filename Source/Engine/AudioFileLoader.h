#pragma once

#include "Engine/Track.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <memory>

namespace bonsai
{
    struct LoadResult
    {
        std::shared_ptr<Track> track;   // null on failure
        juce::String error;             // set on failure

        bool succeeded() const noexcept { return track != nullptr; }
    };

    // Reads audio files fully into memory and turns them into Tracks at the
    // session sample rate. Mono files become mono tracks, stereo files become
    // stereo tracks; anything with more channels is rejected.
    class AudioFileLoader
    {
    public:
        AudioFileLoader();

        LoadResult load (const juce::File& file, double sessionSampleRate);

        // Builds a track from audio that's already in memory, resampling it to
        // the session rate if needed. load() ends up here; it's public so the
        // rules can be tested without files.
        static LoadResult makeTrack (const juce::String& name,
                                     juce::AudioBuffer<float> audio,
                                     double audioSampleRate,
                                     double sessionSampleRate);

        // High-quality (windowed sinc) offline resampling. The output is
        // time-aligned with the input: the interpolator's latency is removed.
        static juce::AudioBuffer<float> resample (const juce::AudioBuffer<float>& input,
                                                  double fromRate,
                                                  double toRate);

        bool canLoad (const juce::File& file) const;
        juce::String getFileWildcard() const;     // e.g. "*.wav;*.aiff;..."
        juce::AudioFormatManager& getFormatManager() noexcept { return formatManager; }

    private:
        juce::AudioFormatManager formatManager;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFileLoader)
    };
}
