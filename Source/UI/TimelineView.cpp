#include "UI/TimelineView.h"
#include "UI/Theme.h"

#include <cmath>

namespace bonsai
{
    //==========================================================================
    TimelineView::PlayheadOverlay::PlayheadOverlay (const TimelineView& ownerView) : owner (ownerView)
    {
        setInterceptsMouseClicks (false, false);
    }

    void TimelineView::PlayheadOverlay::moveTo (double seconds)
    {
        positionSeconds = seconds;
        const auto x = juce::roundToInt (owner.axis.xAtTime (seconds));

        if (x == lastX)
            return;

        // Repaint just the old and new strips, not the whole timeline.
        repaint (lastX - 6, 0, 13, getHeight());
        repaint (x - 6, 0, 13, getHeight());
        lastX = x;
    }

    void TimelineView::PlayheadOverlay::paint (juce::Graphics& g)
    {
        const auto x = (float) owner.axis.xAtTime (positionSeconds);
        lastX = juce::roundToInt (x);

        if (x < -6.0f || x > (float) getWidth() + 6.0f)
            return;

        g.setColour (theme::playhead);
        g.fillRect (x - 0.75f, 0.0f, 1.5f, (float) getHeight());

        // Marker in the ruler.
        juce::Path marker;
        marker.addTriangle (x - 6.0f, 0.0f, x + 6.0f, 0.0f, x, 9.0f);
        g.fillPath (marker);
    }

    //==========================================================================
    TimelineView::TimelineView (juce::AudioFormatManager& fm, juce::AudioThumbnailCache& cache)
        : formatManager (fm),
          thumbnailCache (cache)
    {
        addAndMakeVisible (ruler);

        viewport.setViewedComponent (&laneContainer, false);
        viewport.setScrollBarsShown (true, false);
        addAndMakeVisible (viewport);

        horizontalScrollBar.setAutoHide (false);
        horizontalScrollBar.addListener (this);
        addAndMakeVisible (horizontalScrollBar);

        addAndMakeVisible (playheadOverlay);   // on top of everything

        updateScrollBar();
    }

    TimelineView::~TimelineView()
    {
        horizontalScrollBar.removeListener (this);
    }

    //==========================================================================
    void TimelineView::addTrack (std::shared_ptr<const Track> track)
    {
        sessionLengthSeconds = juce::jmax (sessionLengthSeconds, track->getLengthInSeconds());

        auto* lane = lanes.add (new TrackLane (std::move (track), lanes.size() + 1,
                                               *this, formatManager, thumbnailCache));
        laneContainer.addAndMakeVisible (lane);
        layoutLanes();
        updateScrollBar();
        repaint();
    }

    void TimelineView::removeTrack (const Track& track)
    {
        auto* lane = findLane (track);

        if (lane == nullptr)
            return;

        lanes.removeObject (lane);   // also deletes the lane and its waveform

        // Renumber what's left and recompute the session length.
        sessionLengthSeconds = 0.0;

        for (int i = 0; i < lanes.size(); ++i)
        {
            lanes[i]->setTrackNumber (i + 1);
            sessionLengthSeconds = juce::jmax (sessionLengthSeconds, lanes[i]->getTrack()->getLengthInSeconds());
        }

        if (lanes.isEmpty())
            axis.startSeconds = 0.0;

        layoutLanes();
        timeAxisChanged();
        repaint();   // shows the "drop files here" hint again when empty
    }

    std::shared_ptr<const Track> TimelineView::getSelectedTrack() const
    {
        for (auto* lane : lanes)
            if (lane->isSelected())
                return lane->getTrack();

        return nullptr;
    }

    void TimelineView::selectTrack (const Track& track)
    {
        for (auto* lane : lanes)
            lane->setSelected (lane->getTrack().get() == &track);
    }

    void TimelineView::showTrackMenu (const Track& track)
    {
        auto* lane = findLane (track);

        if (lane == nullptr)
            return;

        enum MenuItem { deleteTrack = 1 };

        juce::PopupMenu menu;
        menu.addSectionHeader (track.getName());
        menu.addItem (deleteTrack, "Delete Track");

        // The lane may be gone by the time the menu closes, so hold the track
        // itself and check that this view still exists.
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (lane).withMousePosition(),
                            [safeThis = juce::Component::SafePointer<TimelineView> (this),
                             trackPtr = lane->getTrack()] (int result)
                            {
                                if (result == deleteTrack && safeThis != nullptr && safeThis->onDeleteTrackRequested)
                                    safeThis->onDeleteTrackRequested (trackPtr);
                            });
    }

    TrackLane* TimelineView::findLane (const Track& track) const
    {
        for (auto* lane : lanes)
            if (lane->getTrack().get() == &track)
                return lane;

        return nullptr;
    }

    void TimelineView::setSampleRate (double newSampleRate)
    {
        if (newSampleRate > 0.0)
            sampleRate = newSampleRate;
    }

    void TimelineView::setPlayheadPosition (double seconds, bool isPlaying)
    {
        playheadSeconds = seconds;

        if (isPlaying)
        {
            // Page along when the playhead runs off either edge.
            const auto x = axis.xAtTime (seconds);
            const auto width = getTimeAreaWidth();

            if (width > 0 && (x > width * 0.98 || x < 0.0))
                setViewStart (seconds - axis.secondsForWidth (width) * 0.02);
        }

        playheadOverlay.moveTo (seconds);
    }

    void TimelineView::zoomToFit()
    {
        const auto width = getTimeAreaWidth();

        if (sessionLengthSeconds <= 0.0 || width <= 0)
            return;

        axis.pixelsPerSecond = juce::jlimit (minPixelsPerSecond, getMaxPixelsPerSecond(),
                                             width * 0.95 / sessionLengthSeconds);
        axis.startSeconds = 0.0;
        timeAxisChanged();
    }

    //==========================================================================
    void TimelineView::paint (juce::Graphics& g)
    {
        g.fillAll (theme::background);

        // Corner above the track headers.
        auto corner = juce::Rectangle<int> (0, 0, TrackLane::headerWidth, rulerHeight);
        g.setColour (theme::panel);
        g.fillRect (corner);
        g.setColour (theme::panelBorder);
        g.drawHorizontalLine (rulerHeight - 1, 0.0f, (float) TrackLane::headerWidth);
        g.drawVerticalLine (TrackLane::headerWidth - 1, 0.0f, (float) rulerHeight);
        g.setColour (theme::dimText);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("TRACKS", corner.reduced (10, 0), juce::Justification::centredLeft);

        if (lanes.isEmpty())
        {
            g.setColour (theme::dimText);
            g.setFont (juce::FontOptions (16.0f));
            g.drawText ("Drop audio files here, or click Load Audio",
                        viewport.getBounds(), juce::Justification::centred);
        }
    }

    void TimelineView::paintOverChildren (juce::Graphics& g)
    {
        if (fileDragActive)
        {
            g.setColour (theme::playhead.withAlpha (0.12f));
            g.fillRect (viewport.getBounds());
            g.setColour (theme::playhead);
            g.drawRect (viewport.getBounds(), 2);
        }
    }

    void TimelineView::resized()
    {
        const auto timeAreaWidth = getTimeAreaWidth();
        auto bounds = getLocalBounds();

        ruler.setBounds (bounds.removeFromTop (rulerHeight)
                               .withX (TrackLane::headerWidth).withWidth (timeAreaWidth));

        horizontalScrollBar.setBounds (bounds.removeFromBottom (scrollBarHeight)
                                             .withX (TrackLane::headerWidth).withWidth (timeAreaWidth));

        viewport.setBounds (bounds);
        layoutLanes();

        playheadOverlay.setBounds (TrackLane::headerWidth, 0, timeAreaWidth, rulerHeight + bounds.getHeight());

        timeAxisChanged();
    }

    //==========================================================================
    bool TimelineView::isInterestedInFileDrag (const juce::StringArray& files)
    {
        for (const auto& path : files)
            if (canLoadFile == nullptr || canLoadFile (juce::File (path)))
                return true;

        return false;
    }

    void TimelineView::fileDragEnter (const juce::StringArray&, int, int)
    {
        fileDragActive = true;
        repaint();
    }

    void TimelineView::fileDragExit (const juce::StringArray&)
    {
        fileDragActive = false;
        repaint();
    }

    void TimelineView::filesDropped (const juce::StringArray& files, int, int)
    {
        fileDragActive = false;
        repaint();

        if (onFilesDropped != nullptr)
            onFilesDropped (files);
    }

    //==========================================================================
    void TimelineView::seekToTime (double seconds)
    {
        if (onSeek != nullptr)
            onSeek (juce::jlimit (0.0, sessionLengthSeconds, seconds));
    }

    bool TimelineView::handleTimelineWheel (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
    {
        const auto mouseX = (double) e.getEventRelativeTo (&playheadOverlay).position.x;

        if (e.mods.isCtrlDown() || e.mods.isCommandDown())
        {
            const auto delta = wheel.deltaY != 0.0f ? wheel.deltaY : wheel.deltaX;

            if (delta != 0.0f)
            {
                // Keep the time under the mouse fixed while zooming.
                const auto timeUnderMouse = axis.timeAtX (mouseX);
                axis.pixelsPerSecond = juce::jlimit (minPixelsPerSecond, getMaxPixelsPerSecond(),
                                                     axis.pixelsPerSecond * std::pow (2.0, (double) delta));
                axis.startSeconds = juce::jmax (0.0, timeUnderMouse - mouseX / axis.pixelsPerSecond);
                timeAxisChanged();
            }

            return true;
        }

        if (e.mods.isShiftDown() || wheel.deltaX != 0.0f)
        {
            const auto delta = wheel.deltaX != 0.0f ? wheel.deltaX : wheel.deltaY;
            const auto visibleSeconds = axis.secondsForWidth (getTimeAreaWidth());
            setViewStart (axis.startSeconds - (double) delta * visibleSeconds * 0.25);
            return true;
        }

        return false;
    }

    //==========================================================================
    void TimelineView::scrollBarMoved (juce::ScrollBar*, double newRangeStart)
    {
        axis.startSeconds = newRangeStart;
        timeAxisChanged();
    }

    void TimelineView::timeAxisChanged()
    {
        updateScrollBar();
        ruler.repaint();

        for (auto* lane : lanes)
            lane->getWaveformView().repaint();

        playheadOverlay.repaint();
    }

    void TimelineView::updateScrollBar()
    {
        const auto visibleSeconds = juce::jmax (0.001, axis.secondsForWidth (getTimeAreaWidth()));
        const auto limit = juce::jmax (sessionLengthSeconds * 1.05, axis.startSeconds + visibleSeconds);

        horizontalScrollBar.setRangeLimits (0.0, limit, juce::dontSendNotification);
        horizontalScrollBar.setCurrentRange (axis.startSeconds, visibleSeconds, juce::dontSendNotification);
    }

    void TimelineView::layoutLanes()
    {
        const auto width = juce::jmax (0, getWidth() - viewport.getScrollBarThickness());
        int y = 0;

        for (auto* lane : lanes)
        {
            const auto height = lane->getPreferredHeight();
            lane->setBounds (0, y, width, height);
            y += height;
        }

        laneContainer.setSize (width, y);
    }

    void TimelineView::setViewStart (double seconds)
    {
        const auto visibleSeconds = axis.secondsForWidth (getTimeAreaWidth());
        const auto maxStart = juce::jmax (0.0, sessionLengthSeconds - visibleSeconds * 0.1);

        axis.startSeconds = juce::jlimit (0.0, maxStart, seconds);
        timeAxisChanged();
    }

    int TimelineView::getTimeAreaWidth() const
    {
        // Always leave room for the vertical scrollbar so the time area doesn't
        // change width when tracks are added.
        return juce::jmax (0, getWidth() - TrackLane::headerWidth - viewport.getScrollBarThickness());
    }

    double TimelineView::getSessionLengthSeconds() const
    {
        return sessionLengthSeconds;
    }

    double TimelineView::getMaxPixelsPerSecond() const
    {
        return sampleRate * 16.0;   // up to 16 pixels per sample
    }
}
