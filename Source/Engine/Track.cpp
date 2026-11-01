#include "Engine/Track.h"

namespace bonsai
{
    namespace
    {
        juce::int64 nextTrackId()
        {
            static std::atomic<juce::int64> counter { 1 };
            return counter.fetch_add (1);
        }
    }

    Track::Track (juce::String trackName,
                  juce::AudioBuffer<float> trackAudio,
                  double rate,
                  double sourceRate)
        : name (std::move (trackName)),
          audio (std::move (trackAudio)),
          sampleRate (rate),
          sourceSampleRate (sourceRate),
          id (nextTrackId())
    {
        jassert (audio.getNumChannels() == 1 || audio.getNumChannels() == 2);
        jassert (sampleRate > 0.0);
    }
}
