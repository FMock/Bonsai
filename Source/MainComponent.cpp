#include "MainComponent.h"
#include "UI/Theme.h"

namespace bonsai
{
    MainComponent::MainComponent()
    {
        const auto deviceError = engine.initialise();
        const auto sampleRate = engine.getSampleRate();

        timeline.setSampleRate (sampleRate);

        if (auto* device = engine.getDeviceManager().getCurrentAudioDevice())
            transportBar.setInfoText (device->getName() + "  |  " + juce::String (sampleRate / 1000.0, 1) + " kHz");
        else
            transportBar.setInfoText ("No audio device - playback is silent");

        transportBar.onLoadAudio = [this] { showLoadDialog(); };
        transportBar.onDeleteTrack = [this] { confirmDeleteTrack (timeline.getSelectedTrack()); };
        transportBar.onPlay      = [this] { play(); };
        transportBar.onPause     = [this] { pause(); };
        transportBar.onStop      = [this] { stop(); };

        timeline.onSeek = [this] (double seconds) { seekToSeconds (seconds); };
        timeline.canLoadFile = [this] (const juce::File& file) { return loader.canLoad (file); };
        timeline.onDeleteTrackRequested = [this] (std::shared_ptr<const Track> track) { confirmDeleteTrack (std::move (track)); };
        timeline.onFilesDropped = [this] (const juce::StringArray& paths)
        {
            juce::Array<juce::File> files;
            for (const auto& path : paths)
                files.add (juce::File (path));
            loadFiles (files);
        };

        addAndMakeVisible (transportBar);
        addAndMakeVisible (timeline);

        lastBrowseFolder = juce::File::getSpecialLocation (juce::File::userMusicDirectory);

        setWantsKeyboardFocus (true);
        setSize (1280, 760);

        startTimerHz (30);

        if (deviceError.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    "Audio device",
                                                    "Couldn't open an audio output device:\n" + deviceError
                                                        + "\n\nYou can still load and view audio.");
    }

    MainComponent::~MainComponent()
    {
        stopTimer();
    }

    void MainComponent::paint (juce::Graphics& g)
    {
        g.fillAll (theme::background);
    }

    void MainComponent::resized()
    {
        auto bounds = getLocalBounds();
        transportBar.setBounds (bounds.removeFromTop (56));
        timeline.setBounds (bounds);
    }

    bool MainComponent::keyPressed (const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::spaceKey)
        {
            togglePlayPause();
            return true;
        }

        if (key == juce::KeyPress::returnKey || key == juce::KeyPress::homeKey)
        {
            returnToStart();
            return true;
        }

        if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        {
            if (auto track = timeline.getSelectedTrack())
                confirmDeleteTrack (std::move (track));

            return true;
        }

        if (key == juce::KeyPress ('o', juce::ModifierKeys::commandModifier, 0))
        {
            showLoadDialog();
            return true;
        }

        return false;
    }

    void MainComponent::timerCallback()
    {
        // Also picks up the transport stopping by itself at the end of the session.
        refreshTransportDisplay();
    }

    void MainComponent::refreshTransportDisplay()
    {
        auto& player = engine.getPlayer();
        const auto seconds = samplesToSeconds (player.getPosition());

        transportBar.update (player.getState(), seconds, player.getNumTracks() > 0,
                            timeline.getSelectedTrack() != nullptr);
        timeline.setPlayheadPosition (seconds, player.isPlaying());
    }

    //==========================================================================
    void MainComponent::showLoadDialog()
    {
        fileChooser = std::make_unique<juce::FileChooser> ("Load audio files",
                                                           lastBrowseFolder,
                                                           loader.getFileWildcard());

        const auto chooserFlags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::canSelectMultipleItems;

        fileChooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& chooser)
        {
            const auto results = chooser.getResults();

            if (! results.isEmpty())
            {
                lastBrowseFolder = results.getFirst().getParentDirectory();
                loadFiles (results);
            }
        });
    }

    void MainComponent::loadFiles (const juce::Array<juce::File>& files)
    {
        juce::StringArray errors;
        const auto sessionWasEmpty = timeline.getNumTracks() == 0;

        // Loading is synchronous for now; big files take a moment.
        juce::MouseCursor::showWaitCursor();

        for (const auto& file : files)
        {
            auto result = loader.load (file, engine.getSampleRate());

            if (result.succeeded())
            {
                engine.getPlayer().addTrack (result.track);
                timeline.addTrack (result.track);
            }
            else
            {
                errors.add (result.error);
            }
        }

        juce::MouseCursor::hideWaitCursor();

        // First load: show everything that was just loaded. After that, leave
        // the user's zoom alone.
        if (sessionWasEmpty)
            timeline.zoomToFit();

        refreshTransportDisplay();
        grabKeyboardFocus();

        if (! errors.isEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    errors.size() == 1 ? "Couldn't load file" : "Couldn't load some files",
                                                    errors.joinIntoString ("\n"));
    }

    //==========================================================================
    void MainComponent::confirmDeleteTrack (std::shared_ptr<const Track> track)
    {
        if (track == nullptr)
            return;

        const auto options = juce::MessageBoxOptions()
                                 .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                 .withTitle ("Delete Track")
                                 .withMessage ("Are you sure you want to delete \"" + track->getName() + "\"?\n\n"
                                               "The track is removed from Bonsai. The audio file on disk is not deleted.")
                                 .withButton ("Delete")
                                 .withButton ("Cancel")
                                 .withAssociatedComponent (this);

        // With two buttons, the first ("Delete") returns 1 and "Cancel" returns 0.
        deleteConfirmation = juce::AlertWindow::showScopedAsync (options, [this, track] (int result)
        {
            if (result == 1)
                deleteTrack (*track);

            grabKeyboardFocus();
        });
    }

    void MainComponent::deleteTrack (const Track& track)
    {
        timeline.removeTrack (track);
        engine.getPlayer().removeTrack (track);
        refreshTransportDisplay();
    }

    //==========================================================================
    void MainComponent::play()
    {
        engine.getPlayer().play();
        refreshTransportDisplay();
    }

    void MainComponent::pause()
    {
        engine.getPlayer().pause();
        refreshTransportDisplay();
    }

    void MainComponent::stop()
    {
        engine.getPlayer().stop();
        refreshTransportDisplay();
    }

    void MainComponent::togglePlayPause()
    {
        if (engine.getPlayer().isPlaying())
            pause();
        else
            play();
    }

    void MainComponent::returnToStart()
    {
        // Keeps playing if it was playing, like most DAWs.
        engine.getPlayer().setPosition (0);
        refreshTransportDisplay();
    }

    void MainComponent::seekToSeconds (double seconds)
    {
        engine.getPlayer().setPosition ((juce::int64) std::llround (seconds * engine.getSampleRate()));
        refreshTransportDisplay();
    }

    double MainComponent::samplesToSeconds (juce::int64 samples) const
    {
        return (double) samples / engine.getSampleRate();
    }
}
