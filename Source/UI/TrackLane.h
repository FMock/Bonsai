#pragma once

#include "Engine/Track.h"
#include "UI/WaveformView.h"

namespace bonsai
{
    // One row of the timeline: a header on the left (name, MONO/STEREO, mute
    // and solo buttons, file details) and the waveform to its right.
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

        // Shows the track's current mute/solo state (buttons and waveform colour).
        void refreshMixState();

    private:
        // Speaker icon that toggles mute. When muted it shows a red circle
        // with a diagonal line over the speaker.
        class MuteButton final : public juce::Button
        {
        public:
            MuteButton();
            void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
        };

        // "S" button that toggles solo; lights up yellow when on.
        class SoloButton final : public juce::Button
        {
        public:
            SoloButton();
            void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
        };

        std::shared_ptr<const Track> track;
        TimelineHost& host;
        int trackNumber;
        bool selected = false;
        WaveformView waveform;
        MuteButton muteButton;
        SoloButton soloButton;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackLane)
    };
}
