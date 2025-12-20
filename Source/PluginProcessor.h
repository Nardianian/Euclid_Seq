#pragma once
#include <JuceHeader.h>
#include "MidiGenerator.h"

struct MidiPortEvent
{
    int portIndex;
    juce::MidiMessage message;
    int sampleOffset;
};

class Euclidean_seqAudioProcessor : public juce::AudioProcessor
{
public:
    Euclidean_seqAudioProcessor();
    ~Euclidean_seqAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "Euclidean_seq"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;

    // Midi Generator
    MidiGenerator midiGen;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    std::vector<MidiPortEvent> stagedMidiEvents;

    // ================= CLOCK STATE =================
    enum class ClockSource
    {
        DAW = 0,
        Internal,
        External
    };


    double currentSampleRate = 44100.0;
    double samplesPerBeat = 0.0;
    double samplesPerStep = 0.0;

    double internalBpm = 120.0;

    int64_t globalSampleCounter = 0;
    // ===== PER-RHYTHM TIMING =====
    std::array<int64_t, 6> nextStepSamplePerRhythm{};
    std::array<int64_t, 6> stepCounterPerRhythm{};
    // ===== MICROTIMING PER STEP INDEX =====
    std::array<std::vector<double>, 6> stepMicrotiming;
    // ===== NOTE-OFF BUFFER CROSSING (BASSO) =====
    struct PendingNoteOff
    {
        int channel = 1;
        int note = 0;
        int remainingSamples = 0;
    };

    std::vector<PendingNoteOff> pendingNoteOffs;

    // ================= EXTERNAL MIDI CLOCK =================
    int midiClockCounter = 0;
    bool externalClockRunning = false;
    // ================= SWING STATE =================
    int64_t globalStepCounter = 0;

    bool isPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Euclidean_seqAudioProcessor)
};
#pragma once

