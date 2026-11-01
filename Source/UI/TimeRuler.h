#pragma once

#include "UI/TimelineHost.h"

namespace bonsai
{
    // The time scale above the tracks. Tick spacing adapts to the zoom level.
    // Clicking or dragging on it moves the playhead.
    class TimeRuler final : public juce::Component
    {
    public:
        explicit TimeRuler (TimelineHost& host);

        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

        static juce::String formatTime (double seconds, double tickInterval);

    private:
        TimelineHost& host;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeRuler)
    };
}
