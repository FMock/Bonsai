#include "UI/WaveformView.h"
#include "UI/Theme.h"

#include <cmath>

namespace bonsai
{
    WaveformView::WaveformView (std::shared_ptr<const Track> trackToShow,
                                TimelineHost& timelineHost,
                                juce::AudioFormatManager& formatManager,
                                juce::AudioThumbnailCache& thumbnailCache)
        : track (std::move (trackToShow)),
          host (timelineHost),
          thumbnail (thumbnailResolution, formatManager, thumbnailCache)
    {
        thumbnail.addChangeListener (this);

        // The track's audio is immutable and outlives this view (we hold a
        // shared_ptr), so the thumbnail can read it directly.
        thumbnail.setSource (&track->getAudio(), track->getSampleRate(), track->getId());

        setOpaque (true);
    }

    WaveformView::~WaveformView()
    {
        thumbnail.removeChangeListener (this);
        thumbnail.clear();
    }

    void WaveformView::changeListenerCallback (juce::ChangeBroadcaster*)
    {
        repaint();
    }

    void WaveformView::paint (juce::Graphics& g)
    {
        g.fillAll (theme::laneBackground);

        const auto& axis = host.getTimeAxis();
        const auto numChannels = track->getNumChannels();
        const auto clipStartX = axis.xAtTime (0.0);
        const auto clipEndX = axis.xAtTime (track->getLengthInSeconds());

        // Visible part of the clip, in whole pixels.
        const auto visibleStart = juce::jmax (0, (int) std::floor (clipStartX));
        const auto visibleEnd = juce::jmin (getWidth(), (int) std::ceil (clipEndX));

        if (visibleEnd > visibleStart)
        {
            const juce::Rectangle<float> clipRect ((float) clipStartX, 1.0f,
                                                   (float) (clipEndX - clipStartX), (float) getHeight() - 2.0f);
            g.setColour (theme::clipBackground);
            g.fillRoundedRectangle (clipRect, 3.0f);
            g.setColour (theme::clipBorder);
            g.drawRoundedRectangle (clipRect, 3.0f, 1.0f);
        }

        const auto laneHeight = getHeight() / numChannels;
        const auto samplesPerPixel = track->getSampleRate() / axis.pixelsPerSecond;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const juce::Rectangle<int> lane (0, ch * laneHeight, getWidth(), laneHeight);

            if (ch > 0)
            {
                g.setColour (theme::panelBorder);
                g.drawHorizontalLine (lane.getY(), 0.0f, (float) getWidth());
            }

            g.setColour (theme::centreLine);
            g.drawHorizontalLine (lane.getCentreY(), (float) visibleStart, (float) visibleEnd);

            if (visibleEnd <= visibleStart)
                continue;

            const auto waveArea = lane.reduced (0, 4);
            g.setColour (theme::waveform);

            if (samplesPerPixel >= thumbnailSamplesPerPixelThreshold)
            {
                thumbnail.drawChannel (g,
                                       waveArea.withX (visibleStart).withRight (visibleEnd),
                                       axis.timeAtX (visibleStart),
                                       axis.timeAtX (visibleEnd),
                                       ch,
                                       1.0f);
            }
            else
            {
                drawChannelFromSamples (g, waveArea, ch, axis);
            }
        }
    }

    void WaveformView::drawChannelFromSamples (juce::Graphics& g,
                                               juce::Rectangle<int> lane,
                                               int channel,
                                               const TimeAxis& axis) const
    {
        const auto& audio = track->getAudio();
        const auto* data = audio.getReadPointer (channel);
        const auto numSamples = (juce::int64) audio.getNumSamples();
        const auto sampleRate = track->getSampleRate();

        // Only draw the columns that actually need repainting.
        const auto area = g.getClipBounds().getIntersection (lane.withX (0).withWidth (getWidth()));

        if (area.isEmpty())
            return;

        const auto midY = (float) lane.getCentreY();
        const auto halfHeight = (float) lane.getHeight() * 0.5f;
        const auto yFor = [=] (float v) { return midY - juce::jlimit (-1.0f, 1.0f, v) * halfHeight; };

        const auto sampleAtX = [&] (double x) { return (juce::int64) std::floor (axis.timeAtX (x) * sampleRate); };

        if (sampleRate / axis.pixelsPerSecond >= 1.0)
        {
            // One or more samples per pixel: draw each column's min..max.
            for (int x = area.getX(); x < area.getRight(); ++x)
            {
                auto s0 = sampleAtX (x);
                auto s1 = juce::jmax (sampleAtX (x + 1), s0 + 1);
                s0 = juce::jmax<juce::int64> (s0, 0);
                s1 = juce::jmin (s1, numSamples);

                if (s0 >= s1)
                    continue;

                const auto range = juce::FloatVectorOperations::findMinAndMax (data + s0, (int) (s1 - s0));
                g.drawVerticalLine (x, yFor (range.getEnd()), yFor (range.getStart()) + 1.0f);
            }
        }
        else
        {
            // Several pixels per sample: join the sample points with lines.
            const auto first = juce::jmax<juce::int64> (0, sampleAtX (area.getX()) - 1);
            const auto last = juce::jmin (numSamples - 1, sampleAtX (area.getRight()) + 2);

            if (first > last)
                return;

            juce::Path path;
            const auto xFor = [&] (juce::int64 s) { return (float) axis.xAtTime ((double) s / sampleRate); };

            path.startNewSubPath (xFor (first), yFor (data[first]));
            for (auto s = first + 1; s <= last; ++s)
                path.lineTo (xFor (s), yFor (data[s]));

            g.strokePath (path, juce::PathStrokeType (1.5f));

            // Far enough in, show the individual samples as dots.
            if (axis.pixelsPerSecond / sampleRate >= 8.0)
                for (auto s = first; s <= last; ++s)
                    g.fillEllipse (xFor (s) - 2.0f, yFor (data[s]) - 2.0f, 4.0f, 4.0f);
        }
    }

    void WaveformView::mouseDown (const juce::MouseEvent& e)
    {
        host.selectTrack (*track);

        if (e.mods.isPopupMenu())
            host.showTrackMenu (*track);
        else
            host.seekToTime (host.getTimeAxis().timeAtX (e.position.x));
    }

    void WaveformView::mouseDrag (const juce::MouseEvent& e)
    {
        if (! e.mods.isPopupMenu())
            host.seekToTime (host.getTimeAxis().timeAtX (e.position.x));
    }

    void WaveformView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
    {
        if (! host.handleTimelineWheel (e, wheel))
            Component::mouseWheelMove (e, wheel);   // pass up so the track list scrolls
    }
}
