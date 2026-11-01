#pragma once

#include "Engine/SessionPlayer.h"

#include <juce_audio_devices/juce_audio_devices.h>

namespace bonsai
{
    // Connects the SessionPlayer to the system's audio output device.
    //
    // Later, recording will add input channels here and a recording path
    // alongside the player.
    class AudioEngine
    {
    public:
        AudioEngine();
        ~AudioEngine();

        // Opens the default output device. Returns an empty string on success,
        // otherwise a description of the problem (the app still runs, silently).
        juce::String initialise();

        // The session sample rate: the device's rate, or a default when no
        // device could be opened.
        double getSampleRate() const;

        SessionPlayer& getPlayer() noexcept                     { return player; }
        juce::AudioDeviceManager& getDeviceManager() noexcept   { return deviceManager; }

    private:
        static constexpr double fallbackSampleRate = 48000.0;

        juce::AudioDeviceManager deviceManager;
        juce::AudioSourcePlayer sourcePlayer;
        SessionPlayer player;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
    };
}
