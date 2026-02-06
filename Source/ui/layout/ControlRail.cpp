#include "ControlRail.h"
#include "../../PluginProcessor.h"
#include <mdsp_gui/components/SpectrogramComponent.h>
#include <mdsp_gui/components/SpectrogramColors.h>

//==============================================================================
ControlRail::ControlRail (mdsp_ui::UiContext& ui, mdsp::gui::SpectrogramComponent& spectrogram, PluginProcessor& processor)
    : ui_ (ui),
      spectrogram_ (spectrogram),
      processor_ (processor),
      displayHeader (ui, "Display"),
      zoomXRow (ui, "Zoom X", zoomXSlider, 0.1, 10.0, 0.1, 1.0),
      zoomYRow (ui, "Zoom Y", zoomYSlider, 0.1, 10.0, 0.1, 1.0),
      historyRow (ui, "History", historyScrollSlider, 0, 511, 1, 0),
      intensityRow (ui, "Intensity", intensitySlider, -60.0, 60.0, 0.1, 0.0),
      colorMapRow (ui, "Color Map", colorMapCombo)
{
    displayHeader.attachToParent (*this);
    zoomXRow.attachToParent (*this);
    zoomYRow.attachToParent (*this);
    historyRow.attachToParent (*this);
    intensityRow.attachToParent (*this);
    colorMapRow.attachToParent (*this);

    intensitySlider.setTextValueSuffix (" dB");

    // Populate color map combo
    for (int i = 0; i < mdsp::gui::SpectrogramColors::numPresets; ++i)
    {
        const auto preset = static_cast<mdsp::gui::SpectrogramColors::ColorMapPreset> (i);
        colorMapCombo.addItem (mdsp::gui::SpectrogramColors::getPresetName (preset), i + 1);
    }
    colorMapCombo.setSelectedId (1, juce::dontSendNotification);

    // Attach to APVTS for state persistence
    auto& apvts = processor_.getValueTreeState();
    colorMapAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, PluginProcessor::kParamIdColorMap, colorMapCombo);

    // Wire color map changes to spectrogram component
    colorMapCombo.onChange = [this]
    {
        const int selectedIndex = colorMapCombo.getSelectedItemIndex();
        if (selectedIndex >= 0 && selectedIndex < mdsp::gui::SpectrogramColors::numPresets)
        {
            spectrogram_.setColorMapPreset (
                static_cast<mdsp::gui::SpectrogramColors::ColorMapPreset> (selectedIndex));
        }
    };

    zoomXSlider.onValueChange = [this] { spectrogram_.setZoomX (static_cast<float> (zoomXSlider.getValue())); };
    zoomYSlider.onValueChange = [this] { spectrogram_.setZoomY (static_cast<float> (zoomYSlider.getValue())); };
    historyScrollSlider.onValueChange = [this] { spectrogram_.setHistoryOffset (static_cast<int> (historyScrollSlider.getValue())); };
    intensitySlider.onValueChange = [this]
    {
        const float dbValue = static_cast<float> (intensitySlider.getValue());
        const float linearGain = std::pow (10.0f, dbValue / 20.0f);
        spectrogram_.setIntensityGain (linearGain);
    };
}

ControlRail::~ControlRail() = default;

void ControlRail::paint (juce::Graphics& g)
{
    const auto& theme = ui_.theme();
    g.fillAll (theme.panel);
    g.setColour (theme.borderDivider);
    g.fillRect (getLocalBounds().removeFromLeft (1));
}

void ControlRail::resized()
{
    const auto& m = ui_.metrics();
    auto area = getLocalBounds().reduced (m.pad);
    int y = area.getY();

    displayHeader.layout (area, y);
    y += m.gapSmall;

    zoomXRow.layout (area, y);
    zoomYRow.layout (area, y);
    historyRow.layout (area, y);
    intensityRow.layout (area, y);
    colorMapRow.layout (area, y);
}
