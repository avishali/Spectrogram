#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <mdsp_ui/UiContext.h>
#include <functional>
#include <memory>

class PluginProcessor;

//==============================================================================
/**
    Header bar with spectrogram analysis controls.
*/
class HeaderBar : public juce::Component,
                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit HeaderBar (mdsp_ui::UiContext& ui, PluginProcessor& processor);
    ~HeaderBar() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** Called when DSP parameters (FFT order, overlap, window type, freq scale) change. */
    std::function<void()> onDspParamsChanged;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    mdsp_ui::UiContext& ui_;
    PluginProcessor& processor_;

    juce::Label titleLabel;
    juce::ComboBox modeCombo;
    juce::ComboBox fftOrderCombo;
    juce::ComboBox overlapCombo;
    juce::ComboBox windowTypeCombo;
    juce::ComboBox freqScaleCombo;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> fftOrderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> overlapAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> windowTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> freqScaleAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderBar)
};
