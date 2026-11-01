#pragma once

#include "Engine/Track.h"
#include "UI/TimelineHost.h"

#include <juce_audio_utils/juce_audio_utils.h>

#include <memory>

namespace bonsai
{
    // Draws one track's audio across the timeline: a clip block with one
    // waveform lane per channel (stereo tracks show L above R).
    //
    // Zoomed out, it uses juce::AudioThumbnail's cached overview. Zoomed in far
    // enough that the overview would look blocky, it draws straight from the
    // samples, down to individual sample points.
    class WaveformView final : public juce::Component,
                               private juce::ChangeListener
    {
    public:
        WaveformView (std::shared_ptr<const Track> track,
                      TimelineHost& host,
                      juce::AudioFormatManager& formatManager,
                      juce::AudioThumbnailCache& thumbnailCache);

        ~WaveformView() override;

        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    private:
        void changeListenerCallback (juce::ChangeBroadcaster*) override;

        void drawChannelFromSamples (juce::Graphics& g,
                                     juce::Rectangle<int> lane,
                                     int channel,
                                     const TimeAxis& axis) const;

        // Below this many samples per pixel we draw from the samples directly.
        static constexpr double thumbnailSamplesPerPixelThreshold = 128.0;
        static constexpr int thumbnailResolution = 128;   // source samples per thumbnail sample

        std::shared_ptr<const Track> track;
        TimelineHost& host;
        juce::AudioThumbnail thumbnail;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformView)
    };
}
