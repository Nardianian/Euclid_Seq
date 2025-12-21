/*
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

Euclidean_seqAudioProcessorEditor::Euclidean_seqAudioProcessorEditor(Euclidean_seqAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1350, 765);

    addAndMakeVisible(clockSettingsButton);
    clockSettingsButton.onClick = [this]()
        {
            openClockSettingsPopup();
        };

    // ===================== RANDOMIZE BUTTON =====================
    addAndMakeVisible(randomizeButton);
    randomizeButton.setButtonText("Randomize");
    randomizeButton.onClick = [this]()
        {
            for (int r = 0; r < 6; ++r)
            {
                auto& row = rhythmRows[r];
                auto& params = audioProcessor.parameters;

                // Random note
                if (auto* noteParam = params.getParameter("rhythm" + juce::String(r) + "_note"))
                    noteParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());

                // Random steps
                if (auto* stepsParam = params.getParameter("rhythm" + juce::String(r) + "_steps"))
                    stepsParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());

                // Random pulses
                if (auto* pulsesParam = params.getParameter("rhythm" + juce::String(r) + "_pulses"))
                    pulsesParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());

                // Random swing
                if (auto* swingParam = params.getParameter("rhythm" + juce::String(r) + "_swing"))
                    swingParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());

                // Random microtiming
                if (auto* microParam = params.getParameter("rhythm" + juce::String(r) + "_microtiming"))
                    microParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f); // [-1,1]

                // Random velocity
                if (auto* velParam = params.getParameter("rhythm" + juce::String(r) + "_velocityAccent"))
                    velParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());

                // Random note length
                if (auto* lenParam = params.getParameter("rhythm" + juce::String(r) + "_noteLength"))
                    lenParam->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());
            }
        };

    //==================== CLOCK BPM SLIDER ====================
    addAndMakeVisible(clockBpmSlider);
    clockBpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    clockBpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);

    // Header labels
    const char* labelsText[14] =
    {
        "Active", "Note", "Steps", "Pulses", "Swing", "Micro", "Velocity", "NoteLen",
        "ARP On", "Mode", "ARP Rate", "ARP Notes", "MIDI Port", "MIDI Ch"
    };
    for (int i = 0; i < 14; ++i)
    {
        addAndMakeVisible(headerLabels[i]);
        headerLabels[i].setText(labelsText[i], juce::dontSendNotification);
        headerLabels[i].setJustificationType(juce::Justification::centred);
    }

    // Rhythm rows
    for (int i = 0; i < 6; ++i)
    {
        auto& row = rhythmRows[i];

        auto setupRotary = [](juce::Slider& s)
            {
                s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 42, 16);
                // Garantisce visibilità su Windows 11 / LookAndFeel V4
                s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange);
                s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::black);
                s.setColour(juce::Slider::thumbColourId, juce::Colours::white);
                s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
                s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            };

        addAndMakeVisible(row.activeButton);

        row.rowLabel.setText("R" + juce::String(i + 1), juce::dontSendNotification);
        row.rowLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(row.rowLabel);

        setupRotary(row.noteKnob);
        setupRotary(row.stepsKnob);
        setupRotary(row.pulsesKnob);
        setupRotary(row.swingKnob);
        setupRotary(row.microKnob);
        setupRotary(row.velocityKnob);
        setupRotary(row.noteLenKnob);

        addAndMakeVisible(row.noteKnob);
        addAndMakeVisible(row.stepsKnob);
        addAndMakeVisible(row.pulsesKnob);
        addAndMakeVisible(row.swingKnob);
        addAndMakeVisible(row.microKnob);
        addAndMakeVisible(row.velocityKnob);
        addAndMakeVisible(row.noteLenKnob);

        // ARP
        addAndMakeVisible(row.arpActive);
        row.arpMode.addItem("UP", 1);
        row.arpMode.addItem("DOWN", 2);
        row.arpMode.addItem("UP_DOWN", 3);
        row.arpMode.addItem("RANDOM", 4);
        addAndMakeVisible(row.arpMode);
        row.arpRateBox.addItemList(
            { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16D" }, 1);
        addAndMakeVisible(row.arpRateBox);

        addAndMakeVisible(row.arpNotesButton);
        row.arpNotesButton.setButtonText("Notes");
        row.arpNotesButton.onClick = [this, i]()
            {
                juce::PopupMenu menu;

                // Note realmente ricevute dal ritmo
                auto inputNotes = audioProcessor.midiGen.getArpInputNotes(i);
                auto& slots = audioProcessor.midiGen.arpNoteSlots[i];

                if (inputNotes.empty())
                {
                    menu.addItem(1, "No notes received yet", false);
                }
                else
                {
                    for (int midiNote : inputNotes)
                    {
                        bool selected = false;

                        for (int s = 0; s < 7; ++s)
                        {
                            if (slots[s].load() == midiNote)
                            {
                                selected = true;
                                break;
                            }
                        }

                        menu.addItem(
                            1 + midiNote,
                            juce::MidiMessage::getMidiNoteName(midiNote, true, true, 3),
                            true,
                            selected);
                    }
                }

                menu.showMenuAsync(
                    juce::PopupMenu::Options(),
                    [this, i](int result)
                    {
                        if (result <= 0)
                            return;

                        int midiNote = result - 1;
                        auto& slots = audioProcessor.midiGen.arpNoteSlots[i];

                        // Toggle select / deselect
                        for (int s = 0; s < 7; ++s)
                        {
                            if (slots[s].load() == midiNote)
                            {
                                slots[s].store(-1);
                                return;
                            }
                        }

                        // Add if free slot exists
                        for (int s = 0; s < 7; ++s)
                        {
                            if (slots[s].load() < 0)
                            {
                                slots[s].store(midiNote);
                                return;
                            }
                        }
                    });
            };

        // MIDI
        addAndMakeVisible(row.midiPortLabel);
        row.midiPortLabel.setText("Port", juce::dontSendNotification);
        addAndMakeVisible(row.midiPortBox);
        addAndMakeVisible(row.midiChannelLabel);
        row.midiChannelLabel.setText("Ch", juce::dontSendNotification);
        addAndMakeVisible(row.midiChannelBox);

        clockSettingsButton.setTooltip("Open Clock Settings");
        clockBpmSlider.setTooltip("Set Internal Clock BPM");
        for (int r = 0; r < 6; ++r)
        {
            rhythmRows[r].noteKnob.setTooltip("Note pitch for this rhythm row");
            rhythmRows[r].stepsKnob.setTooltip("Number of steps");
            rhythmRows[r].pulsesKnob.setTooltip("Number of pulses in pattern");
            rhythmRows[r].swingKnob.setTooltip("Swing amount");
            rhythmRows[r].microKnob.setTooltip("Microtiming offset");
            rhythmRows[r].velocityKnob.setTooltip("Velocity / Accent");
            rhythmRows[r].noteLenKnob.setTooltip("Note length");
            rhythmRows[r].arpActive.setTooltip("Enable arpeggiator");
            rhythmRows[r].arpMode.setTooltip("ARP Mode (UP, DOWN, RANDOM, etc.)");
            rhythmRows[r].arpRateBox.setTooltip("ARP musical rate (1/4, 1/8T, 1/16D, etc.)");
            rhythmRows[r].arpNotesButton.setTooltip("Number of ARP notes");
            rhythmRows[r].midiPortBox.setTooltip("Select MIDI Output Port");
            rhythmRows[r].midiChannelBox.setTooltip("Select MIDI Channel");
        }

        auto& att = rhythmAttachments[i];
        auto& params = audioProcessor.parameters;

        // MIDI Port
        att.midiPort = std::make_unique<ComboBoxAttachment>(
            params, "rhythm" + juce::String(i) + "_midiPort", row.midiPortBox);

        // MIDI Channel
        att.midiChannel = std::make_unique<ComboBoxAttachment>(
            params, "rhythm" + juce::String(i) + "_midiChannel", row.midiChannelBox);

        att.active = std::make_unique<ButtonAttachment>(
            params, "rhythm" + juce::String(i) + "_active", row.activeButton);

        att.note = std::make_unique<SliderAttachment>(
            params, "rhythm" + juce::String(i) + "_note", row.noteKnob);

        att.steps = std::make_unique<SliderAttachment>(
            params, "rhythm" + juce::String(i) + "_steps", row.stepsKnob);

        att.pulses = std::make_unique<SliderAttachment>(
            params, "rhythm" + juce::String(i) + "_pulses", row.pulsesKnob);

        att.swing = std::make_unique<SliderAttachment>(
            params, "rhythm" + juce::String(i) + "_swing", row.swingKnob);

        att.micro = std::make_unique<SliderAttachment>(
            params, "rhythm" + juce::String(i) + "_microtiming", row.microKnob);

        att.velocity = std::make_unique<SliderAttachment>(
            params, "rhythm" + juce::String(i) + "_velocityAccent", row.velocityKnob);

        att.noteLen = std::make_unique<SliderAttachment>(
            params, "rhythm" + juce::String(i) + "_noteLength", row.noteLenKnob);

        att.arpActive = std::make_unique<ButtonAttachment>(
            params, "rhythm" + juce::String(i) + "_arpActive", row.arpActive);

        att.arpMode = std::make_unique<ComboBoxAttachment>(
            params, "rhythm" + juce::String(i) + "_arpMode", row.arpMode);

        att.arpRate = std::make_unique<ComboBoxAttachment>(
            params, "rhythm" + juce::String(i) + "_arpRate", row.arpRateBox);
    }

    clockBPMAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.parameters, "clockBPM", clockBpmSlider);

    startTimerHz(60);
}

Euclidean_seqAudioProcessorEditor::~Euclidean_seqAudioProcessorEditor() {}

void Euclidean_seqAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void Euclidean_seqAudioProcessorEditor::openClockSettingsPopup()
{
    auto* dialog = new ClockSettingsDialog([this](int newSource)
        {
            if (auto* param = audioProcessor.parameters.getParameter("clockSource"))
                param->setValueNotifyingHost(newSource - 1); // ComboBox ID 1 = DAW = value 0
        });

    dialog->setSize(200, 80);
    clockPopup.reset(new juce::CallOutBox(*dialog, getLocalBounds(), nullptr));
    clockPopup->setAlwaysOnTop(true);
    clockPopup->enterModalState(true);
}

void Euclidean_seqAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    const int headerHeight = 40;
    const int rowHeight = 90;
    const int knobSize = 48;
    const int valueHeight = 15;

    // ================= COLUMN DEFINITIONS =================
    const int startX = 40;   // margine sinistro
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

    // ================= HEADER =================
    for (int i = 0; i < NUM_COLUMNS; ++i)
    {
        headerLabels[i].setBounds(
            columnX[i] - 20,
            0,
            colSpacing,
            headerHeight);
    }

    // ================= CLOCK CONTROLS =================
    clockSettingsButton.setBounds(getWidth() - 180, getHeight() - 50, 170, 30);
    clockBpmSlider.setBounds(getWidth() - 360, getHeight() - 50, 170, 30);

    // ================= RHYTHM ROWS =================
    for (int r = 0; r < 6; ++r)
    {
        auto& row = rhythmRows[r];
        int y = headerHeight + r * rowHeight;

        // Row label R1–R6
        row.rowLabel.setBounds(columnX[COL_ACTIVE] - 30, y + 6, 25, 20);

        // Active
        row.activeButton.setBounds(columnX[COL_ACTIVE] - 2, y, 26, 26);

        // Rotary knobs + value boxes
        row.noteKnob.setBounds(columnX[COL_NOTE], y, knobSize, knobSize);
        row.stepsKnob.setBounds(columnX[COL_STEPS], y, knobSize, knobSize);
        row.pulsesKnob.setBounds(columnX[COL_PULSES], y, knobSize, knobSize);
        row.swingKnob.setBounds(columnX[COL_SWING], y, knobSize, knobSize);
        row.microKnob.setBounds(columnX[COL_MICRO], y, knobSize, knobSize);
        row.velocityKnob.setBounds(columnX[COL_VELOCITY], y, knobSize, knobSize);
        row.noteLenKnob.setBounds(columnX[COL_NOTELEN], y, knobSize, knobSize);

        // Arp On
        row.arpActive.setBounds(columnX[COL_ARP_ON] - 2, y, 26, 26);

        // Arp Notes
        row.arpNotesButton.setBounds(columnX[COL_ARP_NOTES], y, 80, 25);

        // Arp Mode
        row.arpMode.setBounds(columnX[COL_ARP_MODE], y, 80, 25);

        // Arp Rate
        row.arpRateBox.setBounds(columnX[COL_ARP_RATE], y, 80, 25);

        // MIDI
        row.midiPortBox.setBounds(columnX[COL_MIDI_PORT], y, 80, 25);
        row.midiChannelBox.setBounds(columnX[COL_MIDI_CH], y, 60, 25);
    }
}

void Euclidean_seqAudioProcessorEditor::timerCallback()
{
    repaint();
}

