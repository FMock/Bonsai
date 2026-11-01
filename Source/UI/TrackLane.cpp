#include "UI/TrackLane.h"
#include "UI/Theme.h"

namespace bonsai
{
    //==========================================================================
    TrackLane::MuteButton::MuteButton() : juce::Button ("Mute")
    {
        setClickingTogglesState (true);
        setWantsKeyboardFocus (false);   // keep Space for play/pause
    }

    void TrackLane::MuteButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
    {
        const auto muted = getToggleState();
        auto bounds = getLocalBounds().toFloat();

        if (highlighted || down)
        {
            g.setColour (juce::Colours::white.withAlpha (down ? 0.15f : 0.08f));
            g.fillRoundedRectangle (bounds, 3.0f);
        }

        // Speaker, in a square area centred in the button.
        const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.8f;
        const auto area = bounds.withSizeKeepingCentre (size, size);
        const auto px = [area] (float fx) { return area.getX() + fx * area.getWidth(); };
        const auto py = [area] (float fy) { return area.getY() + fy * area.getHeight(); };

        juce::Path speaker;
        speaker.startNewSubPath (px (0.10f), py (0.38f));
        speaker.lineTo (px (0.28f), py (0.38f));
        speaker.lineTo (px (0.52f), py (0.16f));
        speaker.lineTo (px (0.52f), py (0.84f));
        speaker.lineTo (px (0.28f), py (0.62f));
        speaker.lineTo (px (0.10f), py (0.62f));
        speaker.closeSubPath();

        g.setColour (muted ? theme::dimText : theme::text);
        g.fillPath (speaker);

        const auto stroke = juce::jmax (1.2f, size * 0.07f);

        if (! muted)
        {
            // Sound waves.
            juce::Path waves;
            waves.addCentredArc (px (0.52f), py (0.50f), size * 0.16f, size * 0.16f, 0.0f,
                                 juce::MathConstants<float>::pi * 0.25f, juce::MathConstants<float>::pi * 0.75f, true);
            waves.addCentredArc (px (0.52f), py (0.50f), size * 0.32f, size * 0.32f, 0.0f,
                                 juce::MathConstants<float>::pi * 0.22f, juce::MathConstants<float>::pi * 0.78f, true);
            g.strokePath (waves, juce::PathStrokeType (stroke, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
            return;
        }

        // Muted: red circle with a diagonal line over the speaker.
        const auto circle = area.reduced (size * 0.02f);
        const auto ring = juce::jmax (1.5f, size * 0.09f);
        g.setColour (theme::muteRed);
        g.drawEllipse (circle.reduced (ring * 0.5f), ring);

        const auto r = circle.getWidth() * 0.5f - ring * 0.5f;
        const auto d = r * 0.7071f;   // 45 degrees
        const auto c = circle.getCentre();
        g.drawLine (c.x - d, c.y - d, c.x + d, c.y + d, ring);
    }

    //==========================================================================
    TrackLane::SoloButton::SoloButton() : juce::Button ("Solo")
    {
        setClickingTogglesState (true);
        setWantsKeyboardFocus (false);   // keep Space for play/pause
    }

    void TrackLane::SoloButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
    {
        const auto soloed = getToggleState();
        const auto box = getLocalBounds().toFloat().reduced (2.0f);

        if (soloed)
        {
            g.setColour (theme::soloYellow.brighter (highlighted ? 0.15f : 0.0f));
            g.fillRoundedRectangle (box, 3.0f);
        }
        else
        {
            g.setColour (juce::Colours::white.withAlpha (down ? 0.15f : highlighted ? 0.08f : 0.0f));
            g.fillRoundedRectangle (box, 3.0f);
            g.setColour (theme::dimText);
            g.drawRoundedRectangle (box.reduced (0.5f), 3.0f, 1.0f);
        }

        g.setColour (soloed ? theme::laneBackground : theme::text);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText ("S", box.translated (0.0f, down ? 1.0f : 0.0f), juce::Justification::centred);
    }

    //==========================================================================
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

        muteButton.onClick = [this]
        {
            track->setMuted (muteButton.getToggleState());
            host.mixStateChanged();
        };
        addAndMakeVisible (muteButton);

        soloButton.onClick = [this]
        {
            track->setSoloed (soloButton.getToggleState());
            host.mixStateChanged();   // other tracks go silent or come back
        };
        addAndMakeVisible (soloButton);

        refreshMixState();
    }

    void TrackLane::refreshMixState()
    {
        muteButton.setToggleState (track->isMuted(), juce::dontSendNotification);
        muteButton.setTooltip (track->isMuted() ? "Unmute this track" : "Mute this track");
        soloButton.setToggleState (track->isSoloed(), juce::dontSendNotification);
        soloButton.setTooltip (track->isSoloed() ? "Unsolo this track"
                                                 : "Solo this track (only soloed tracks are heard)");
        waveform.repaint();
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

        // Same row as the MONO/STEREO badge, just to its right (see paint()).
        muteButton.setBounds (10 + 58 + 8, 8 + 20 + 6 - 3, 24, 24);
        soloButton.setBounds (muteButton.getRight() + 4, muteButton.getY(), 24, 24);
    }
}
