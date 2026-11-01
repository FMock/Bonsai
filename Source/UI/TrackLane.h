#pragma once

#include "Engine/Track.h"
#include "UI/WaveformView.h"

namespace bonsai
{
    // One row of the timeline: a header on the left (name, MONO/STEREO, file
    // details) and the waveform to its right.
    class TrackLane final : public juce::Component
    {
    public:
        static constexpr int headerWidth = 180;

        TrackLane (std::shared_ptr<const Track> track,
                   int trackNumber,
                   TimelineHost& host,
                   juce::AudioFormatManager& formatManager,
                   juce::AudioThumbnailCache& thumbnailCache);

        // Stereo tracks are taller so each channel gets a usable lane.
        int getPreferredHeight() const noexcept;

        void paint (juce::Graphics& g) override;
        void paintOverChildren (juce::Graphics& g) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent& e) override;   // clicks on the header

        WaveformView& getWaveformView() noexcept                { return waveform; }
        const std::shared_ptr<const Track>& getTrack() const    { return track; }

        void setSelected (bool shouldBeSelected);
        bool isSelected() const noexcept                        { return selected; }

        void setTrackNumber (int number);

    private:
        std::shared_ptr<const Track> track;
        TimelineHost& host;
        int trackNumber;
        bool selected = false;
        WaveformView waveform;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackLane)
    };
}
