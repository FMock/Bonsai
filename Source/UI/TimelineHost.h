#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bonsai
{
    class Track;

    // Maps time to horizontal pixels in the timeline's time area. x = 0 is the
    // left edge of the time area (just right of the track headers).
    struct TimeAxis
    {
        double startSeconds = 0.0;       // time at the left edge
        double pixelsPerSecond = 50.0;   // zoom

        double timeAtX (double x) const noexcept  { return startSeconds + x / pixelsPerSecond; }
        double xAtTime (double t) const noexcept  { return (t - startSeconds) * pixelsPerSecond; }
        double secondsForWidth (int width) const noexcept { return width / pixelsPerSecond; }
    };

    // What the ruler and waveform views need from the timeline that owns them.
    class TimelineHost
    {
    public:
        virtual ~TimelineHost() = default;

        virtual const TimeAxis& getTimeAxis() const = 0;

        // Move the playhead to a time (from a click or drag).
        virtual void seekToTime (double seconds) = 0;

        // Ctrl+wheel zooms, Shift+wheel / horizontal wheel scrolls in time.
        // Returns false for plain vertical wheel moves, which the caller should
        // pass on so the track list scrolls.
        virtual bool handleTimelineWheel (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) = 0;

        // Highlight a track (from a click on its header or waveform).
        virtual void selectTrack (const Track& track) = 0;

        // Show the right-click menu for a track (Delete Track, ...).
        virtual void showTrackMenu (const Track& track) = 0;
    };
}
