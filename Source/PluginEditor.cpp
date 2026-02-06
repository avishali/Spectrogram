#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      mainView_ (ui_, p)
{
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf_);
    addAndMakeVisible (mainView_);
    setSize (calculateMinimumWidth(), calculateMinimumHeight());
}

PluginEditor::~PluginEditor()
{
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui_.theme().background);
}

void PluginEditor::resized()
{
    mainView_.setBounds (getLocalBounds());
}

int PluginEditor::calculateMinimumWidth() const
{
    return 1200;
}

int PluginEditor::calculateMinimumHeight() const
{
    const int headerH = 32;
    const int footerH = 22;
    const int minSpectrogramHeight = 800;
    return headerH + footerH + minSpectrogramHeight + 2 * ui_.metrics().pad;
}
