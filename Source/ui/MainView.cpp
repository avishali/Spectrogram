#include "MainView.h"
#include "../PluginProcessor.h"

//==============================================================================
MainView::MainView (mdsp_ui::UiContext& ui, PluginProcessor& processor)
    : ui_ (ui),
      processor_ (processor),
      header_ (ui, processor),
      rail_ (ui, spectrogramComponent_, processor),
      footer_ (ui)
{
    spectrogramComponent_.setAccumulator (&processor_.getSpectrogramAccumulator());

    header_.onDspParamsChanged = [this] { onDspParamsChanged(); };

    addAndMakeVisible (header_);
    addAndMakeVisible (rail_);
    addAndMakeVisible (footer_);
    addAndMakeVisible (spectrogramComponent_);
}

MainView::~MainView() = default;

void MainView::onDspParamsChanged()
{
    spectrogramComponent_.clear();
    spectrogramComponent_.setAccumulator (&processor_.getSpectrogramAccumulator());
}

void MainView::paint (juce::Graphics& g)
{
    g.fillAll (ui_.theme().background);
}

void MainView::resized()
{
    const auto& m = ui_.metrics();
    static constexpr int headerH = 32;
    static constexpr int footerH = 22;
    static constexpr int railW = 200;

    auto bounds = getLocalBounds().reduced (m.pad);

    auto footerArea = bounds.removeFromBottom (footerH);
    footer_.setBounds (footerArea);

    auto headerArea = bounds.removeFromTop (headerH);
    header_.setBounds (headerArea);

    auto railArea = bounds.removeFromRight (railW);
    railArea.removeFromLeft (m.sectionSpacing);
    rail_.setBounds (railArea);

    spectrogramComponent_.setBounds (bounds);
}
