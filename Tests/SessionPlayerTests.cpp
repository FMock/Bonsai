#include "Engine/SessionPlayer.h"

namespace bonsai
{
    namespace
    {
        constexpr double testRate = 48000.0;

        // A track whose every sample on channel c equals values[c].
        std::shared_ptr<Track> makeConstantTrack (std::initializer_list<float> values, int numSamples)
        {
            juce::AudioBuffer<float> audio ((int) values.size(), numSamples);

            int ch = 0;
            for (auto v : values)
                juce::FloatVectorOperations::fill (audio.getWritePointer (ch++), v, numSamples);

            return std::make_shared<Track> ("test", std::move (audio), testRate, testRate);
        }

        // A mono track where sample n has the value n, so a block's first sample
        // reveals exactly where in the track playback was.
        std::shared_ptr<Track> makeRampTrack (int numSamples)
        {
            juce::AudioBuffer<float> audio (1, numSamples);

            for (int n = 0; n < numSamples; ++n)
                audio.setSample (0, n, (float) n);

            return std::make_shared<Track> ("ramp", std::move (audio), testRate, testRate);
        }

        void render (SessionPlayer& player, juce::AudioBuffer<float>& block)
        {
            juce::AudioSourceChannelInfo info (block);
            player.getNextAudioBlock (info);
        }
    }

    class SessionPlayerTests final : public juce::UnitTest
    {
    public:
        SessionPlayerTests() : juce::UnitTest ("SessionPlayer", "Bonsai") {}

        void runTest() override
        {
            constexpr int blockSize = 64;
            juce::AudioBuffer<float> block (2, blockSize);

            beginTest ("Silent while stopped");
            {
                SessionPlayer player;
                player.addTrack (makeConstantTrack ({ 0.5f }, 1000));
                render (player, block);
                expectEquals (block.getMagnitude (0, blockSize), 0.0f);
                expectEquals (player.getPosition(), (juce::int64) 0);
            }

            beginTest ("Mono track plays equally in left and right");
            {
                SessionPlayer player;
                player.addTrack (makeConstantTrack ({ 0.25f }, 1000));
                player.play();
                render (player, block);
                expectEquals (block.getSample (0, 10), 0.25f);
                expectEquals (block.getSample (1, 10), 0.25f);
            }

            beginTest ("Stereo track keeps its channels separate");
            {
                SessionPlayer player;
                player.addTrack (makeConstantTrack ({ 0.1f, 0.7f }, 1000));
                player.play();
                render (player, block);
                expectWithinAbsoluteError (block.getSample (0, 10), 0.1f, 1.0e-6f);
                expectWithinAbsoluteError (block.getSample (1, 10), 0.7f, 1.0e-6f);
            }

            beginTest ("Tracks are summed");
            {
                SessionPlayer player;
                player.addTrack (makeConstantTrack ({ 0.25f }, 1000));
                player.addTrack (makeConstantTrack ({ 0.1f, 0.2f }, 1000));
                player.play();
                render (player, block);
                expectWithinAbsoluteError (block.getSample (0, 0), 0.35f, 1.0e-6f);
                expectWithinAbsoluteError (block.getSample (1, 0), 0.45f, 1.0e-6f);
            }

            beginTest ("Pause keeps the position; play resumes from it");
            {
                SessionPlayer player;
                player.addTrack (makeRampTrack (1000));
                player.play();
                render (player, block);
                render (player, block);
                expectEquals (player.getPosition(), (juce::int64) (2 * blockSize));

                player.pause();
                expect (player.getState() == TransportState::Paused);
                render (player, block);
                expectEquals (player.getPosition(), (juce::int64) (2 * blockSize));
                expectEquals (block.getMagnitude (0, blockSize), 0.0f);

                player.play();
                render (player, block);
                expectEquals (block.getSample (0, 0), (float) (2 * blockSize));
            }

            beginTest ("Stop returns to the start");
            {
                SessionPlayer player;
                player.addTrack (makeRampTrack (1000));
                player.play();
                render (player, block);
                player.stop();
                expect (player.getState() == TransportState::Stopped);
                expectEquals (player.getPosition(), (juce::int64) 0);

                player.play();
                render (player, block);
                expectEquals (block.getSample (0, 1), 1.0f);
            }

            beginTest ("Seeking moves the playhead");
            {
                SessionPlayer player;
                player.addTrack (makeRampTrack (1000));
                player.setPosition (500);
                player.play();
                render (player, block);
                expectEquals (block.getSample (0, 0), 500.0f);

                player.setPosition (5000);  // past the end is clamped
                expectEquals (player.getPosition(), (juce::int64) 1000);
            }

            beginTest ("Shorter track goes silent after its end");
            {
                SessionPlayer player;
                player.addTrack (makeConstantTrack ({ 0.5f }, 100));
                player.addTrack (makeConstantTrack ({ 0.25f }, 1000));
                player.play();
                render (player, block);                         // samples 0..63
                expectEquals (block.getSample (0, 0), 0.75f);
                render (player, block);                         // samples 64..127
                expectEquals (block.getSample (0, 100 - blockSize - 1), 0.75f);
                expectEquals (block.getSample (0, 100 - blockSize), 0.25f);
            }

            beginTest ("Playback stops by itself at the end of the longest track");
            {
                SessionPlayer player;
                player.addTrack (makeConstantTrack ({ 0.5f }, 100));
                player.play();
                render (player, block);
                expect (player.isPlaying());
                expect (! player.consumeReachedEnd());

                render (player, block);                         // passes sample 100
                expect (player.getState() == TransportState::Stopped);
                expectEquals (player.getPosition(), (juce::int64) 0);
                expect (player.consumeReachedEnd());
                expect (! player.consumeReachedEnd());          // cleared once read

                // Partial last block: real audio up to the end, then silence.
                expectEquals (block.getSample (0, 100 - blockSize - 1), 0.5f);
                expectEquals (block.getSample (0, 100 - blockSize), 0.0f);
            }

            beginTest ("Removed track is no longer heard");
            {
                SessionPlayer player;
                auto keep = makeConstantTrack ({ 0.25f }, 1000);
                auto drop = makeConstantTrack ({ 0.5f }, 1000);
                player.addTrack (keep);
                player.addTrack (drop);

                expect (player.removeTrack (*drop));
                expect (! player.removeTrack (*drop));      // already gone
                expectEquals (player.getNumTracks(), 1);

                player.play();
                render (player, block);
                expectEquals (block.getSample (0, 0), 0.25f);
            }

            beginTest ("Removing the longest track shortens the session");
            {
                SessionPlayer player;
                auto shortTrack = makeConstantTrack ({ 0.1f }, 100);
                auto longTrack = makeConstantTrack ({ 0.1f }, 1000);
                player.addTrack (shortTrack);
                player.addTrack (longTrack);
                player.setPosition (500);

                player.removeTrack (*longTrack);
                expectEquals (player.getLengthInSamples(), (juce::int64) 100);
                expectEquals (player.getPosition(), (juce::int64) 100);     // pulled back to the end
            }

            beginTest ("Removing the last track stops playback");
            {
                SessionPlayer player;
                auto only = makeConstantTrack ({ 0.1f }, 1000);
                player.addTrack (only);
                player.play();
                render (player, block);

                player.removeTrack (*only);
                expect (player.getState() == TransportState::Stopped);
                expectEquals (player.getPosition(), (juce::int64) 0);
                expectEquals (player.getLengthInSamples(), (juce::int64) 0);

                render (player, block);
                expectEquals (block.getMagnitude (0, blockSize), 0.0f);
            }

            beginTest ("Play does nothing with no tracks");
            {
                SessionPlayer player;
                player.play();
                expect (player.getState() == TransportState::Stopped);
            }
        }
    };

    static SessionPlayerTests sessionPlayerTests;
}
