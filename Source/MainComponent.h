#pragma once

#include "Engine/AudioEngine.h"
#include "Engine/AudioFileLoader.h"
#include "UI/TimelineView.h"
#include "UI/TransportBar.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace bonsai
{
    // The whole window: owns the audio engine and file loader, and connects
    // them to the transport bar and timeline.
    class MainComponent final : public juce::Component,
                                private juce::Timer
    {
    public:
        MainComponent();
        ~MainComponent() override;

        void paint (juce::Graphics& g) override;
        void resized() override;
        bool keyPressed (const juce::KeyPress& key) override;

        // Loads each file as a new track; problems are reported in one message.
        void loadFiles (const juce::Array<juce::File>& files);

    private:
        void timerCallback() override;

        void showLoadDialog();

        void confirmDeleteTrack (std::shared_ptr<const Track> track);
        void deleteTrack (const Track& track);

        void play();
        void pause();
        void stop();
        void togglePlayPause();
        void returnToStart();
        void seekToSeconds (double seconds);

        void refreshTransportDisplay();

        double samplesToSeconds (juce::int64 samples) const;

        AudioEngine engine;
        AudioFileLoader loader;
        juce::AudioThumbnailCache thumbnailCache { 32 };

        TransportBar transportBar;
        TimelineView timeline { loader.getFormatManager(), thumbnailCache };
        juce::TooltipWindow tooltipWindow { this };

        std::unique_ptr<juce::FileChooser> fileChooser;
        juce::ScopedMessageBox deleteConfirmation;   // closes the prompt if we're destroyed first
        juce::File lastBrowseFolder;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
    };
}
