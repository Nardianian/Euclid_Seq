/*
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MidiGenerator.h"
#include "Arp.h"

// Popup window for Global Clock settings
class ClockSettingsDialog : public juce::Component
{
public:
    ClockSettingsDialog(std::function<void(int)> onChangeCallback)
        : onChange(onChangeCallback)
    {
        addAndMakeVisible(clockSourceBox);
        clockSourceBox.addItem("DAW Clock", 1);
        clockSourceBox.addItem("Internal Clock", 2);
        clockSourceBox.addItem("External Clock", 3);
        clockSourceBox.onChange = [this]()
            {
                onChange(clockSourceBox.getSelectedId());
            };
        clockSourceBox.setSelectedId(1);
    }

    void resized() override
    {
        clockSourceBox.setBounds(20, 20, getWidth() - 40, 30);
    }

private:
    juce::ComboBox clockSourceBox;
    std::function<void(int)> onChange;
};

//==============================================================================

class Euclidean_seqAudioProcessorEditor :
    public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    Euclidean_seqAudioProcessorEditor(Euclidean_seqAudioProcessor&);
    ~Euclidean_seqAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void openClockSettingsPopup();  // or move to private

private:
    Euclidean_seqAudioProcessor& audioProcessor;

    juce::TextButton randomizeButton{ "Randomize" };    // or remove the staples with the content
    juce::TextButton clockSettingsButton{ "Clock Settings" };
    std::unique_ptr<juce::CallOutBox> clockPopup;
    juce::TooltipWindow tooltipWindow;

    // Array di controlli per ogni rhythm row
    struct RhythmRowControls
    {
        juce::ToggleButton activeButton;
        juce::Slider noteKnob;
        juce::Slider stepsKnob;
        juce::Slider pulsesKnob;
        juce::Slider swingKnob;
        juce::Slider microKnob;
        juce::Slider velocityKnob;
        juce::Slider noteLenKnob;
        juce::Label rowLabel;


        // ARP Controls
        juce::ToggleButton arpActive;
        juce::ComboBox arpMode;
        juce::ComboBox arpRateBox;
        juce::TextButton arpNotesButton;

        // MIDI output
        juce::Label midiPortLabel;
        juce::ComboBox midiPortBox;
        juce::Label midiChannelLabel;
        juce::ComboBox midiChannelBox;
    };

    RhythmRowControls rhythmRows[6];

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct RhythmRowAttachments
    {
        std::unique_ptr<ComboBoxAttachment> midiPort;
        std::unique_ptr<ComboBoxAttachment> midiChannel;

        std::unique_ptr<ButtonAttachment> active;
        std::unique_ptr<SliderAttachment> note;
        std::unique_ptr<SliderAttachment> steps;
        std::unique_ptr<SliderAttachment> pulses;
        std::unique_ptr<SliderAttachment> swing;
        std::unique_ptr<SliderAttachment> micro;
        std::unique_ptr<SliderAttachment> velocity;
        std::unique_ptr<SliderAttachment> noteLen;

        std::unique_ptr<ButtonAttachment> arpActive;
        std::unique_ptr<ComboBoxAttachment> arpMode;
        std::unique_ptr<ComboBoxAttachment> arpRate;
    };

    RhythmRowAttachments rhythmAttachments[6];

    std::unique_ptr<ComboBoxAttachment> clockSourceAttachment;
    std::unique_ptr<SliderAttachment> clockBPMAttachment;

    juce::Label headerLabels[16];
    juce::Slider clockBpmSlider;
    juce::ComboBox clockBpmBox;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Euclidean_seqAudioProcessorEditor)
};





