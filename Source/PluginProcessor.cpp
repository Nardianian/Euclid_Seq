#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Euclidean_seqAudioProcessor::Euclidean_seqAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, "Params", createParameterLayout())
{
    // ===== ENUMERATE MIDI OUTPUT PORTS =====
    refreshMidiOutputs();

    // ===== INIT RHYTHM MIDI STATE =====
    for (auto& r : rhythmMidiOuts)
    {
        r.selectedPortIndex = -1;
        r.selectedChannel = 1;
    }

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
            "rhythm" + String(i) + "_mute", "Mute", false));

        layout.add(std::make_unique<AudioParameterBool>(
            "rhythm" + String(i) + "_solo", "Solo", false));

        layout.add(std::make_unique<AudioParameterBool>(
            "rhythm" + String(i) + "_reset", "Reset", false));

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

        layout.add(std::make_unique<AudioParameterChoice>(
            "rhythm" + String(i) + "_arpRate",
            "ARP Rate",
            StringArray{ "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16D" },
            2)); // default = 1/4

        layout.add(std::make_unique<AudioParameterInt>(
            "rhythm" + String(i) + "_arpNotesMask",
            "ARP Notes Mask",
            0,
            127,
            0b0001111)); // default: prime 4 note attive

        // ===== MIDI PORT (dinamico, allineato alla ComboBox) =====
        juce::StringArray midiPortNames;
        auto midiDevices = juce::MidiOutput::getAvailableDevices();

        for (const auto& dev : midiDevices)
            midiPortNames.add(dev.name);

        if (midiPortNames.isEmpty())
            midiPortNames.add("No MIDI Outputs");

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

    layout.add(std::make_unique<AudioParameterFloat>(
        "bassGlideTime",
        "Bass Glide Time",
        NormalisableRange<float>(0.01f, 0.5f),
        0.08f));

    layout.add(std::make_unique<AudioParameterFloat>(
        "bassGlideCurve",
        "Bass Glide Curve",
        NormalisableRange<float>(0.2f, 2.0f),
        0.6f));

    layout.add(std::make_unique<AudioParameterChoice>(
        "bassPitchBendRange",
        "Bass Pitch Bend Range",
        juce::StringArray({ "±2 semitones", "±12 semitones" }),
        1));

    layout.add(std::make_unique<AudioParameterBool>(
        "bassMode",
        "Bass Mono Mode",
        true));

    layout.add(std::make_unique<AudioParameterBool>(
        "globalPlay",
        "Global Play",
        false));

    return layout;
}
//==============================================================================
void Euclidean_seqAudioProcessor::refreshMidiOutputs()
{
    // 1) Legge lista attuale dispositivi MIDI
    auto currentDevices = juce::MidiOutput::getAvailableDevices();

    // 2) Confronta con lista precedente per rilevare cambi
    bool changed = (currentDevices != availableMidiOutputs);

    if (!changed)
        return; // nessuna modifica

    // 3) Aggiorna lista globale
    availableMidiOutputs = currentDevices;

    // 4) Chiudi porte non più valide
    for (auto& r : rhythmMidiOuts)
    {
        if (r.selectedPortIndex >= availableMidiOutputs.size())
        {
            r.output.reset();
            r.selectedPortIndex = -1; // reset sicuro
        }
    }
    // NOTA: aggiornamento ComboBox avverrà in Editor tramite timer o callback
}

void Euclidean_seqAudioProcessor::updateMidiOutputForRhythm(int rhythmIndex, int deviceIndex)
{
    if (rhythmIndex < 0 || rhythmIndex >= 6)
        return;

    auto& r = rhythmMidiOuts[rhythmIndex];

    if (deviceIndex < 0 || deviceIndex >= availableMidiOutputs.size())
    {
        r.output.reset();
        r.selectedPortIndex = -1;
        return;
    }

    if (r.selectedPortIndex == deviceIndex && r.output)
        return;

    r.output.reset();
    r.output = juce::MidiOutput::openDevice(
        availableMidiOutputs[deviceIndex].identifier);

    r.selectedPortIndex = deviceIndex;
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
    // ================== SYNCHRONIZE NOTE SOURCE FROM PARAMETERS ==================
    for (int r = 0; r < 6; ++r)
    {
        if (auto* noteParam = parameters.getParameter("rhythm" + juce::String(r) + "_note"))
        {
            float val = noteParam->getValue(); // 0..1
            int intVal = (int)(val * 10000.0f); // convertire in range più utile
            // Decidere tipo
            if (intVal < 1000)
            {
                noteSources[r].type = NoteSourceType::Single;
                noteSources[r].value = intVal + 36; // 36->84 come in GUI
            }
            else if (intVal < 2000)
            {
                noteSources[r].type = NoteSourceType::Scale;
                noteSources[r].value = intVal; // ID della scala
            }
            else
            {
                noteSources[r].type = NoteSourceType::Chord;
                noteSources[r].value = intVal; // ID dell'accordo
            }
        }
    }

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
            bassNoteActive = false;
            lastBassNote = -1;
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

    // ===== UPDATE ARP ENABLE FLAGS (once per block) =====
    for (int r = 0; r < 6; ++r)
    {
        bool arpOn = parameters.getRawParameterValue(
            "rhythm" + juce::String(r) + "_arpActive")->load() > 0.5f;

        midiGen.arpEnabled[r].store(arpOn);
    }

    const int numSamples = buffer.getNumSamples();

    // ================= READ CLOCK SOURCE =================
    auto* clockSourceParam = parameters.getRawParameterValue("clockSource");
    ClockSource clockSource = static_cast<ClockSource>((int)clockSourceParam->load());

    double bpm = 120.0;
    bool dawPlaying = false;

#if JUCE_STANDALONE
    // ================= STANDALONE =====================
    // ================= INTERNAL CLOCK =================
    if (clockSource == ClockSource::Internal)
    {
        bpm = parameters.getRawParameterValue("clockBPM")->load();
        dawPlaying = true;
    }
#else
    // ===== PLUGIN =====
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
    else if (clockSource == ClockSource::Internal)
    {
        bpm = parameters.getRawParameterValue("clockBPM")->load();
        dawPlaying = true;
    }
#endif
    else
    {
        bpm = parameters.getRawParameterValue("clockBPM")->load();
        dawPlaying = true;
    }

    bool globalPlay =
        parameters.getRawParameterValue("globalPlay")->load() > 0.5f;

    globalPlayState.store(globalPlay);

    // clock fermo → stop
    if (clockSource != ClockSource::External && !dawPlaying)
    {
        midiMessages.clear();
        return;
    }

    // PLAY globale fermo → stop
    if (!globalPlayState.load())
    {
        midiMessages.clear();
        return;
    }

    // ================= MUSICAL TIME =================
    samplesPerBeat = currentSampleRate * 60.0 / bpm;

    // 1 step = 1/16 note (standard Euclidean grid)
    samplesPerStep = samplesPerBeat / 4.0;

    // ===== BASS (R6) GLIDE TIME =====
    bassGlideTimeSeconds =
        parameters.getRawParameterValue("bassGlideTime")->load();

    bassGlideCurve =
        parameters.getRawParameterValue("bassGlideCurve")->load();

    bassPitchBendRange =
        (parameters.getRawParameterValue("bassPitchBendRange")->load() < 0.5f)
        ? 2
        : 12;

    // ================== READ PARAMETERS (DAW-SAFE) ==================
    for (int i = 0; i < 6; ++i)
    {
        auto* arpActive = parameters.getRawParameterValue(
            "rhythm" + juce::String(i) + "_arpActive");

        auto* arpMode = parameters.getRawParameterValue(
            "rhythm" + juce::String(i) + "_arpMode");

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

            auto* arpRateParam = parameters.getRawParameterValue(
                "rhythm" + juce::String(i) + "_arpRate");
            // Mapping musicale → moltiplicatore
           static const double arpRateTable[] =
           {
            1.0,    // 1/1
            0.5,    // 1/2
            0.25,   // 1/4
            0.125,  // 1/8
            0.0625, // 1/16
            0.03125,// 1/32
            0.1666667, // 1/8T
            0.375      // 1/16D
           };
           int rateIndex = juce::jlimit(0, 7, (int)arpRateParam->load());
           midiGen.rhythmArps[i].setRate(arpRateTable[rateIndex]);

           auto* arpNotesMask = parameters.getRawParameterValue(
               "rhythm" + juce::String(i) + "_arpNotesMask");
           
           midiGen.arpNotes[i].clear();
           
           int mask = (int)arpNotesMask->load();
           for (int n = 0; n < 7; ++n)
           {
               if (mask & (1 << n))
                   midiGen.arpNotes[i].push_back(60 + n); // C, C#, D...
           }
        }
        else
        {
            // ===== ARP DISABLED → RESET =====
            midiGen.rhythmArps[i].reset();
            midiGen.arpNotes[i].clear();
        }
    }
    // ================= RESET PER-RHYTHM (one-shot, pre-sample) =================
    for (int r = 0; r < 6; ++r)
    {
        auto* resetParam = parameters.getRawParameterValue(
            "rhythm" + juce::String(r) + "_reset");

        if (resetParam != nullptr && resetParam->load() > 0.5f)
        {
            // Reset pattern + arp
            midiGen.rhythmPatterns[r].reset();
            midiGen.rhythmArps[r].reset();

            // Reset timing
            stepCounterPerRhythm[r] = 0;
            nextStepSamplePerRhythm[r] = globalSampleCounter;
            stepMicrotiming[r].clear();

            // Reset bass state if R6
            if (r == 5)
            {
                bassNoteActive = false;
                bassGlideActive = false;
                lastBassNote = -1;
            }

            // Reset one-shot parameter
            if (auto* p = parameters.getParameter(
                "rhythm" + juce::String(r) + "_reset"))
            {
                p->setValueNotifyingHost(0.0f);
            }
        }
    }

    // ================= SAMPLE-ACCURATE PROCESSING =================
    for (int sample = 0; sample < numSamples; ++sample)
    {
        bool anySoloActive = false;
        for (int r = 0; r < 6; ++r)
        {
            if (parameters.getRawParameterValue("rhythm" + juce::String(r) + "_solo")->load() > 0.5f)
            {
                anySoloActive = true;
                break;
            }
        }

        for (int r = 0; r < 6; ++r)
        {
            // ===== ACTIVE / MUTE / SOLO check =====
            const bool isActive =
                *parameters.getRawParameterValue("rhythm" + juce::String(r) + "_active") > 0.5f;

            if (!isActive)
                continue;

            const bool isMuted =
                *parameters.getRawParameterValue("rhythm" + juce::String(r) + "_mute") > 0.5f;

            const bool isSolo =
                *parameters.getRawParameterValue("rhythm" + juce::String(r) + "_solo") > 0.5f;

            // SOLO logic
            if (anySoloActive && !isSolo)
                continue;

            // ===== SAMPLES PER ARP STEP =====
            double samplesPerArpStep = samplesPerStep; // default = 1 step
            if (parameters.getRawParameterValue("rhythm" + juce::String(r) + "_arpActive")->load() > 0.5f)
            {
                auto* arpRateParam = parameters.getRawParameterValue("rhythm" + juce::String(r) + "_arpRate");
                static const double arpRateTable[] =
                { 1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125, 0.1666667, 0.375 };
                int rateIndex = juce::jlimit(0, 7, (int)arpRateParam->load());
                double arpRate = arpRateTable[rateIndex];
                samplesPerArpStep = samplesPerStep * arpRate;
            }

            // Se mutato → non emettere MIDI (ma il clock avanza)
            if (isMuted)
            {
                midiGen.advanceArp(r, samplesPerStep, samplesPerArpStep);
                continue;
            }

            // 4.4: RESET edge-trigger
            const bool doReset = *parameters.getRawParameterValue("rhythm" + juce::String(r) + "_reset") > 0.5f;
            if (doReset)
            {
                midiGen.rhythmPatterns[r].reset();
                midiGen.rhythmArps[r].reset();

                if (auto* resetParam = parameters.getParameter("rhythm" + juce::String(r) + "_reset"))
                    resetParam->setValueNotifyingHost(0.0f);
            }

            // ===== READ MIDI OUTPUT PORT (per rhythm) =====
            int midiPort = (int)parameters.getRawParameterValue(
                "rhythm" + juce::String(r) + "_midiPort")->load();

            // ===== UPDATE REAL MIDI OUTPUT =====
            // AudioParameterChoice usa index-based values
            updateMidiOutputForRhythm(r, midiPort);

            bool stepTriggered = false;

            // ===== ARP SAMPLE-ACCURATE ADVANCE =====
            if (parameters.getRawParameterValue(
                "rhythm" + juce::String(r) + "_arpActive")->load() > 0.5f)
            {
                auto* arpRateParam = parameters.getRawParameterValue(
                    "rhythm" + juce::String(r) + "_arpRate");

                static const double arpRateTable[] =
                {
                    1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125, 0.1666667, 0.375
                };

                int rateIndex = juce::jlimit(0, 7, (int)arpRateParam->load());
                double arpRate = arpRateTable[rateIndex];

                double samplesPerArpStep = samplesPerStep * arpRate;
                midiGen.rhythmArps[r].advanceSamples(1.0, samplesPerArpStep);
            }

            // ===== EXTERNAL MIDI CLOCK (sample-accurate, jitter-free) =====
            if (clockSource == ClockSource::External && externalClockRunning)
            {
                // 24 MIDI clock ticks = 1 beat
                constexpr double ticksPerBeat = 24.0;

                // samples per MIDI clock tick
                const double samplesPerTick = samplesPerBeat / ticksPerBeat;

                // accumulatore per rhythm (persistente)
                static double extClockSampleAccum[6] = { 0.0 };

                // aggiungiamo i sample corrispondenti ai tick ricevuti
                if (midiClockCounter > 0)
                {
                    extClockSampleAccum[r] += midiClockCounter * samplesPerTick;
                    midiClockCounter = 0;
                }

                // trigger step SOLO quando il tempo musicale è passato
                if (extClockSampleAccum[r] >= samplesPerStep)
                {
                    extClockSampleAccum[r] -= samplesPerStep;
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
            int baseNote = (int)parameters.getRawParameterValue(
                "rhythm" + juce::String(r) + "_note")->load();

            // ================== GENERA NOTE MIDI PER QUESTO STEP ==================
            std::vector<int> generatedNotes;

            // Usa la funzione del generatore Euclidean per ottenere le note da noteSources
            switch (noteSources[r].type)
            {
            case NoteSourceType::Single:
                generatedNotes.push_back(noteSources[r].value);
                break;

            case NoteSourceType::Scale:
                generatedNotes = midiGen.getScaleNotes(noteSources[r].value); // funzione da implementare in midiGen
                break;

            case NoteSourceType::Chord:
                generatedNotes = midiGen.getChordNotes(noteSources[r].value); // funzione da implementare in midiGen
                break;
            }

            // ===== UPDATE ARP NOTE SLOTS (max 7 note) =====
            if (parameters.getRawParameterValue(
                "rhythm" + juce::String(r) + "_arpActive")->load() > 0.5f)
            {
                if (generatedNotes.size() > 7)
                    generatedNotes.resize(7);

                midiGen.setArpNotes(r, generatedNotes);
            }

            // Per ora prendiamo la prima nota come baseNote
            if (!generatedNotes.empty())
                baseNote = generatedNotes[0];

            bool shouldTrigger = false;

            // ===== EUCLIDEAN / ARP TRIGGER DECISION =====
            if (parameters.getRawParameterValue(
                "rhythm" + juce::String(r) + "_arpActive")->load() > 0.5f)
            {
                // ARP attivo → trigger solo se l'ARP è avanzato
                if (midiGen.rhythmArps[r].advanceSamples(0.0, 0.0)) // stato già avanzato sopra
                {
                    shouldTrigger = midiGen.triggerStep(r, baseNote, outMidiNote);
                }
            }
            else
            {
                // ARP disattivo → trigger euclideo normale
                shouldTrigger = midiGen.triggerStep(r, baseNote, outMidiNote);
            }

            if (shouldTrigger)
            {
                // ===== READ MIDI OUTPUT PORT INDEX =====
                int midiPortIndex = 0;
                if (auto* portParam = parameters.getRawParameterValue(
                    "rhythm" + juce::String(r) + "_midiPort"))
                {
                    midiPortIndex = (int)portParam->load();
                }

                // ===== BUILD PORT TAG (SysEx) =====
                const juce::uint8 sysExData[2] =
                {
                    0x7D, // non-commercial SysEx ID
                    static_cast<juce::uint8>(midiPortIndex)
                };

                juce::MidiMessage portTag =
                    juce::MidiMessage::createSysExMessage(sysExData, 2);

                // ===== PUSH PORT TAG (FIRST EVENT) =====
                stagedMidiEvents.push_back({
                    r,              // rhythmIndex
                    midiPortIndex,  // deviceIndex
                    portTag,
                    sample
                    });

                // ===== VELOCITY =====
                int velocity = 100;
                if (auto* velParam = parameters.getRawParameterValue(
                    "rhythm" + juce::String(r) + "_velocityAccent"))
                {
                    velocity = juce::jlimit(1, 127, (int)velParam->load());
                }

                // ===== NOTE LENGTH (sample-accurate) =====
                double noteLength = 0.5;
                if (auto* lenParam = parameters.getRawParameterValue(
                    "rhythm" + juce::String(r) + "_noteLength"))
                {
                    noteLength = lenParam->load();
                }

                int noteOffSample = sample + int(samplesPerStep * noteLength);

                // ===== BASS R6 — TRUE MONO LEGATO + MIDI GLIDE =====
                if (r == 5)
                {
                    if (bassNoteActive)
                    {
                        // LEGATO: no NoteOff
                        bassStartNote = lastBassNote;
                        bassTargetNote = outMidiNote;

                        bassGlideActive = true;
                        bassGlideProgress = 0.0;
                    }
                    else
                    {
                        bassNoteActive = true;
                    }

                    // ONE NoteOn only
                    stagedMidiEvents.push_back({
                        r,
                        midiPortIndex,
                        juce::MidiMessage::noteOn(
                            midiChannel,
                            outMidiNote,
                            (juce::uint8)velocity),
                        sample
                        });

                    lastBassNote = outMidiNote;

                    // Conditional NoteOff
                    int noteOffSampleAdjusted = sample + int(samplesPerStep * noteLength);
                    if (noteOffSampleAdjusted >= numSamples)
                    {
                        pendingNoteOffs.push_back(
                            { midiChannel, outMidiNote,
                              noteOffSampleAdjusted - numSamples });
                    }
                    else
                    {
                        stagedMidiEvents.push_back({
                            r,
                            midiPortIndex,
                            juce::MidiMessage::noteOff(midiChannel, outMidiNote),
                            noteOffSampleAdjusted
                            });
                    }

                    continue; // 🔴 IMPORTANT
                }

                // ===== NORMAL RHYTHMS =====
                stagedMidiEvents.push_back({
                    r,
                    midiPortIndex,
                    juce::MidiMessage::noteOn(
                        midiChannel,
                        outMidiNote,
                        (juce::uint8)velocity),
                    sample
                    });

                stagedMidiEvents.push_back({
                    r,
                    midiPortIndex,
                    juce::MidiMessage::noteOff(midiChannel, outMidiNote),
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

        // ===== BASS R6 — MIDI GLIDE (Pitch Bend) =====
        if (bassGlideActive)
        {
            double glideSamplesTotal = bassGlideTimeSeconds * currentSampleRate;
            bassGlideProgress += 1.0;

            double t = bassGlideProgress / glideSamplesTotal;
            if (t >= 1.0)
            {
                t = 1.0;
                bassGlideActive = false;
            }

            // ===== ANALOG-STYLE CURVE =====
            // curve < 1.0  -> log
            // curve = 1.0  -> linear
            // curve > 1.0  -> exp
            t = std::pow(t, bassGlideCurve);

            // Pitch Bend range: ±2 semitoni (standard)
            double semitoneDelta = (bassTargetNote - bassStartNote) * t;
            double normalized = semitoneDelta / (double)bassPitchBendRange;

            int pitchValue = juce::jlimit(
                0, 16383,
                8192 + int(normalized * 8192.0)
            );

            juce::MidiMessage bend =
                juce::MidiMessage::pitchWheel(1, pitchValue);

            midiMessages.addEvent(bend, sample);
        }
    }
    // ===== FLUSH MIDI EVENTS (REAL MULTI-PORT MIDI OUT) =====
    for (const auto& e : stagedMidiEvents)
    {
        // 1) DAW MIDI
        midiMessages.addEvent(e.message, e.sampleOffset);

        // 2) MIDI REALE (per rhythm)
        if (e.rhythmIndex >= 0 && e.rhythmIndex < 6)
        {
            auto& r = rhythmMidiOuts[e.rhythmIndex];
            if (r.output)
                r.output->sendMessageNow(e.message);
        }
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








