#pragma once 
#include <JuceHeader.h>
#include "MidiGenerator.h"
#include "ClockManager.h"
// Per thread-safety nell'ARP
#include <mutex>
#include <atomic>
#include <vector>

struct MidiPortEvent
{
    int rhythmIndex;        // 0–5
    int deviceIndex;        // MIDI device
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

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Euclidean_seq"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    MidiGenerator midiGen;

    ClockManager clock;

    // ================== NOTE SOURCE TYPES ==================
    enum class NoteSourceType
    {
        Single,
        Scale,
        Chord
    };

    struct NoteSource
    {
        NoteSourceType type = NoteSourceType::Single;
        int value = 60; // default C4 per Single, o ID di Scale/Chord
    };

    NoteSource noteSources[6];

    // ===== Thread-safe / GUI state for selected notes in the ARP =====
    std::array<std::vector<bool>, 6> arpNoteSelected; // ogni riga: 128 elementi

    const juce::Array<juce::MidiDeviceInfo>& getAvailableMidiOutputs() const
    {
        return availableMidiOutputs;
    }

    void refreshMidiOutputs();
    void updateMidiOutputForRhythm(int rhythmIndex, int deviceIndex);
    bool isRowActive(int row) const;

private:
    juce::Array<juce::MidiDeviceInfo> availableMidiOutputs;

    struct RhythmMidiOut
    {
        int selectedPortIndex = -1;
        int selectedChannel = 1;
        std::unique_ptr<juce::MidiOutput> output;
    };
    std::array<RhythmMidiOut, 6> rhythmMidiOuts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    std::vector<MidiPortEvent> stagedMidiEvents;

    int lastBassNote = -1;
    bool bassNoteActive = false;

    double bassGlideTimeSeconds = 0.08;
    double bassGlideProgress = 0.0;
    int bassStartNote = -1;
    int bassTargetNote = -1;
    bool bassGlideActive = false;

    float bassGlideCurve = 0.6f;
    int bassPitchBendRange = 12;

    enum class ClockSource { DAW = 0, Internal, External };
    ClockSource clockSource = ClockSource::Internal;
    bool externalClockRunning = false;
    bool isMuted = false;

    double currentSampleRate = 44100.0;
    double samplesPerBeat = 0.0;
    double samplesPerStep = 0.0;
    double internalBpm = 120.0;

    int64_t globalSampleCounter = 0;
    std::array<int64_t, 6> nextStepSamplePerRhythm{};
    std::array<int64_t, 6> stepCounterPerRhythm{};
    std::array<std::vector<double>, 6> stepMicrotiming;

    struct PendingNoteOff
    {
        int channel = 1;
        int note = 0;
        int remainingSamples = 0;
    };
    std::vector<PendingNoteOff> pendingNoteOffs;

    int midiClockCounter = 0;
    int64_t globalStepCounter = 0;
    bool isPlaying = false;
    std::atomic<bool> globalPlayState{ false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Euclidean_seqAudioProcessor)
};

