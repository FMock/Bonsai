#include "UI/TransportBar.h"
#include "UI/Theme.h"

namespace bonsai
{
    //==========================================================================
    juce::Path TransportBar::IconButton::createIcon (Icon icon, juce::Rectangle<float> area)
    {
        const auto px = [area] (float fx) { return area.getX() + fx * area.getWidth(); };
        const auto py = [area] (float fy) { return area.getY() + fy * area.getHeight(); };

        juce::Path path;

        switch (icon)
        {
            case Icon::play:
                // Triangle pointing right, nudged so it looks centred.
                path.addTriangle (px (0.20f), py (0.08f), px (0.20f), py (0.92f), px (0.92f), py (0.50f));
                break;

            case Icon::pause:
                // Two thin bars.
                path.addRectangle (px (0.18f), py (0.10f), area.getWidth() * 0.22f, area.getHeight() * 0.80f);
                path.addRectangle (px (0.60f), py (0.10f), area.getWidth() * 0.22f, area.getHeight() * 0.80f);
                break;

            case Icon::stop:
                // Square.
                path.addRectangle (px (0.14f), py (0.14f), area.getWidth() * 0.72f, area.getHeight() * 0.72f);
                break;

            case Icon::trash:
                // Handle on the lid.
                path.startNewSubPath (px (0.38f), py (0.17f));
                path.lineTo (px (0.38f), py (0.06f));
                path.lineTo (px (0.62f), py (0.06f));
                path.lineTo (px (0.62f), py (0.17f));

                // Lid.
                path.startNewSubPath (px (0.08f), py (0.17f));
                path.lineTo (px (0.92f), py (0.17f));

                // Can, slightly narrower at the bottom.
                path.startNewSubPath (px (0.18f), py (0.28f));
                path.lineTo (px (0.25f), py (0.95f));
                path.lineTo (px (0.75f), py (0.95f));
                path.lineTo (px (0.82f), py (0.28f));

                // Ribs.
                for (auto fx : { 0.38f, 0.50f, 0.62f })
                {
                    path.startNewSubPath (px (fx), py (0.40f));
                    path.lineTo (px (fx), py (0.83f));
                }
                break;
        }

        return path;
    }

    void TransportBar::IconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
    {
        // Same background as the other buttons (the look-and-feel dims it when
        // disabled). The "on" colour shows while the button is toggled on.
        const auto background = findColour (getToggleState() ? juce::TextButton::buttonOnColourId
                                                              : juce::TextButton::buttonColourId);
        getLookAndFeel().drawButtonBackground (g, *this, background, highlighted, down);

        const auto bounds = getLocalBounds().toFloat();
        const auto scale = icon == Icon::trash ? 0.52f : 0.40f;
        const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight()) * scale;
        const auto iconArea = bounds.withSizeKeepingCentre (size, size).translated (0.0f, down ? 1.0f : 0.0f);

        juce::Colour colour = theme::text;

        if (! isEnabled())
            colour = colour.withAlpha (0.3f);
        else if (highlighted || down)
            colour = hoverColour;

        g.setColour (colour);
        const auto path = createIcon (icon, iconArea);

        if (icon == Icon::trash)
            g.strokePath (path, juce::PathStrokeType (juce::jmax (1.5f, size * 0.09f),
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        else
            g.fillPath (path);
    }

    //==========================================================================
    TransportBar::TransportBar()
    {
        for (auto* button : std::initializer_list<juce::Button*> { &loadButton, &deleteButton, &playButton, &pauseButton, &stopButton })
        {
            // Keep keyboard focus on the main window so Space always means
            // play/pause instead of "click the focused button".
            button->setWantsKeyboardFocus (false);
            addAndMakeVisible (button);
        }

        loadButton.setTooltip ("Load audio files as new tracks (Ctrl+O)");
        deleteButton.setTooltip ("Delete the selected track (Delete)");
        playButton.setTooltip ("Play (Space)");
        pauseButton.setTooltip ("Pause (Space)");
        stopButton.setTooltip ("Stop and return to the start (Enter / Home)");

        playButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3f7a6a));
        playButton.setClickingTogglesState (false);

        loadButton.onClick   = [this] { if (onLoadAudio) onLoadAudio(); };
        deleteButton.onClick = [this] { if (onDeleteTrack) onDeleteTrack(); };
        playButton.onClick  = [this] { if (onPlay) onPlay(); };
        pauseButton.onClick = [this] { if (onPause) onPause(); };
        stopButton.onClick  = [this] { if (onStop) onStop(); };

        timeLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 24.0f, juce::Font::bold));
        timeLabel.setJustificationType (juce::Justification::centred);
        timeLabel.setColour (juce::Label::textColourId, theme::playhead);
        timeLabel.setColour (juce::Label::backgroundColourId, theme::background);
        timeLabel.setColour (juce::Label::outlineColourId, theme::panelBorder);
        addAndMakeVisible (timeLabel);

        infoLabel.setFont (juce::FontOptions (12.0f));
        infoLabel.setJustificationType (juce::Justification::centredRight);
        infoLabel.setColour (juce::Label::textColourId, theme::dimText);
        addAndMakeVisible (infoLabel);

        update (TransportState::Stopped, 0.0, false, false);
    }

    juce::String TransportBar::formatTime (double seconds)
    {
        const auto totalMs = (juce::int64) std::floor (juce::jmax (0.0, seconds) * 1000.0);
        const auto minutes = totalMs / 60000;
        const auto secs = (int) ((totalMs / 1000) % 60);
        const auto ms = (int) (totalMs % 1000);

        return juce::String (minutes) + ":" + juce::String (secs).paddedLeft ('0', 2)
             + "." + juce::String (ms).paddedLeft ('0', 3);
    }

    void TransportBar::update (TransportState state, double positionSeconds, bool hasTracks, bool hasSelectedTrack)
    {
        const auto playing = state == TransportState::Playing;

        deleteButton.setEnabled (hasSelectedTrack);
        playButton.setEnabled (hasTracks && ! playing);
        pauseButton.setEnabled (playing);
        stopButton.setEnabled (hasTracks && (state != TransportState::Stopped || positionSeconds > 0.0));
        playButton.setToggleState (playing, juce::dontSendNotification);

        timeLabel.setText (formatTime (positionSeconds), juce::dontSendNotification);
    }

    void TransportBar::setInfoText (const juce::String& text)
    {
        infoLabel.setText (text, juce::dontSendNotification);
    }

    void TransportBar::paint (juce::Graphics& g)
    {
        g.fillAll (theme::panel);
        g.setColour (theme::panelBorder);
        g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
        g.drawText ("Bonsai", 14, 0, 100, getHeight(), juce::Justification::centredLeft);
    }

    void TransportBar::resized()
    {
        auto area = getLocalBounds().reduced (10, 10);

        area.removeFromLeft (100);   // app name
        loadButton.setBounds (area.removeFromLeft (110));
        area.removeFromLeft (6);
        deleteButton.setBounds (area.removeFromLeft (area.getHeight() + 4));
        area.removeFromLeft (24);

        const auto transportWidth = area.getHeight() + 14;
        playButton.setBounds (area.removeFromLeft (transportWidth));
        area.removeFromLeft (6);
        pauseButton.setBounds (area.removeFromLeft (transportWidth));
        area.removeFromLeft (6);
        stopButton.setBounds (area.removeFromLeft (transportWidth));
        area.removeFromLeft (24);

        timeLabel.setBounds (area.removeFromLeft (160));
        area.removeFromLeft (12);

        infoLabel.setBounds (area);
    }
}
