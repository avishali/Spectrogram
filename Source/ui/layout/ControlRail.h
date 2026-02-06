#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <mdsp_ui/UiContext.h>
#include <mdsp_ui/controls/SectionHeader.h>
#include <mdsp_ui/controls/SliderRow.h>
#include <mdsp_ui/controls/ChoiceRow.h>

namespace mdsp
{
namespace gui
{
class SpectrogramComponent;
}
}

//==============================================================================
/**
    Control rail for spectrogram display controls (zoom, history, intensity).
*/
class PluginProcessor;

class ControlRail : public juce::Component
{
public:
    ControlRail (mdsp_ui::UiContext& ui, mdsp::gui::SpectrogramComponent& spectrogram, PluginProcessor& processor);
    ~ControlRail() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    mdsp_ui::UiContext& ui_;
    mdsp::gui::SpectrogramComponent& spectrogram_;
    PluginProcessor& processor_;

    mdsp_ui::SectionHeader displayHeader;
    juce::Slider zoomXSlider;
    juce::Slider zoomYSlider;
    juce::Slider historyScrollSlider;
    juce::Slider intensitySlider;
    mdsp_ui::SliderRow zoomXRow;
    mdsp_ui::SliderRow zoomYRow;
    mdsp_ui::SliderRow historyRow;
    mdsp_ui::SliderRow intensityRow;

    juce::ComboBox colorMapCombo;
    mdsp_ui::ChoiceRow colorMapRow;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> colorMapAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlRail)
};
