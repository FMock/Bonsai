#pragma once

#include "Engine/Track.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <memory>
#include <vector>

namespace bonsai
{
    enum class TransportState
    {
        Stopped,
        Playing,
        Paused
    };

    // The session's tracks plus the transport (play/pause/stop/playhead), and
    // the mixer that renders them. It is a plain juce::AudioSource with no
    // device code, so it can be driven directly in tests.
    //
    // Threading: the control methods are called on the message thread and
    // getNextAudioBlock() on the audio thread. The audio thread never blocks:
    // it uses a try-lock on the track list and outputs silence for the rare
    // block where the list is being changed.
    class SessionPlayer final : public juce::AudioSource
    {
    public:
        SessionPlayer() = default;

        // AudioSource
        void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock (const juce::AudioSourceChannelInfo& info) override;

        // Tracks
        void addTrack (std::shared_ptr<Track> track);

        // Removes a track from the session (the file on disk is untouched).
        // Safe during playback. Removing the last track stops the transport.
        // Returns false if the track isn't in the session.
        bool removeTrack (const Track& track);
        std::vector<std::shared_ptr<Track>> getTracks() const;
        int getNumTracks() const;

        // Transport
        void play();
        void pause();
        void stop();                                // halts and returns to the start
        void setPosition (juce::int64 samplePosition);

        TransportState getState() const noexcept     { return state.load(); }
        bool isPlaying() const noexcept              { return getState() == TransportState::Playing; }
        juce::int64 getPosition() const noexcept     { return position.load(); }

        // Length of the longest track, in samples.
        juce::int64 getLengthInSamples() const noexcept { return lengthInSamples.load(); }

        // True once after playback ran off the end of the session and stopped
        // by itself. Reading it clears it.
        bool consumeReachedEnd() noexcept            { return reachedEnd.exchange (false); }

    private:
        static void mixTrack (const Track& track,
                              juce::int64 startSample,
                              const juce::AudioSourceChannelInfo& info);

        juce::CriticalSection trackLock;
        std::vector<std::shared_ptr<Track>> tracks;

        std::atomic<TransportState> state { TransportState::Stopped };
        std::atomic<juce::int64> position { 0 };
        std::atomic<juce::int64> lengthInSamples { 0 };
        std::atomic<bool> reachedEnd { false };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SessionPlayer)
    };
}
