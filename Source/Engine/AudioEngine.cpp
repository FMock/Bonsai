#include "Engine/AudioEngine.h"

namespace bonsai
{
    AudioEngine::AudioEngine() = default;

    AudioEngine::~AudioEngine()
    {
        deviceManager.removeAudioCallback (&sourcePlayer);
        sourcePlayer.setSource (nullptr);
    }

    juce::String AudioEngine::initialise()
    {
        // No inputs for now; recording will ask for them later.
        const auto error = deviceManager.initialiseWithDefaultDevices (0, 2);

        sourcePlayer.setSource (&player);
        deviceManager.addAudioCallback (&sourcePlayer);

        if (error.isNotEmpty())
            return error;

        if (deviceManager.getCurrentAudioDevice() == nullptr)
            return "No audio output device is available.";

        return {};
    }

    double AudioEngine::getSampleRate() const
    {
        if (auto* device = deviceManager.getCurrentAudioDevice())
        {
            const auto rate = device->getCurrentSampleRate();

            if (rate > 0.0)
                return rate;
        }

        return fallbackSampleRate;
    }
}
