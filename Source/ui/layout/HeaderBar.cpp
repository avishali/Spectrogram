#include "HeaderBar.h"
#include "../../PluginProcessor.h"
#include <mdsp_dsp/spectrogram/SpectrogramSettings.h>

//==============================================================================
HeaderBar::HeaderBar (mdsp_ui::UiContext& ui, PluginProcessor& processor)
    : ui_ (ui),
      processor_ (processor)
{
    const auto& theme = ui_.theme();
    const auto& type = ui_.type();

    titleLabel.setText ("Spectrogram", juce::dontSendNotification);
    titleLabel.setFont (type.titleFont());
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setColour (juce::Label::textColourId, theme.text);
    addAndMakeVisible (titleLabel);

    modeCombo.addItemList ({ "Real-time", "Offline" }, 1);
    fftOrderCombo.addItemList ({ "256", "512", "1024", "2048", "4096", "8192", "16384" }, 1);
    for (int i = 1; i <= 8; ++i)
        overlapCombo.addItem (juce::String (i), i);
    windowTypeCombo.addItem ("Hann", 1);
    windowTypeCombo.addItem ("Blackman-Harris", 2);
    windowTypeCombo.addItem ("Flat Top", 3);
    windowTypeCombo.addItem ("Rectangular", 4);
    windowTypeCombo.addItem ("Kaiser", 5);
    windowTypeCombo.addItem (juce::String::fromUTF8 ("Cosine\xc2\xb3"), 6);
    windowTypeCombo.setSelectedId (1, juce::dontSendNotification);
    freqScaleCombo.addItem ("Linear", 1);
    freqScaleCombo.addItem ("Logarithmic", 2);
    freqScaleCombo.addItem ("Mel", 3);
    freqScaleCombo.addItem ("Bark", 4);
    freqScaleCombo.setSelectedId (2, juce::dontSendNotification);

    addAndMakeVisible (modeCombo);
    addAndMakeVisible (fftOrderCombo);
    addAndMakeVisible (overlapCombo);
    addAndMakeVisible (windowTypeCombo);
    addAndMakeVisible (freqScaleCombo);

    auto& apvts = processor_.getValueTreeState();
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, "mode", modeCombo);
    fftOrderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, mdsp::dsp::SpectrogramSettings::kParamIdFftOrder, fftOrderCombo);
    overlapAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, mdsp::dsp::SpectrogramSettings::kParamIdOverlap, overlapCombo);
    windowTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, mdsp::dsp::SpectrogramSettings::kParamIdWindowType, windowTypeCombo);
    freqScaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, mdsp::dsp::SpectrogramSettings::kParamIdFreqScale, freqScaleCombo);

    apvts.addParameterListener (mdsp::dsp::SpectrogramSettings::kParamIdFftOrder, this);
    apvts.addParameterListener (mdsp::dsp::SpectrogramSettings::kParamIdOverlap, this);
    apvts.addParameterListener (mdsp::dsp::SpectrogramSettings::kParamIdWindowType, this);
    apvts.addParameterListener (mdsp::dsp::SpectrogramSettings::kParamIdFreqScale, this);
}

HeaderBar::~HeaderBar()
{
    auto& apvts = processor_.getValueTreeState();
    apvts.removeParameterListener (mdsp::dsp::SpectrogramSettings::kParamIdFftOrder, this);
    apvts.removeParameterListener (mdsp::dsp::SpectrogramSettings::kParamIdOverlap, this);
    apvts.removeParameterListener (mdsp::dsp::SpectrogramSettings::kParamIdWindowType, this);
    apvts.removeParameterListener (mdsp::dsp::SpectrogramSettings::kParamIdFreqScale, this);
}

void HeaderBar::paint (juce::Graphics& g)
{
    const auto& theme = ui_.theme();
    g.fillAll (theme.panel);
    g.setColour (theme.borderDivider);
    g.fillRect (getLocalBounds().removeFromBottom (1));
}

void HeaderBar::resized()
{
    const auto& m = ui_.metrics();
    auto area = getLocalBounds().reduced (m.pad);
    const int comboH = m.comboH;
    const int spacing = m.gap;

    titleLabel.setBounds (area.removeFromLeft (120).withHeight (comboH));

    area.removeFromLeft (spacing);

    const int comboW = 90;
    modeCombo.setBounds (area.removeFromLeft (comboW).withHeight (comboH));
    area.removeFromLeft (spacing);

    fftOrderCombo.setBounds (area.removeFromLeft (comboW).withHeight (comboH));
    area.removeFromLeft (spacing);

    overlapCombo.setBounds (area.removeFromLeft (60).withHeight (comboH));
    area.removeFromLeft (spacing);

    windowTypeCombo.setBounds (area.removeFromLeft (130).withHeight (comboH));
    area.removeFromLeft (spacing);

    freqScaleCombo.setBounds (area.removeFromLeft (110).withHeight (comboH));
}

void HeaderBar::parameterChanged (const juce::String& parameterID, float /*newValue*/)
{
    if (parameterID == mdsp::dsp::SpectrogramSettings::kParamIdFftOrder ||
        parameterID == mdsp::dsp::SpectrogramSettings::kParamIdOverlap ||
        parameterID == mdsp::dsp::SpectrogramSettings::kParamIdWindowType ||
        parameterID == mdsp::dsp::SpectrogramSettings::kParamIdFreqScale)
    {
        if (onDspParamsChanged)
            onDspParamsChanged();
    }
}
