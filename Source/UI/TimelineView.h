#pragma once

#include "UI/TimeRuler.h"
#include "UI/TrackLane.h"

#include <functional>
#include <memory>

namespace bonsai
{
    // The main editing area: time ruler on top, track lanes stacked below
    // (scrolling vertically), a horizontal time scrollbar, and the playhead.
    //
    //   Ctrl + wheel          zoom in/out around the mouse
    //   Shift + wheel         scroll in time
    //   wheel                 scroll the track list
    //   click / drag          move the playhead
    //   drop audio files      load them as new tracks
    class TimelineView final : public juce::Component,
                               public juce::FileDragAndDropTarget,
                               public TimelineHost,
                               private juce::ScrollBar::Listener
    {
    public:
        TimelineView (juce::AudioFormatManager& formatManager,
                      juce::AudioThumbnailCache& thumbnailCache);
        ~TimelineView() override;

        void addTrack (std::shared_ptr<const Track> track);
        void removeTrack (const Track& track);
        int getNumTracks() const noexcept { return lanes.size(); }

        // The highlighted track, or null if none.
        std::shared_ptr<const Track> getSelectedTrack() const;

        void setSampleRate (double newSampleRate);

        // Called regularly by the owner. While playing, the view pages along
        // to keep the playhead visible.
        void setPlayheadPosition (double seconds, bool isPlaying);

        void zoomToFit();

        std::function<void (double seconds)> onSeek;
        std::function<void (const juce::StringArray& files)> onFilesDropped;
        std::function<bool (const juce::File& file)> canLoadFile;

        // "Delete Track" was chosen from a track's menu. The owner confirms
        // with the user and then calls removeTrack().
        std::function<void (std::shared_ptr<const Track> track)> onDeleteTrackRequested;

        // Component
        void paint (juce::Graphics& g) override;
        void paintOverChildren (juce::Graphics& g) override;
        void resized() override;

        // FileDragAndDropTarget
        bool isInterestedInFileDrag (const juce::StringArray& files) override;
        void fileDragEnter (const juce::StringArray& files, int x, int y) override;
        void fileDragExit (const juce::StringArray& files) override;
        void filesDropped (const juce::StringArray& files, int x, int y) override;

        // TimelineHost
        const TimeAxis& getTimeAxis() const override { return axis; }
        void seekToTime (double seconds) override;
        bool handleTimelineWheel (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
        void selectTrack (const Track& track) override;
        void showTrackMenu (const Track& track) override;

    private:
        // Draws the playhead line over the ruler and lanes; ignores the mouse.
        class PlayheadOverlay final : public juce::Component
        {
        public:
            explicit PlayheadOverlay (const TimelineView& owner);
            void paint (juce::Graphics& g) override;
            void moveTo (double seconds);

        private:
            const TimelineView& owner;
            double positionSeconds = 0.0;
            int lastX = -1;
        };

        static constexpr int rulerHeight = 28;
        static constexpr int scrollBarHeight = 14;
        static constexpr double minPixelsPerSecond = 0.5;

        void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;

        TrackLane* findLane (const Track& track) const;
        void timeAxisChanged();           // repaint everything that depends on the axis
        void updateScrollBar();
        void layoutLanes();
        void setViewStart (double seconds);
        int getTimeAreaWidth() const;
        double getSessionLengthSeconds() const;
        double getMaxPixelsPerSecond() const;

        juce::AudioFormatManager& formatManager;
        juce::AudioThumbnailCache& thumbnailCache;

        TimeAxis axis;
        double sampleRate = 48000.0;
        double sessionLengthSeconds = 0.0;
        double playheadSeconds = 0.0;
        bool fileDragActive = false;

        TimeRuler ruler { *this };
        juce::Viewport viewport;
        juce::Component laneContainer;
        juce::OwnedArray<TrackLane> lanes;
        juce::ScrollBar horizontalScrollBar { false };
        PlayheadOverlay playheadOverlay { *this };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimelineView)
    };
}
