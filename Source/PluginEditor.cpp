#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float mmToPx = 96.0f / 25.4f;
}

//==============================================================================
Euclidean_seqAudioProcessorEditor::Euclidean_seqAudioProcessorEditor(Euclidean_seqAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(baseWidth, baseHeight);
    setResizable(true, true);
    setResizeLimits(baseWidth / 2, baseHeight / 2, baseWidth * 2, baseHeight * 2);
    getConstrainer()->setFixedAspectRatio((double)baseWidth / (double)baseHeight);

    addAndMakeVisible(clockSettingsButton);
    clockSettingsButton.onClick = [this]() { openClockSettingsPopup(); };

    // ================= GLOBAL PLAY =================
    addAndMakeVisible(playButton);
    playButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
    playButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    playButton.setClickingTogglesState(true);
    playButton.setTooltip("Global Play / Stop");

    playAttachment = std::make_unique<ButtonAttachment>(
        audioProcessor.parameters,
        "globalPlay",
        playButton);

    // ================= RANDOMIZE BUTTON =================
    addAndMakeVisible(randomizeButton);
    randomizeButton.setButtonText("Randomize");
    randomizeButton.onClick = [this]()
        {
            for (int r = 0; r < 6; ++r)
            {
                auto& row = rhythmRows[r];
                auto& params = audioProcessor.parameters;

                if (auto* noteParam = params.getParameter("rhythm" + juce::String(r) + "_note"))
                    noteParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());
                if (auto* stepsParam = params.getParameter("rhythm" + juce::String(r) + "_steps"))
                    stepsParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());
                if (auto* pulsesParam = params.getParameter("rhythm" + juce::String(r) + "_pulses"))
                    pulsesParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());
                if (auto* swingParam = params.getParameter("rhythm" + juce::String(r) + "_swing"))
                    swingParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());
                if (auto* microParam = params.getParameter("rhythm" + juce::String(r) + "_microtiming"))
                    microParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
                if (auto* velParam = params.getParameter("rhythm" + juce::String(r) + "_velocityAccent"))
                    velParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());
                if (auto* lenParam = params.getParameter("rhythm" + juce::String(r) + "_noteLength"))
                    lenParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());
            }
        };

    // ================= HEADER LABELS =================
    const char* labelsText[14] =
    {
        "Active", "Note", "Steps", "Pulses", "Swing", "Micro", "Velocity", "NoteLen",
        "ARP On", "ARP Notes", "Mode", "ARP Rate", "MIDI Port", "MIDI Ch"
    };
    for (int i = 0; i < 14; ++i)
    {
        addAndMakeVisible(headerLabels[i]);
        headerLabels[i].setText(labelsText[i], juce::dontSendNotification);
        headerLabels[i].setJustificationType(juce::Justification::centred);
    }

    // ================= RHYTHM ROWS =================
    for (int i = 0; i < 6; ++i)
    {
        auto& row = rhythmRows[i];

        // ===== ROW LABEL (R1–R6) =====
        addAndMakeVisible(row.rowLabel);
        row.rowLabel.setText("R" + juce::String(i + 1), juce::dontSendNotification);
        row.rowLabel.setJustificationType(juce::Justification::centred);
        row.rowLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // ===== ACTIVE BUTTON =====
        addAndMakeVisible(row.activeButton);
        row.activeButton.setClickingTogglesState(true);

        rhythmAttachments[i].active =
            std::make_unique<ButtonAttachment>(
                audioProcessor.parameters,
                "rhythm" + juce::String(i) + "_active",
                row.activeButton);

        // ===== MUTE / SOLO / RESET =====
        addAndMakeVisible(row.muteButton);
        addAndMakeVisible(row.soloButton);
        addAndMakeVisible(row.resetButton);

        rhythmAttachments[i].mute =
            std::make_unique<ButtonAttachment>(
                audioProcessor.parameters,
                "rhythm" + juce::String(i) + "_mute",
                row.muteButton);

        rhythmAttachments[i].solo =
            std::make_unique<ButtonAttachment>(
                audioProcessor.parameters,
                "rhythm" + juce::String(i) + "_solo",
                row.soloButton);

        rhythmAttachments[i].reset =
            std::make_unique<ButtonAttachment>(
                audioProcessor.parameters,
                "rhythm" + juce::String(i) + "_reset",
                row.resetButton);

        // ===== BASS BUTTON (ONLY R6) =====
        if (i == 5)
        {
            addAndMakeVisible(bassSettingsButton);
            bassSettingsButton.onClick = [this]()
                {
                    auto* comp = new BassSettingsComponent(audioProcessor.parameters);

                    // >>> MANDATORY DIMENSIONS <<<
                    comp->setSize(320, 260);

                    bassPopup.reset(new juce::CallOutBox(
                        *comp,
                        bassSettingsButton.getScreenBounds(),
                        nullptr));

                    bassPopup->setAlwaysOnTop(true);
                    bassPopup->enterModalState(true);
                };
        }

        // Setup rotary knobs
        auto setupRotary = [](juce::Slider& s)
            {
                s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 42, 16);
                s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange);
                s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
                s.setColour(juce::Slider::thumbColourId, juce::Colours::white);
                s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
                s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            };

        setupRotary(row.stepsKnob);
        setupRotary(row.pulsesKnob);
        setupRotary(row.swingKnob);
        setupRotary(row.microKnob);
        setupRotary(row.velocityKnob);
        setupRotary(row.noteLenKnob);

        addAndMakeVisible(row.noteSourceButton);
        row.noteSourceButton.setButtonText("Note");
        row.noteSourceButton.onClick = [this, i]()
            {
                juce::PopupMenu menu;

                juce::PopupMenu singleMenu;
                static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

                int id = 100;
                for (int octave = 0; octave <= 8; ++octave)
                {
                    for (int n = 0; n < 12; ++n)
                    {
                        int midiNote = octave * 12 + n;
                        if (midiNote > 127) continue;

                        singleMenu.addItem(
                            id++,
                            juce::String(noteNames[n]) + juce::String(octave));
                    }
                }

                juce::PopupMenu scaleMenu;
                scaleMenu.addItem(200, "Major");
                scaleMenu.addItem(201, "Minor");
                scaleMenu.addItem(202, "Dorian");
                scaleMenu.addItem(203, "Phrygian");
                scaleMenu.addItem(204, "Lydian");
                scaleMenu.addItem(205, "Mixolydian");

                juce::PopupMenu chordMenu;
                chordMenu.addItem(300, "Major");
                chordMenu.addItem(301, "Minor");
                chordMenu.addItem(302, "Diminished");
                chordMenu.addItem(303, "Augmented");
                chordMenu.addItem(304, "Major 7");
                chordMenu.addItem(305, "Minor 7");

                menu.addSubMenu("Single Note", singleMenu);
                menu.addSubMenu("Scale", scaleMenu);
                menu.addSubMenu("Chord", chordMenu);

                menu.showMenuAsync(juce::PopupMenu::Options(),
                    [this, i](int result)
                    {
                        if (result <= 0) return;

                        auto& src = audioProcessor.noteSources[i];

                        // ===== SINGLE NOTE =====
                        if (result >= 100 && result < 200)
                        {
                            src.type = Euclidean_seqAudioProcessor::NoteSourceType::Single;
                            src.value = result - 100;
                            rhythmRows[i].noteSourceButton.setButtonText("Note");
                            audioProcessor.updateRowInputNotes(i);
                        }
                        // ===== SCALE =====
                        else if (result >= 200 && result < 300)
                        {
                            src.type = Euclidean_seqAudioProcessor::NoteSourceType::Scale;
                            src.value = result - 200;
                            rhythmRows[i].noteSourceButton.setButtonText("Scale");
                            audioProcessor.updateRowInputNotes(i);
                        }
                        // ===== CHORD =====
                        else if (result >= 300)
                        {
                            src.type = Euclidean_seqAudioProcessor::NoteSourceType::Chord;
                            src.value = result - 300;
                            rhythmRows[i].noteSourceButton.setButtonText("Chord");
                            audioProcessor.updateRowInputNotes(i);
                        }
                    });
            };

        addAndMakeVisible(row.stepsKnob);
        addAndMakeVisible(row.pulsesKnob);
        addAndMakeVisible(row.swingKnob);
        addAndMakeVisible(row.microKnob);
        addAndMakeVisible(row.velocityKnob);
        addAndMakeVisible(row.noteLenKnob);

        // ARP Buttons
        addAndMakeVisible(row.arpActive);

        addAndMakeVisible(row.arpNotesButton);
        row.arpNotesButton.setButtonText("Notes");
        row.arpNotesButton.onClick = [this, i]()
            {
                auto& arpSlots = audioProcessor.midiGen.arpNotes[i]; // <- unique definition

                auto inputNotes = audioProcessor.midiGen.getArpInputNotes(i);

                juce::PopupMenu menu;

                // Show full popup 0–127
                for (int note = 0; note <= 127; ++note)
                {
                    bool selected = false;
                    for (size_t s = 0; s < arpSlots.size(); ++s)
                        if (arpSlots[s] == note)
                            selected = true;

                    menu.addItem(1 + note,
                        juce::MidiMessage::getMidiNoteName(note, true, true, 3),
                        true,
                        selected);
                }

                // Shows received notes
                if (inputNotes.empty())
                {
                    menu.addItem(1, "No notes received yet", false);
                }
                else
                {
                    for (int midiNote : inputNotes)
                    {
                        bool selected = false;
                        for (size_t s = 0; s < arpSlots.size(); ++s)
                            if (arpSlots[s] == midiNote)
                                selected = true;

                        menu.addItem(1 + midiNote,
                            juce::MidiMessage::getMidiNoteName(midiNote, true, true, 3),
                            true,
                            selected);
                    }
                }

                menu.showMenuAsync(juce::PopupMenu::Options(),
                    [this, &arpSlots](int result)
                    {
                        if (result <= 0) return;

                        int midiNote = result - 1;

                        // Toggle
                        for (size_t s = 0; s < arpSlots.size(); ++s)
                        {
                            if (arpSlots[s] == midiNote)
                            {
                                arpSlots[s] = -1;
                                return;
                            }
                        }

                        // Add if free slot
                        for (size_t s = 0; s < arpSlots.size(); ++s)
                        {
                            if (arpSlots[s] < 0)
                            {
                                arpSlots[s] = midiNote;
                                return;
                            }
                        }
                    });
            };

        addAndMakeVisible(row.arpMode);
        row.arpMode.addItem("UP", 1);
        row.arpMode.addItem("DOWN", 2);
        row.arpMode.addItem("UP_DOWN", 3);
        row.arpMode.addItem("RANDOM", 4);

        addAndMakeVisible(row.arpRateBox);
        row.arpRateBox.addItemList({ "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16D" }, 1);

        // ===== MIDI PORT =====
        addAndMakeVisible(row.midiPortBox);
        addAndMakeVisible(row.midiChannelBox);

        // ===== MIDI CHANNEL (1–16) =====
        row.midiChannelBox.clear();
        for (int ch = 1; ch <= 16; ++ch)
            row.midiChannelBox.addItem("Ch " + juce::String(ch), ch);

        row.midiChannelBox.setSelectedId(1, juce::dontSendNotification);

        rhythmAttachments[i].midiPort =
            std::make_unique<ComboBoxAttachment>(
                audioProcessor.parameters,
                "rhythm" + juce::String(i) + "_midiPort",
                row.midiPortBox);

        rhythmAttachments[i].midiChannel =
            std::make_unique<ComboBoxAttachment>(
                audioProcessor.parameters,
                "rhythm" + juce::String(i) + "_midiChannel",
                row.midiChannelBox);
    }

    // ===== HOT-PLUG MIDI GUI =====
    auto refreshMidiBox = [this](juce::ComboBox& box, const juce::Array<juce::MidiDeviceInfo>& midiDevices, std::atomic<float>* param)
        {
            int currentId = box.getSelectedId();
            box.clear();

            if (midiDevices.isEmpty())
            {
                box.addItem("No MIDI Outputs", 1);
                box.setSelectedId(1, juce::dontSendNotification);
                return;
            }

            for (int i = 0; i < midiDevices.size(); ++i)
                box.addItem(midiDevices[i].name, i + 1);

            if (param)
            {
                int deviceIndex = (int)param->load();
                if (deviceIndex >= 0 && deviceIndex < midiDevices.size())
                    box.setSelectedId(deviceIndex + 1, juce::dontSendNotification);
                else
                    box.setSelectedId(1, juce::dontSendNotification);
            }
            else
            {
                int deviceIndex = currentId - 1;
                if (deviceIndex >= 0 && deviceIndex < midiDevices.size())
                    box.setSelectedId(deviceIndex + 1, juce::dontSendNotification);
                else
                    box.setSelectedId(1, juce::dontSendNotification);
            }
        };

    audioProcessor.refreshMidiOutputs();
    const auto& midiDevices = audioProcessor.getAvailableMidiOutputs();
    for (int r = 0; r < 6; ++r)
    {
        auto* param = audioProcessor.parameters.getRawParameterValue("rhythm" + juce::String(r) + "_midiPort");
        refreshMidiBox(rhythmRows[r].midiPortBox, midiDevices, param);
    }

    clockBPMAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.parameters, "clockBPM", clockBpmSlider);

    startTimerHz(60);

    addAndMakeVisible(versionLabel);
    versionLabel.setText(ProjectInfo::versionString, juce::dontSendNotification);
    versionLabel.setJustificationType(juce::Justification::centredRight);
    versionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    versionLabel.setColour(juce::Label::textColourId, juce::Colours::white);
}

// Update the ARP GUI
void Euclidean_seqAudioProcessorEditor::updateArpGui()
{
    for (int row = 0; row < 6; ++row)
    {
        const auto& notes = audioProcessor.midiGen.arpNotes[row];
        DBG("Ritmo " << row << " ARP Notes: ");
        for (auto note : notes)
            DBG(" " << note);
    }
    repaint();
}

Euclidean_seqAudioProcessorEditor::~Euclidean_seqAudioProcessorEditor() {}

void Euclidean_seqAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    g.addTransform(juce::AffineTransform::scale(guiScale));
}

void Euclidean_seqAudioProcessorEditor::resized()
{
    // ===== AUTO SCALE CALCULATION =====
    const float scaleX = (float)getWidth() / (float)baseWidth;
    const float scaleY = (float)getHeight() / (float)baseHeight;
    guiScale = juce::jmin(scaleX, scaleY);
    repaint();

    const int headerHeight = 40;
    const int rowHeight = 90;
    const int knobSize = 100;

    // ================= COLUMN DEFINITIONS =================
    const int startX = 40;
    const int colSpacing = 85;

    enum Column
    {
        COL_ACTIVE = 0,
        COL_NOTE,
        COL_STEPS,
        COL_PULSES,
        COL_SWING,
        COL_MICRO,
        COL_VELOCITY,
        COL_NOTELEN,
        COL_ARP_ON,
        COL_ARP_NOTES,
        COL_ARP_MODE,
        COL_ARP_RATE,
        COL_MIDI_PORT,
        COL_MIDI_CH,
        NUM_COLUMNS
    };

    int columnX[NUM_COLUMNS];
    for (int c = 0; c < NUM_COLUMNS; ++c)
        columnX[c] = startX + c * colSpacing;

    // ===== HEADER =====
    for (int i = 0; i < NUM_COLUMNS; ++i)
    {
        headerLabels[i].setBounds(
            columnX[i] - 20,
            0,
            colSpacing,
            headerHeight);
    }

    // ================= TRANSPORT =================
    const int bottomMargin = 50;
    const int buttonHeight = 30;

    clockSettingsButton.setBounds(getWidth() - 180, getHeight() - bottomMargin, 170, buttonHeight);
    clockBpmSlider.setBounds(getWidth() - 360, getHeight() - bottomMargin, 170, buttonHeight);
    playButton.setBounds((getWidth() - 90) / 2, getHeight() - bottomMargin, 90, buttonHeight);

    // ================= RHYTHM ROWS =================
    const int rowGapPx = juce::roundToInt(5.0f * mmToPx * getDesktopScaleFactor());

    for (int r = 0; r < 6; ++r)
    {
        auto& row = rhythmRows[r];

        int y = headerHeight + r * rowHeight;
        if (r > 0)
            y += r * rowGapPx;

        // Row label
        row.rowLabel.setBounds(columnX[COL_ACTIVE] - 35, y, 30, 20);

        // Active Button
        row.activeButton.setBounds(columnX[COL_ACTIVE] - 2, y+10, 30, 30);
        row.activeButton.setClickingTogglesState(true);
        // M/S/R sotto Active
        const int subY = y + 45;
        row.muteButton.setBounds(columnX[COL_ACTIVE] - 6, subY, 24, 24);
        row.soloButton.setBounds(columnX[COL_ACTIVE] + 22, subY, 24, 24);
        row.resetButton.setBounds(columnX[COL_ACTIVE] + 50, subY, 24, 24);

        if (r == 5)
            bassSettingsButton.setBounds(columnX[COL_ACTIVE] - 6, subY + 30, 80, 26);

        // Note source
        row.noteSourceButton.setBounds(columnX[COL_NOTE], y, 80, 80);

        // Knobs
        row.stepsKnob.setBounds(columnX[COL_STEPS], y, knobSize, knobSize);
        row.pulsesKnob.setBounds(columnX[COL_PULSES], y, knobSize, knobSize);
        row.swingKnob.setBounds(columnX[COL_SWING], y, knobSize, knobSize);
        row.microKnob.setBounds(columnX[COL_MICRO], y, knobSize, knobSize);
        row.velocityKnob.setBounds(columnX[COL_VELOCITY], y, knobSize, knobSize);
        row.noteLenKnob.setBounds(columnX[COL_NOTELEN], y, knobSize, knobSize);

        // ARP
        row.arpActive.setBounds(columnX[COL_ARP_ON] - 2, y, 26, 26);
        row.arpNotesButton.setBounds(columnX[COL_ARP_NOTES], y, 80, 25);
        row.arpMode.setBounds(columnX[COL_ARP_MODE], y, 80, 25);
        row.arpRateBox.setBounds(columnX[COL_ARP_RATE], y, 80, 25);

		// MIDI OUTPUT
        row.midiPortBox.setBounds(columnX[COL_MIDI_PORT], y, 90, 25);
        row.midiChannelBox.setBounds(columnX[COL_MIDI_CH] + 10, y, 60, 25);
    }

    versionLabel.setBounds(getWidth() - 120, 5, 110, 20);
}

void Euclidean_seqAudioProcessorEditor::openClockSettingsPopup()
{
    auto* dialog = new ClockSettingsDialog([this](int newSource)
        {
            if (auto* param = audioProcessor.parameters.getParameter("clockSource"))
                param->setValueNotifyingHost(newSource - 1);
        });

    dialog->setSize(220, 100);
    clockPopup.reset(new juce::CallOutBox(*dialog, clockSettingsButton.getScreenBounds(), nullptr));
    clockPopup->setAlwaysOnTop(true);
    clockPopup->enterModalState(true);
}

void Euclidean_seqAudioProcessorEditor::timerCallback()
{
    audioProcessor.refreshMidiOutputs();
    const auto& midiDevices = audioProcessor.getAvailableMidiOutputs();
    for (int r = 0; r < 6; ++r)
    {
        auto& box = rhythmRows[r].midiPortBox;
        int currentId = box.getSelectedId();
        box.clear();
        for (int i = 0; i < midiDevices.size(); ++i)
            box.addItem(midiDevices[i].name, i + 1);
        if (midiDevices.isEmpty())
            box.addItem("No MIDI Outputs", 1);

        int deviceIndex = currentId - 1;
        if (deviceIndex >= 0 && deviceIndex < midiDevices.size())
            box.setSelectedId(deviceIndex + 1, juce::dontSendNotification);
        else
            box.setSelectedId(0, juce::dontSendNotification);
    }

    updateArpGui();
    repaint();
}








