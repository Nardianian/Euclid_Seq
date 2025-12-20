/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Euclidean_seqAudioProcessor::Euclidean_seqAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, "Params", createParameterLayout())
{

    // Initialize default rhythm patterns and arp notes
    for (int i = 0; i < 6; ++i)
    {
        midiGen.rhythmPatterns[i].setPattern(16, 4);
        midiGen.rhythmArps[i].setType(ArpType::UP);
        midiGen.arpNotes[i].resize(4, 60);
    }
}

Euclidean_seqAudioProcessor::~Euclidean_seqAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
Euclidean_seqAudioProcessor::createParameterLayout()
{
    using namespace juce;

    AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < 6; ++i)
    {
        layout.add(std::make_unique<AudioParameterBool>(
            "rhythm" + String(i) + "_active", "Active", true));

        layout.add(std::make_unique<AudioParameterFloat>(
            "rhythm" + String(i) + "_note", "Note",
            NormalisableRange<float>(0.0f, 127.0f), 60.0f));

        layout.add(std::make_unique<AudioParameterFloat>(
            "rhythm" + String(i) + "_steps", "Steps",
            NormalisableRange<float>(1.0f, 32.0f), 16.0f));

        layout.add(std::make_unique<AudioParameterFloat>(
            "rhythm" + String(i) + "_pulses", "Pulses",
            NormalisableRange<float>(0.0f, 32.0f), 4.0f));

        layout.add(std::make_unique<AudioParameterFloat>(
            "rhythm" + String(i) + "_swing", "Swing",
            NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        layout.add(std::make_unique<AudioParameterFloat>(
            "rhythm" + String(i) + "_microtiming", "Microtiming",
            NormalisableRange<float>(-0.5f, 0.5f), 0.0f));

        layout.add(std::make_unique<AudioParameterFloat>(
            "rhythm" + String(i) + "_velocityAccent", "Velocity",
            NormalisableRange<float>(0.0f, 127.0f), 100.0f));

        layout.add(std::make_unique<AudioParameterFloat>(
            "rhythm" + String(i) + "_noteLength", "Note Length",
            NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        layout.add(std::make_unique<AudioParameterBool>(
            "rhythm" + String(i) + "_arpActive", "ARP Active", false));

        layout.add(std::make_unique<AudioParameterChoice>(
            "rhythm" + String(i) + "_arpMode", "ARP Mode",
            StringArray({ "UP", "DOWN", "UP_DOWN", "RANDOM" }), 0));

        layout.add(std::make_unique<AudioParameterFloat>(
            "rhythm" + String(i) + "_arpRate", "ARP Rate",
            NormalisableRange<float>(0.1f, 4.0f), 1.0f));

        layout.add(std::make_unique<AudioParameterFloat>(
            "rhythm" + String(i) + "_arpNotes", "ARP Notes",
            NormalisableRange<float>(1.0f, 16.0f), 4.0f));

        // MIDI Port
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "rhythm" + juce::String(i) + "_midiPort",
            "MIDI Port",
            juce::StringArray({ "Port1", "Port2", "Port3", "Port4" }), 0));

        // MIDI Channel
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "rhythm" + juce::String(i) + "_midiChannel",
            "MIDI Channel",
            juce::StringArray({ "1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16" }), 0));
    }

    layout.add(std::make_unique<AudioParameterChoice>(
        "clockSource", "Clock Source",
        StringArray({ "DAW", "Internal", "External" }), 0));

    layout.add(std::make_unique<AudioParameterFloat>(
        "clockBPM", "Clock BPM",
        NormalisableRange<float>(40.0f, 300.0f), 120.0f));

    return layout;
}
//==============================================================================
void Euclidean_seqAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    globalSampleCounter = 0;  // reset contatore globale dei sample
    for (int r = 0; r < 6; ++r)
    {
        nextStepSamplePerRhythm[r] = 0;  // reset clock per rhythm
        stepCounterPerRhythm[r] = 0;     // reset step counter
        stepMicrotiming[r].clear();      // microtiming inizializzato vuoto
    }

    midiClockCounter = 0;                // reset contatore clock esterno
    externalClockRunning = false;        // reset stato clock esterno
}

void Euclidean_seqAudioProcessor::releaseResources()
{
}

//==============================================================================
void Euclidean_seqAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // ================= EXTERNAL MIDI CLOCK INPUT =================
    for (const auto metadata : midiMessages)
    {
        const auto& msg = metadata.getMessage();

        if (msg.isMidiStart())
        {
            externalClockRunning = true;
            midiClockCounter = 0;

            for (int r = 0; r < 6; ++r)
            {
                stepCounterPerRhythm[r] = 0;
                nextStepSamplePerRhythm[r] = globalSampleCounter;
            }
        }
        else if (msg.isMidiStop())
        {
            externalClockRunning = false;
        }
        else if (msg.isMidiContinue())
        {
            externalClockRunning = true;

            for (int r = 0; r < 6; ++r)
                nextStepSamplePerRhythm[r] = globalSampleCounter;
        }
        else if (msg.isMidiClock())
        {
            if (externalClockRunning)
                midiClockCounter++;
        }
    }

    buffer.clear();

    const int numSamples = buffer.getNumSamples();

    // ================= READ CLOCK SOURCE =================
    auto* clockSourceParam = parameters.getRawParameterValue("clockSource");
    ClockSource clockSource = static_cast<ClockSource>((int)clockSourceParam->load());

    double bpm = 120.0;
    bool dawPlaying = false;

    // ================= DAW CLOCK =================
    if (clockSource == ClockSource::DAW)
    {
        if (auto* playHead = getPlayHead())
        {
            juce::AudioPlayHead::CurrentPositionInfo pos;
            if (playHead->getCurrentPosition(pos))
            {
                bpm = pos.bpm > 0.0 ? pos.bpm : 120.0;
                dawPlaying = pos.isPlaying;
            }
        }
    }
    // ================= INTERNAL CLOCK =================
    else
    {
        bpm = parameters.getRawParameterValue("clockBPM")->load();
        dawPlaying = true;
    }

    if (clockSource != ClockSource::External && !dawPlaying)
    {
        midiMessages.clear();
        return;
    }

    // ================= MUSICAL TIME =================
    samplesPerBeat = currentSampleRate * 60.0 / bpm;

    // 1 step = 1/16 note (standard Euclidean grid)
    samplesPerStep = samplesPerBeat / 4.0;

    // ================== READ PARAMETERS (DAW-SAFE) ==================
    for (int i = 0; i < 6; ++i)
    {
        auto* arpActive = parameters.getRawParameterValue(
            "rhythm" + juce::String(i) + "_arpActive");

        auto* arpMode = parameters.getRawParameterValue(
            "rhythm" + juce::String(i) + "_arpMode");

        auto* arpRate = parameters.getRawParameterValue(
            "rhythm" + juce::String(i) + "_arpRate");

        auto* arpNotes = parameters.getRawParameterValue(
            "rhythm" + juce::String(i) + "_arpNotes");

        auto* steps = parameters.getRawParameterValue(
            "rhythm" + juce::String(i) + "_steps");

        auto* pulses = parameters.getRawParameterValue(
            "rhythm" + juce::String(i) + "_pulses");

        midiGen.rhythmPatterns[i].setPattern(
            (int)steps->load(),
            (int)pulses->load());

        int numSteps = (int)steps->load();

        // Resize microtiming array if needed
        if ((int)stepMicrotiming[i].size() != numSteps)
        {
            stepMicrotiming[i].resize(numSteps, 0.0);
        }


        if (arpActive->load() > 0.5f)
        {
            midiGen.rhythmArps[i].setType(
                static_cast<ArpType>((int)arpMode->load()));

            midiGen.rhythmArps[i].setRate(arpRate->load());

            midiGen.arpNotes[i].resize(
                (int)arpNotes->load(), 60);
        }
    }
// ================= SAMPLE-ACCURATE PROCESSING =================
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int r = 0; r < 6; ++r)
        {
            bool stepTriggered = false;

            // ===== EXTERNAL MIDI CLOCK =====
            if (clockSource == ClockSource::External)
            {
                if (externalClockRunning && midiClockCounter >= 6)
                {
                    midiClockCounter -= 6;
                    stepTriggered = true;
                }
            }
            // ===== DAW / INTERNAL CLOCK =====
            else
            {
                if (globalSampleCounter >= nextStepSamplePerRhythm[r])
                    stepTriggered = true;
            }

            if (!stepTriggered)
                continue;

            // ===== READ MIDI CHANNEL (per rhythm) =====
            int midiChannel = 1;
            if (auto* chParam = parameters.getRawParameterValue(
                "rhythm" + juce::String(r) + "_midiChannel"))
            {
                midiChannel = (int)chParam->load() + 1; // 0–15 → 1–16
            }

            int outMidiNote = 0;

            // ===== TRIGGER EUCLIDEAN / ARP / NOTE ON =====
            if (midiGen.triggerStep(r, outMidiNote))
            {
                int midiPortIndex = 0;
                if (auto* portParam = parameters.getRawParameterValue(
                    "rhythm" + juce::String(r) + "_midiPort"))
                {
                    midiPortIndex = (int)portParam->load(); // 0..N
                }

                juce::MidiMessage noteOn =
                    juce::MidiMessage::noteOn(midiChannel, outMidiNote, (juce::uint8)100);

                stagedMidiEvents.push_back({
                    midiPortIndex,
                    noteOn,
                    sample
                    });

                // ===== NOTE LENGTH (sample-accurate) =====
                double noteLength = 0.5;
                if (auto* lenParam = parameters.getRawParameterValue(
                    "rhythm" + juce::String(r) + "_noteLength"))
                {
                    noteLength = lenParam->load();
                }

                int noteOffSample = sample + int(samplesPerStep * noteLength);

                // ===== BUFFER-CROSSING NOTE-OFF (solo basso r == 5) =====
                if (r == 5 && noteOffSample >= numSamples)
                {
                    pendingNoteOffs.push_back({
                        midiChannel,
                        outMidiNote,
                        noteOffSample - numSamples
                        });
                    noteOffSample = numSamples - 1;
                }
                // ===== NOTE OFF =====
                juce::MidiMessage noteOff =
                    juce::MidiMessage::noteOff(midiChannel, outMidiNote);

                stagedMidiEvents.push_back({
                    midiPortIndex,
                    noteOff,
                    noteOffSample
                    });
            }
            // ===== SWING =====
            double swingAmount = 0.0;
            if (auto* swingParam = parameters.getRawParameterValue(
                "rhythm" + juce::String(r) + "_swing"))
            {
                swingAmount = swingParam->load();
            }

            bool isOffBeat = (stepCounterPerRhythm[r] % 2) == 1;
            double swingOffset =
                isOffBeat ? samplesPerStep * 0.5 * swingAmount : 0.0;

            // ===== MICROTIMING =====
            double microOffset = 0.0;
            if (!stepMicrotiming[r].empty())
            {
                int idx = stepCounterPerRhythm[r] % stepMicrotiming[r].size();
                microOffset = stepMicrotiming[r][idx] * samplesPerStep;
            }

            // ===== ADVANCE STEP =====
            nextStepSamplePerRhythm[r] +=
                (int64_t)(samplesPerStep + swingOffset + microOffset);

            stepCounterPerRhythm[r]++;
        }

        // ===== APPLY PENDING NOTE-OFFS (buffer crossing) =====
        for (auto it = pendingNoteOffs.begin(); it != pendingNoteOffs.end(); )
        {
            it->remainingSamples--;
            if (it->remainingSamples <= 0)
            {
                midiMessages.addEvent(
                    juce::MidiMessage::noteOff(it->channel, it->note),
                    sample);
                it = pendingNoteOffs.erase(it);
            }
            else
            {
                ++it;
            }
        }

        globalSampleCounter++;
    }
    // ===== FLUSH MIDI EVENTS (multi-port logical routing) ===== //cfr se mettere prima di graffa blu superiore [da for a clear()]
    for (const auto& e : stagedMidiEvents)
    {
        // Attualmente: tutte le porte finiscono nello stesso MidiBuffer
        // Il portIndex può essere usato da host / script / future routing
        midiMessages.addEvent(e.message, e.sampleOffset);
    }

    stagedMidiEvents.clear();
}
//==============================================================================
juce::AudioProcessorEditor* Euclidean_seqAudioProcessor::createEditor()
{
    return new Euclidean_seqAudioProcessorEditor(*this);
}

//==============================================================================
void Euclidean_seqAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void Euclidean_seqAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Euclidean_seqAudioProcessor();
}

