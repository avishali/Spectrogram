#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <mdsp_gui/components/SpectrogramComponent.h>
#include <mdsp_ui/UiContext.h>
#include "layout/HeaderBar.h"
#include "layout/ControlRail.h"
#include "layout/FooterBar.h"

class PluginProcessor;

//==============================================================================
/**
    Main UI view with AnalyzerPro-style layout.
*/
class MainView : public juce::Component
{
public:
    explicit MainView (mdsp_ui::UiContext& ui, PluginProcessor& processor);
    ~MainView() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    mdsp::gui::SpectrogramComponent& getSpectrogramComponent() { return spectrogramComponent_; }

private:
    void onDspParamsChanged();

    mdsp_ui::UiContext& ui_;
    PluginProcessor& processor_;

    mdsp::gui::SpectrogramComponent spectrogramComponent_;
    HeaderBar header_;
    ControlRail rail_;
    FooterBar footer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};
