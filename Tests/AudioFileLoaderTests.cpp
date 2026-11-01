#include "Engine/AudioFileLoader.h"

namespace bonsai
{
    class AudioFileLoaderTests final : public juce::UnitTest
    {
    public:
        AudioFileLoaderTests() : juce::UnitTest ("AudioFileLoader", "Bonsai") {}

        void runTest() override
        {
            beginTest ("Mono audio makes a mono track, stereo makes stereo");
            {
                auto mono = AudioFileLoader::makeTrack ("m", juce::AudioBuffer<float> (1, 100), 48000.0, 48000.0);
                auto stereo = AudioFileLoader::makeTrack ("s", juce::AudioBuffer<float> (2, 100), 48000.0, 48000.0);

                expect (mono.succeeded() && ! mono.track->isStereo());
                expect (stereo.succeeded() && stereo.track->isStereo());
            }

            beginTest ("More than two channels is rejected");
            {
                auto result = AudioFileLoader::makeTrack ("surround", juce::AudioBuffer<float> (6, 100), 48000.0, 48000.0);
                expect (! result.succeeded());
                expect (result.error.contains ("6 channels"));
            }

            beginTest ("Empty audio is rejected");
            {
                auto result = AudioFileLoader::makeTrack ("empty", juce::AudioBuffer<float> (1, 0), 48000.0, 48000.0);
                expect (! result.succeeded());
            }

            beginTest ("Resampling changes the length by the rate ratio");
            {
                auto result = AudioFileLoader::makeTrack ("r", juce::AudioBuffer<float> (2, 44100), 44100.0, 48000.0);
                expect (result.succeeded());
                expectEquals (result.track->getLengthInSamples(), (juce::int64) 48000);
                expectEquals (result.track->getSampleRate(), 48000.0);
                expectEquals (result.track->getSourceSampleRate(), 44100.0);
            }

            beginTest ("Resampling keeps audio time-aligned");
            {
                // An impulse 1000 samples in at 44.1 kHz should land ~1088 samples
                // in at 48 kHz (same moment in time).
                juce::AudioBuffer<float> impulse (1, 4410);
                impulse.clear();
                impulse.setSample (0, 1000, 1.0f);

                const auto out = AudioFileLoader::resample (impulse, 44100.0, 48000.0);
                const auto* data = out.getReadPointer (0);

                int peakIndex = 0;
                for (int i = 1; i < out.getNumSamples(); ++i)
                    if (std::abs (data[i]) > std::abs (data[peakIndex]))
                        peakIndex = i;

                const auto expectedIndex = 1000.0 * 48000.0 / 44100.0;
                expectWithinAbsoluteError ((double) peakIndex, expectedIndex, 1.5);
            }

            beginTest ("Resampling preserves a sine's level");
            {
                juce::AudioBuffer<float> sine (1, 44100);
                for (int n = 0; n < sine.getNumSamples(); ++n)
                    sine.setSample (0, n, 0.5f * std::sin (juce::MathConstants<float>::twoPi * 1000.0f * (float) n / 44100.0f));

                const auto out = AudioFileLoader::resample (sine, 44100.0, 48000.0);

                // Ignore the edges, where the filter ramps in and out.
                const auto rms = out.getRMSLevel (0, 2000, out.getNumSamples() - 4000);
                expectWithinAbsoluteError (rms, 0.5f / std::sqrt (2.0f), 0.01f);
            }
        }
    };

    static AudioFileLoaderTests audioFileLoaderTests;
}
