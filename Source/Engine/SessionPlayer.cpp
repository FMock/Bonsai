#include "Engine/SessionPlayer.h"

#include <algorithm>

namespace bonsai
{
    void SessionPlayer::prepareToPlay (int, double)
    {
        // Tracks are already stored at the session rate and the mixer keeps no
        // per-block state, so there's nothing to prepare yet.
    }

    void SessionPlayer::releaseResources()
    {
    }

    void SessionPlayer::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
    {
        info.clearActiveBufferRegion();

        if (state.load() != TransportState::Playing)
            return;

        const juce::ScopedTryLock lock (trackLock);

        if (! lock.isLocked())
            return;     // a track is being added right now; skip one block

        auto start = position.load();

        for (const auto& track : tracks)
            mixTrack (*track, start, info);

        const auto end = start + info.numSamples;

        if (end >= lengthInSamples.load())
        {
            // Ran off the end: stop and rewind, unless the user changed the
            // transport while this block was rendering.
            auto expected = TransportState::Playing;

            if (state.compare_exchange_strong (expected, TransportState::Stopped))
            {
                position.store (0);
                reachedEnd.store (true);
            }
        }
        else
        {
            // If the playhead was moved (seek) during this block, keep the new
            // position rather than overwriting it.
            position.compare_exchange_strong (start, end);
        }
    }

    void SessionPlayer::mixTrack (const Track& track,
                                  juce::int64 startSample,
                                  const juce::AudioSourceChannelInfo& info)
    {
        const auto trackLength = track.getLengthInSamples();

        if (startSample >= trackLength)
            return;     // this track has already finished; it contributes silence

        const auto numSamples = (int) std::min<juce::int64> (info.numSamples, trackLength - startSample);
        const auto sourceStart = (int) startSample;
        const auto gain = track.getGain();
        const auto& audio = track.getAudio();
        auto& out = *info.buffer;
        const auto numOutChannels = out.getNumChannels();

        if (numOutChannels == 0)
            return;

        if (numOutChannels == 1)
        {
            // Mono output device: fold stereo tracks down.
            const auto channelGain = track.isStereo() ? gain * 0.5f : gain;

            for (int ch = 0; ch < track.getNumChannels(); ++ch)
                out.addFrom (0, info.startSample, audio, ch, sourceStart, numSamples, channelGain);

            return;
        }

        if (track.isStereo())
        {
            out.addFrom (0, info.startSample, audio, 0, sourceStart, numSamples, gain);
            out.addFrom (1, info.startSample, audio, 1, sourceStart, numSamples, gain);
        }
        else
        {
            // Mono track: same signal in left and right (centered).
            out.addFrom (0, info.startSample, audio, 0, sourceStart, numSamples, gain);
            out.addFrom (1, info.startSample, audio, 0, sourceStart, numSamples, gain);
        }
    }

    void SessionPlayer::addTrack (std::shared_ptr<Track> track)
    {
        jassert (track != nullptr);

        const juce::ScopedLock lock (trackLock);

        const auto newLength = std::max (lengthInSamples.load(), track->getLengthInSamples());
        tracks.push_back (std::move (track));
        lengthInSamples.store (newLength);
    }

    bool SessionPlayer::removeTrack (const Track& track)
    {
        // Taken out of the list under the lock, but released after it, so the
        // audio (if this was the last reference) is freed here on the calling
        // thread and never on the audio thread.
        std::shared_ptr<Track> removed;

        {
            const juce::ScopedLock lock (trackLock);

            const auto it = std::find_if (tracks.begin(), tracks.end(),
                                          [&] (const auto& t) { return t.get() == &track; });

            if (it == tracks.end())
                return false;

            removed = std::move (*it);
            tracks.erase (it);

            juce::int64 newLength = 0;
            for (const auto& t : tracks)
                newLength = std::max (newLength, t->getLengthInSamples());

            lengthInSamples.store (newLength);
        }

        if (lengthInSamples.load() == 0)
        {
            stop();
        }
        else if (position.load() > lengthInSamples.load())
        {
            // The playhead is now past the end of the session. If playing, the
            // next audio block notices and stops; otherwise pull it back in.
            if (! isPlaying())
                position.store (lengthInSamples.load());
        }

        return true;
    }

    std::vector<std::shared_ptr<Track>> SessionPlayer::getTracks() const
    {
        const juce::ScopedLock lock (trackLock);
        return tracks;
    }

    int SessionPlayer::getNumTracks() const
    {
        const juce::ScopedLock lock (trackLock);
        return (int) tracks.size();
    }

    void SessionPlayer::play()
    {
        if (lengthInSamples.load() == 0)
            return;     // nothing to play

        // Pressing Play with the playhead at (or past) the end starts over.
        if (position.load() >= lengthInSamples.load())
            position.store (0);

        state.store (TransportState::Playing);
    }

    void SessionPlayer::pause()
    {
        auto expected = TransportState::Playing;
        state.compare_exchange_strong (expected, TransportState::Paused);
    }

    void SessionPlayer::stop()
    {
        state.store (TransportState::Stopped);
        position.store (0);
    }

    void SessionPlayer::setPosition (juce::int64 samplePosition)
    {
        position.store (std::clamp<juce::int64> (samplePosition, 0, lengthInSamples.load()));
    }
}
