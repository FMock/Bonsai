#include "UI/TimeRuler.h"
#include "UI/Theme.h"

#include <cmath>

namespace bonsai
{
    namespace
    {
        // Pick the smallest "nice" interval that leaves room for a label.
        double chooseLabelInterval (double pixelsPerSecond)
        {
            static constexpr double intervals[] = { 0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5,
                                                    1.0, 2.0, 5.0, 10.0, 15.0, 30.0, 60.0, 120.0, 300.0, 600.0 };
            constexpr double minPixelsBetweenLabels = 90.0;

            for (auto interval : intervals)
                if (interval * pixelsPerSecond >= minPixelsBetweenLabels)
                    return interval;

            return 1800.0;
        }
    }

    TimeRuler::TimeRuler (TimelineHost& timelineHost) : host (timelineHost)
    {
        setOpaque (true);
    }

    juce::String TimeRuler::formatTime (double seconds, double tickInterval)
    {
        const auto totalMs = juce::roundToInt (seconds * 1000.0);
        const auto minutes = totalMs / 60000;
        const auto secs = (totalMs / 1000) % 60;
        const auto ms = totalMs % 1000;

        auto text = juce::String (minutes) + ":" + juce::String (secs).paddedLeft ('0', 2);

        if (tickInterval < 0.01)
            text << "." << juce::String (ms).paddedLeft ('0', 3);
        else if (tickInterval < 0.1)
            text << "." << juce::String (ms / 10).paddedLeft ('0', 2);
        else if (tickInterval < 1.0)
            text << "." << juce::String (ms / 100);

        return text;
    }

    void TimeRuler::paint (juce::Graphics& g)
    {
        g.fillAll (theme::panel);

        g.setColour (theme::panelBorder);
        g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());

        const auto& axis = host.getTimeAxis();
        const auto labelInterval = chooseLabelInterval (axis.pixelsPerSecond);
        const auto minorInterval = labelInterval / 5.0;

        const auto startTime = juce::jmax (0.0, axis.timeAtX (0.0));
        const auto endTime = axis.timeAtX (getWidth());
        const auto firstTick = (juce::int64) std::floor (startTime / minorInterval);
        const auto lastTick = (juce::int64) std::ceil (endTime / minorInterval);

        g.setFont (juce::FontOptions (11.0f));

        for (auto i = firstTick; i <= lastTick; ++i)
        {
            const auto t = (double) i * minorInterval;
            const auto x = (float) axis.xAtTime (t);
            const auto isMajor = (i % 5) == 0;

            g.setColour (theme::rulerTick);
            g.drawVerticalLine (juce::roundToInt (x),
                                (float) getHeight() - (isMajor ? 12.0f : 5.0f),
                                (float) getHeight() - 1.0f);

            if (isMajor)
            {
                g.setColour (theme::rulerText);
                g.drawText (formatTime (t, labelInterval),
                            juce::roundToInt (x) + 3, 2, 80, getHeight() - 12,
                            juce::Justification::centredLeft, false);
            }
        }
    }

    void TimeRuler::mouseDown (const juce::MouseEvent& e)
    {
        host.seekToTime (host.getTimeAxis().timeAtX (e.position.x));
    }

    void TimeRuler::mouseDrag (const juce::MouseEvent& e)
    {
        host.seekToTime (host.getTimeAxis().timeAtX (e.position.x));
    }

    void TimeRuler::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
    {
        if (! host.handleTimelineWheel (e, wheel))
            Component::mouseWheelMove (e, wheel);
    }
}
