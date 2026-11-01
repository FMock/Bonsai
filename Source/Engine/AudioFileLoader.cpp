#include "Engine/AudioFileLoader.h"

#include <cmath>
#include <limits>
#include <vector>

namespace bonsai
{
    AudioFileLoader::AudioFileLoader()
    {
        formatManager.registerBasicFormats();
    }

    LoadResult AudioFileLoader::load (const juce::File& file, double sessionSampleRate)
    {
        if (! file.existsAsFile())
            return { nullptr, file.getFileName() + ": file not found" };

        // createReaderFor returns a raw owning pointer in JUCE 9.
        std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

        if (reader == nullptr)
            return { nullptr, file.getFileName() + ": not a supported audio file" };

        const auto numChannels = (int) reader->numChannels;

        if (numChannels < 1 || numChannels > 2)
            return { nullptr, file.getFileName() + ": has " + juce::String (numChannels)
                                + " channels; only mono and stereo files are supported" };

        if (reader->lengthInSamples <= 0)
            return { nullptr, file.getFileName() + ": contains no audio" };

        if (reader->lengthInSamples > std::numeric_limits<int>::max())
            return { nullptr, file.getFileName() + ": is too long to load" };

        const auto numSamples = (int) reader->lengthInSamples;

        juce::AudioBuffer<float> audio (numChannels, numSamples);

        if (! reader->read (&audio, 0, numSamples, 0, true, numChannels > 1))
            return { nullptr, file.getFileName() + ": could not be read" };

        return makeTrack (file.getFileNameWithoutExtension(),
                          std::move (audio),
                          reader->sampleRate,
                          sessionSampleRate);
    }

    LoadResult AudioFileLoader::makeTrack (const juce::String& name,
                                           juce::AudioBuffer<float> audio,
                                           double audioSampleRate,
                                           double sessionSampleRate)
    {
        const auto numChannels = audio.getNumChannels();

        if (numChannels < 1 || numChannels > 2)
            return { nullptr, name + ": has " + juce::String (numChannels)
                                + " channels; only mono and stereo are supported" };

        if (audio.getNumSamples() == 0)
            return { nullptr, name + ": contains no audio" };

        if (audioSampleRate <= 0.0 || sessionSampleRate <= 0.0)
            return { nullptr, name + ": invalid sample rate" };

        if (! juce::approximatelyEqual (audioSampleRate, sessionSampleRate))
            audio = resample (audio, audioSampleRate, sessionSampleRate);

        return { std::make_shared<Track> (name, std::move (audio), sessionSampleRate, audioSampleRate), {} };
    }

    juce::AudioBuffer<float> AudioFileLoader::resample (const juce::AudioBuffer<float>& input,
                                                        double fromRate,
                                                        double toRate)
    {
        jassert (fromRate > 0.0 && toRate > 0.0);

        const auto numChannels = input.getNumChannels();
        const auto numInput = input.getNumSamples();

        // Input samples consumed per output sample.
        const auto speedRatio = fromRate / toRate;
        const auto numOutput = (int) std::ceil ((double) numInput / speedRatio);

        // The interpolator delays its output by a fixed number of *input*
        // samples. Generate that much extra output and drop it from the front so
        // the result lines up with the original.
        const auto latencyOutputSamples =
            juce::roundToInt (juce::WindowedSincInterpolator::getBaseLatency() / speedRatio);
        const auto numToGenerate = numOutput + latencyOutputSamples;

        juce::AudioBuffer<float> output (numChannels, numOutput);
        std::vector<float> scratch ((size_t) numToGenerate);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::WindowedSincInterpolator interpolator;

            // Past the end of the input the interpolator is fed zeros
            // (wrapAround = 0), which flushes the tail out cleanly.
            interpolator.process (speedRatio,
                                  input.getReadPointer (ch),
                                  scratch.data(),
                                  numToGenerate,
                                  numInput,
                                  0);

            output.copyFrom (ch, 0, scratch.data() + latencyOutputSamples, numOutput);
        }

        return output;
    }

    bool AudioFileLoader::canLoad (const juce::File& file) const
    {
        return file.existsAsFile()
            && formatManager.findFormatForFileExtension (file.getFileExtension()) != nullptr;
    }

    juce::String AudioFileLoader::getFileWildcard() const
    {
        return formatManager.getWildcardForAllFormats();
    }
}
