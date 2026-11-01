#include "UI/TrackLane.h"
#include "UI/Theme.h"

namespace bonsai
{
    TrackLane::TrackLane (std::shared_ptr<const Track> trackToShow,
                          int number,
                          TimelineHost& timelineHost,
                          juce::AudioFormatManager& formatManager,
                          juce::AudioThumbnailCache& thumbnailCache)
        : track (trackToShow),
          host (timelineHost),
          trackNumber (number),
          waveform (std::move (trackToShow), timelineHost, formatManager, thumbnailCache)
    {
        addAndMakeVisible (waveform);
    }

    void TrackLane::setSelected (bool shouldBeSelected)
    {
        if (selected != shouldBeSelected)
        {
            selected = shouldBeSelected;
            repaint();
        }
    }

    void TrackLane::setTrackNumber (int number)
    {
        if (trackNumber != number)
        {
            trackNumber = number;
            repaint();
        }
    }

    void TrackLane::mouseDown (const juce::MouseEvent& e)
    {
        host.selectTrack (*track);

        if (e.mods.isPopupMenu())
            host.showTrackMenu (*track);
    }

    void TrackLane::paintOverChildren (juce::Graphics& g)
    {
        if (! selected)
            return;

        // Outline the whole lane, header and waveform, in the accent colour.
        g.setColour (theme::selection);
        g.drawRect (getLocalBounds().withTrimmedBottom (1), 2);
    }

    int TrackLane::getPreferredHeight() const noexcept
    {
        return track->isStereo() ? 150 : 90;
    }

    void TrackLane::paint (juce::Graphics& g)
    {
        auto header = getLocalBounds().withWidth (headerWidth);

        g.setColour (selected ? theme::trackHeaderSelected : theme::trackHeader);
        g.fillRect (header);

        g.setColour (theme::panelBorder);
        g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());
        g.drawVerticalLine (headerWidth - 1, 0.0f, (float) getHeight());

        auto content = header.reduced (10, 8);

        // Name, with the track number in front.
        auto nameRow = content.removeFromTop (20);
        g.setColour (theme::dimText);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (juce::String (trackNumber), nameRow.removeFromLeft (20), juce::Justification::centredLeft);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawText (track->getName(), nameRow, juce::Justification::centredLeft, true);

        content.removeFromTop (6);

        // MONO / STEREO badge.
        const auto stereo = track->isStereo();
        auto badge = content.removeFromTop (18).withWidth (stereo ? 58 : 46);
        g.setColour (stereo ? theme::stereoBadge : theme::monoBadge);
        g.fillRoundedRectangle (badge.toFloat(), 3.0f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (stereo ? "STEREO" : "MONO", badge, juce::Justification::centred);

        content.removeFromTop (6);

        // File details.
        g.setColour (theme::dimText);
        g.setFont (juce::FontOptions (12.0f));

        auto details = juce::String (track->getSourceSampleRate() / 1000.0, 1) + " kHz";
        if (! juce::approximatelyEqual (track->getSourceSampleRate(), track->getSampleRate()))
            details << " (resampled)";

        const auto seconds = track->getLengthInSeconds();
        details << "  |  " << juce::String ((int) seconds / 60) << ":"
                << juce::String ((int) seconds % 60).paddedLeft ('0', 2);

        g.drawText (details, content.removeFromTop (16), juce::Justification::centredLeft, true);

        // L / R markers next to the waveform lanes.
        if (stereo)
        {
            const auto laneHeight = waveform.getHeight() / 2;
            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawText ("L", headerWidth - 18, waveform.getY(), 14, laneHeight, juce::Justification::centred);
            g.drawText ("R", headerWidth - 18, waveform.getY() + laneHeight, 14, laneHeight, juce::Justification::centred);
        }
    }

    void TrackLane::resized()
    {
        waveform.setBounds (getLocalBounds().withTrimmedLeft (headerWidth).withTrimmedBottom (1));
    }
}
