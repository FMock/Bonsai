#pragma once

#include "Engine/SessionPlayer.h"
#include "UI/Theme.h"

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
        // A square button that draws a symbol instead of text, styled like
        // the TextButtons.
        class IconButton final : public juce::Button
        {
        public:
            enum class Icon { trash, play, pause, stop };

            IconButton (const juce::String& name, Icon iconToUse, juce::Colour hoverColourToUse)
                : juce::Button (name), icon (iconToUse), hoverColour (hoverColourToUse) {}

            void paintButton (juce::Graphics& g, bool highlighted, bool down) override;

            static juce::Path createIcon (Icon icon, juce::Rectangle<float> area);

        private:
            Icon icon;
            juce::Colour hoverColour;
        };

        juce::TextButton loadButton { "Load Audio..." };
        IconButton deleteButton { "Delete Track", IconButton::Icon::trash, theme::danger };
        IconButton playButton   { "Play",  IconButton::Icon::play,  theme::playhead };
        IconButton pauseButton  { "Pause", IconButton::Icon::pause, theme::playhead };
        IconButton stopButton   { "Stop",  IconButton::Icon::stop,  theme::playhead };
        juce::Label timeLabel;
        juce::Label infoLabel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportBar)
    };
}
