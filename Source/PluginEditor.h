#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MidiGenerator.h"
#include "Arp.h"

// Settings pop-up window Global Clock
class ClockSettingsDialog : public juce::Component
{
public:
    ClockSettingsDialog(std::function<void(int)> onChangeCallback)
        : onChange(onChangeCallback)
    {
        addAndMakeVisible(clockSourceBox);
#if JUCE_STANDALONE
        // ===== STANDALONE =====
        clockSourceBox.addItem("Internal Clock", 1);
        clockSourceBox.addItem("External Clock", 2);
        clockSourceBox.setSelectedId(1);
#else
        // ===== PLUGIN =====
        clockSourceBox.addItem("DAW Clock", 1);
        clockSourceBox.addItem("Internal Clock", 2);
        clockSourceBox.addItem("External Clock", 3);
        clockSourceBox.setSelectedId(1);
#endif

        clockSourceBox.onChange = [this]()
            {
                onChange(clockSourceBox.getSelectedId());
            };
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
// Bass (R6) Settings Popup Component
// =============================================================================
class BassSettingsComponent : public juce::Component
{
public:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    BassSettingsComponent(juce::AudioProcessorValueTreeState& state)
        : parameters(state)
    {
        // ===== Glide Time =====
        addAndMakeVisible(glideTimeLabel);
        glideTimeLabel.setText("Glide Time", juce::dontSendNotification);

        addAndMakeVisible(glideTimeSlider);
        glideTimeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        glideTimeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
        glideTimeAttachment = std::make_unique<SliderAttachment>(
            parameters, "bassGlideTime", glideTimeSlider);

        // ===== Glide Curve =====
        addAndMakeVisible(glideCurveLabel);
        glideCurveLabel.setText("Glide Curve", juce::dontSendNotification);

        addAndMakeVisible(glideCurveSlider);
        glideCurveSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        glideCurveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
        glideCurveAttachment = std::make_unique<SliderAttachment>(
            parameters, "bassGlideCurve", glideCurveSlider);

        // ===== Pitch Bend Range =====
        addAndMakeVisible(pbRangeLabel);
        pbRangeLabel.setText("Pitch Bend Range", juce::dontSendNotification);

        addAndMakeVisible(pbRangeBox);
        pbRangeBox.addItem("±2 semitones", 1);
        pbRangeBox.addItem("±12 semitones", 2);
        pbRangeAttachment = std::make_unique<ComboBoxAttachment>(
            parameters, "bassPitchBendRange", pbRangeBox);

        // ===== Bass Mode =====
        addAndMakeVisible(bassModeToggle);
        bassModeToggle.setButtonText("Bass Mono (Legato + Glide)");
        bassModeAttachment = std::make_unique<ButtonAttachment>(
            parameters, "bassMode", bassModeToggle);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        const int rowH = 70;

        auto row1 = area.removeFromTop(rowH);
        glideTimeLabel.setBounds(row1.removeFromLeft(120));
        glideTimeSlider.setBounds(row1);

        auto row2 = area.removeFromTop(rowH);
        glideCurveLabel.setBounds(row2.removeFromLeft(120));
        glideCurveSlider.setBounds(row2);

        auto row3 = area.removeFromTop(40);
        pbRangeLabel.setBounds(row3.removeFromLeft(120));
        pbRangeBox.setBounds(row3.removeFromLeft(160));

        area.removeFromTop(10);
        bassModeToggle.setBounds(area.removeFromTop(30));
    }

private:
    juce::AudioProcessorValueTreeState& parameters;

    juce::Label glideTimeLabel;
    juce::Slider glideTimeSlider;
    std::unique_ptr<SliderAttachment> glideTimeAttachment;

    juce::Label glideCurveLabel;
    juce::Slider glideCurveSlider;
    std::unique_ptr<SliderAttachment> glideCurveAttachment;

    juce::Label pbRangeLabel;
    juce::ComboBox pbRangeBox;
    std::unique_ptr<ComboBoxAttachment> pbRangeAttachment;

    juce::ToggleButton bassModeToggle;
    std::unique_ptr<ButtonAttachment> bassModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassSettingsComponent)
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
    void openClockSettingsPopup();  // oppure spostare in private 
    void updateArpGui();
    // Stato GUI delle note selezionate in ARP Notes per ogni riga
    std::array<std::vector<juce::ToggleButton*>, 6> arpNoteButtons;

private:
    Euclidean_seqAudioProcessor& audioProcessor;

    juce::TextButton randomizeButton{ "Randomize" };    // oppure eliminare le graffe con il contenuto
    juce::TextButton clockSettingsButton{ "Clock Settings" };

    // ===== GLOBAL PLAY =====
    juce::TextButton playButton{ "PLAY" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAttachment;

    // ===== Bass (R6) Popup =====
    juce::TextButton bassSettingsButton{ "BASS" };
    std::unique_ptr<juce::CallOutBox> bassPopup;
    std::unique_ptr<juce::CallOutBox> clockPopup;

    // ===== ARP Notes Popup =====
    std::unique_ptr<juce::CallOutBox> arpNotesPopup;

    juce::TooltipWindow tooltipWindow;

    // Array of controls for each rhythm row
    struct RhythmRowControls
    {
        juce::ToggleButton activeButton;
        juce::TextButton noteSourceButton{ "Note" };    // NOTE SOURCE (Single / Scale / Chord)
        juce::Slider stepsKnob;
        juce::Slider pulsesKnob;
        juce::Slider swingKnob;
        juce::Slider microKnob;
        juce::Slider velocityKnob;
        juce::Slider noteLenKnob;
        juce::Label rowLabel;

        // ARP Controls
        juce::ToggleButton arpActive;
        juce::TextButton arpNotesButton;
        juce::ComboBox arpMode;
        juce::ComboBox arpRateBox;

        // MIDI output
        juce::Label midiPortLabel;
        juce::ComboBox midiPortBox;
        juce::Label midiChannelLabel;
        juce::ComboBox midiChannelBox;

        // M S R == PLAY Button
        juce::TextButton muteButton{ "M" };
        juce::TextButton soloButton{ "S" };
        juce::TextButton resetButton{ "R" };
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
        std::unique_ptr<SliderAttachment> steps;
        std::unique_ptr<SliderAttachment> pulses;
        std::unique_ptr<SliderAttachment> swing;
        std::unique_ptr<SliderAttachment> micro;
        std::unique_ptr<SliderAttachment> velocity;
        std::unique_ptr<SliderAttachment> noteLen;

        std::unique_ptr<ButtonAttachment> arpActive;
        std::unique_ptr<ComboBoxAttachment> arpMode;
        std::unique_ptr<ComboBoxAttachment> arpRate;

        std::unique_ptr<ButtonAttachment> mute;
        std::unique_ptr<ButtonAttachment> solo;
        std::unique_ptr<ButtonAttachment> reset;
    };

    RhythmRowAttachments rhythmAttachments[6];

    std::unique_ptr<ComboBoxAttachment> clockSourceAttachment;
    std::unique_ptr<SliderAttachment> clockBPMAttachment;

    juce::Label headerLabels[16];
    juce::Slider clockBpmSlider;
    juce::ComboBox clockBpmBox;

    juce::Label versionLabel;

    void timerCallback() override;

    // ===== GUI SCALING =====
    static constexpr int baseWidth = 1350;
    static constexpr int baseHeight = 765;

    float guiScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Euclidean_seqAudioProcessorEditor)
};


