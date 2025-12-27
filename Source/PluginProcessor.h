#pragma once
#include <JuceHeader.h>
#include "MidiGenerator.h"

struct MidiPortEvent
{
    int rhythmIndex;        // 0–5 (CHIAVE CORRETTA)
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

    // ================== NOTE SOURCE TYPES ==================
    enum class NoteSourceType
    {
        Single,
        Scale,
        Chord
    };

    // Struttura per memorizzare le info scelte da GUI
    struct NoteSource
    {
        NoteSourceType type = NoteSourceType::Single;
        int value = 60; // default C4 per Single, o ID di Scale/Chord
    };

    // Array per ogni row
    NoteSource noteSources[6];

    // ===== MIDI OUTPUT ENUMERATION =====
    const juce::Array<juce::MidiDeviceInfo>& getAvailableMidiOutputs() const
    {
        return availableMidiOutputs;
    }

    // ===== MIDI OUTPUT ENUMERATION & HOT-PLUG (REFRESH EDITOR SAFE) =====
    /** Aggiorna lista porte MIDI disponibili, chiude porte non valide e gestisce hot-plug */
    void refreshMidiOutputs();
    void updateMidiOutputForRhythm(int rhythmIndex, int deviceIndex);

private:
    // ===== MIDI OUTPUT PORTS (GLOBAL LIST) =====
    juce::Array<juce::MidiDeviceInfo> availableMidiOutputs;

    // ===== PER-RHYTHM MIDI OUTPUT =====
    struct RhythmMidiOut
    {
        int selectedPortIndex = -1;
        int selectedChannel = 1; // 1–16
        std::unique_ptr<juce::MidiOutput> output;
    };

    std::array<RhythmMidiOut, 6> rhythmMidiOuts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    std::vector<MidiPortEvent> stagedMidiEvents;

    // ===== BASS (R6) LEGATO / GLIDE STATE =====
    int lastBassNote = -1;
    bool bassNoteActive = false;

    // Glide / Portamento
    double bassGlideTimeSeconds = 0.08; // default 80 ms
    double bassGlideProgress = 0.0;
    int bassStartNote = -1;
    int bassTargetNote = -1;
    bool bassGlideActive = false;

    // ===== BASS GLIDE CURVE / RANGE =====
    float bassGlideCurve = 0.6f;   // 0 = lineare, <1 log, >1 exp
    int bassPitchBendRange = 12;   // semitoni (2 o 12 tipico)

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

    std::atomic<bool> globalPlayState{ false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Euclidean_seqAudioProcessor)
};


