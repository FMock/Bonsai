#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>

namespace bonsai
{
    // One track in the session: a mono or stereo block of audio held in memory
    // at the session sample rate.
    //
    // The audio itself never changes after construction, so the audio thread
    // can read it without locking. Only the gain is adjustable, and it's atomic.
    class Track
    {
    public:
        Track (juce::String name,
               juce::AudioBuffer<float> audio,
               double sampleRate,
               double sourceSampleRate);

        const juce::String& getName() const noexcept            { return name; }
        int getNumChannels() const noexcept                      { return audio.getNumChannels(); }
        bool isStereo() const noexcept                           { return getNumChannels() == 2; }
        juce::int64 getLengthInSamples() const noexcept          { return audio.getNumSamples(); }
        double getLengthInSeconds() const noexcept               { return (double) audio.getNumSamples() / sampleRate; }
        const juce::AudioBuffer<float>& getAudio() const noexcept { return audio; }

        // Rate the audio is stored at (the session rate when it was loaded).
        double getSampleRate() const noexcept                    { return sampleRate; }

        // Rate of the original file, before any resampling.
        double getSourceSampleRate() const noexcept              { return sourceSampleRate; }

        // Unique per track; used as the waveform cache key.
        juce::int64 getId() const noexcept                       { return id; }

        float getGain() const noexcept                           { return gain.load(); }
        void setGain (float newGain) noexcept                    { gain.store (newGain); }

    private:
        const juce::String name;
        const juce::AudioBuffer<float> audio;
        const double sampleRate;
        const double sourceSampleRate;
        const juce::int64 id;
        std::atomic<float> gain { 1.0f };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Track)
    };
}
