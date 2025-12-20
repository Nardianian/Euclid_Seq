/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
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
    const char* labelsText[16] =
    {
        "Active", "Note", "Steps", "Pulses", "Swing", "Micro", "Velocity", "NoteLen",
        "ARP On", "Mode", "ARP Rate", "ARP Notes", "MIDI Port", "Port", "MIDI Ch", "Ch"
    };
    for (int i = 0; i < 16; ++i)
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
                s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 38, 15);
            };

        addAndMakeVisible(row.activeButton);

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
        addAndMakeVisible(row.arpRateKnob);
        addAndMakeVisible(row.arpNotesKnob);

        // MIDI
        addAndMakeVisible(row.midiPortLabel);
        addAndMakeVisible(row.midiPortBox);
        addAndMakeVisible(row.midiChannelLabel);
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
            rhythmRows[r].arpRateKnob.setTooltip("ARP Rate multiplier");
            rhythmRows[r].arpNotesKnob.setTooltip("Number of ARP notes");
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

        att.arpRate = std::make_unique<SliderAttachment>(
            params, "rhythm" + juce::String(i) + "_arpRate", row.arpRateKnob);

        att.arpNotes = std::make_unique<SliderAttachment>(
            params, "rhythm" + juce::String(i) + "_arpNotes", row.arpNotesKnob);
    }

    clockSourceAttachment = std::make_unique<ComboBoxAttachment>(
        audioProcessor.parameters, "clockSource", clockBpmBox);

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

    // HEADER
    int hx = 0;
    for (int i = 0; i < 16; ++i)
    {
        headerLabels[i].setBounds(hx, 0, 60, headerHeight);
        hx += 65;
    }

    // COSTANTI
    const int knobSize = 38;
    const int valueHeight = 15;

    clockSettingsButton.setBounds(getWidth() - 160, 10, 150, 30);
    clockBpmSlider.setBounds(getWidth() - 320, 10, 150, 30);

    // OFFSET ORIZZONTALI (pixel)
    const int dx[] =
    {
        0,      // active
        19,     // note
        38,     // steps
        57,     // pulses
        76,     // swing
        95,     // micro
        114,    // velocity
        133,    // notelen
        152,    // arp on
        171,    // arp mode
        140,    // arp rate
        166,    // arp notes
        209,    // midi port
        247     // midi channel
    };

    for (int r = 0; r < 6; ++r)
    {
        int y = headerHeight + r * rowHeight;
        int baseX = 10;
        auto& row = rhythmRows[r];

        row.activeButton.setBounds(baseX, y, 30, 30);

        row.noteKnob.setBounds(baseX + dx[1], y, knobSize, knobSize);
        row.stepsKnob.setBounds(baseX + dx[2], y, knobSize, knobSize);
        row.pulsesKnob.setBounds(baseX + dx[3], y, knobSize, knobSize);
        row.swingKnob.setBounds(baseX + dx[4], y, knobSize, knobSize);
        row.microKnob.setBounds(baseX + dx[5], y, knobSize, knobSize);
        row.velocityKnob.setBounds(baseX + dx[6], y, knobSize, knobSize);
        row.noteLenKnob.setBounds(baseX + dx[7], y, knobSize, knobSize);

        row.arpActive.setBounds(baseX + dx[8], y, 30, 30);
        row.arpMode.setBounds(baseX + dx[9], y, 60, 25);

        row.arpRateKnob.setBounds(baseX + dx[10], y, knobSize, knobSize);
        row.arpNotesKnob.setBounds(baseX + dx[11], y, knobSize, knobSize);

        row.midiPortBox.setBounds(baseX + dx[12], y, 80, 25);
        row.midiChannelBox.setBounds(baseX + dx[13], y, 60, 25);
    }
}

void Euclidean_seqAudioProcessorEditor::timerCallback()
{
    repaint();
}

