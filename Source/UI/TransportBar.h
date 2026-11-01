#pragma once

#include "Engine/SessionPlayer.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace bonsai
{
    // Top bar: Load Audio, transport buttons, the time display, and the
    // session/device info.
    class TransportBar final : public juce::Component
    {
    public:
        TransportBar();

        std::function<void()> onLoadAudio;
        std::function<void()> onDeleteTrack;   // delete the selected track
        std::function<void()> onPlay;
        std::function<void()> onPause;
        std::function<void()> onStop;

        // Refresh the buttons and time display. Cheap enough to call on a timer.
        void update (TransportState state, double positionSeconds, bool hasTracks, bool hasSelectedTrack);

        void setInfoText (const juce::String& text);

        static juce::String formatTime (double seconds);   // m:ss.mmm

        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        // A square button showing a trash can, styled like the TextButtons.
        class TrashButton final : public juce::Button
        {
        public:
            TrashButton() : juce::Button ("Delete Track") {}

            void paintButton (juce::Graphics& g, bool highlighted, bool down) override;

            static juce::Path createTrashCanIcon (juce::Rectangle<float> area);
        };

        juce::TextButton loadButton { "Load Audio..." };
        TrashButton deleteButton;
        juce::TextButton playButton { "Play" };
        juce::TextButton pauseButton { "Pause" };
        juce::TextButton stopButton { "Stop" };
        juce::Label timeLabel;
        juce::Label infoLabel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportBar)
    };
}
