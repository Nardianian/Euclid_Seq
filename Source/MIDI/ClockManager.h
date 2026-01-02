#pragma once
#include <JuceHeader.h>
#include <atomic>

enum class ClockSource
{
    DAW,
    Internal,
    External
};

enum class ClockType
{
    MidiBeatClock,   // 24 PPQN
    MidiTimeCode,
    PPQN
};

enum class ClockRole
{
    Master,
    Slave
};

class ClockManager
{
public:
    void prepare(double sampleRate);

    void setClockSource(ClockSource src);
    void setClockType(ClockType type);
    void setClockRole(ClockRole role);

    void processBlock(int numSamples);
    void processMidi(const juce::MidiBuffer& midi);

    double getSamplesPerStep() const;

private:
    double sampleRate = 44100.0;
    double bpm = 120.0;

    std::atomic<ClockSource> source{ ClockSource::Internal };
    std::atomic<ClockType> type{ ClockType::MidiBeatClock };
    std::atomic<ClockRole> role{ ClockRole::Master };

    // ===== MIDI CLOCK =====
    int midiClockCounter = 0;
    double samplesPerMidiClock = 0.0;
    double sampleCounter = 0.0;

    // ===== INTERNAL =====
    double internalSamplesPerStep = 0.0;
};
