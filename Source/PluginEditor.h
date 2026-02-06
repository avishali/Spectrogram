#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <mdsp_ui/UiContext.h>
#include <mdsp_ui/ThemeVariant.h>
#include <mdsp_ui/LookAndFeel.h>
#include "PluginProcessor.h"
#include "ui/MainView.h"

class PluginEditor : public juce::AudioProcessorEditor
{
public:
    PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PluginProcessor& audioProcessor;
    mdsp_ui::UiContext ui_ { mdsp_ui::ThemeVariant::Dark };
    mdsp_ui::LookAndFeel lnf_ { ui_ };
    MainView mainView_;

    int calculateMinimumWidth() const;
    int calculateMinimumHeight() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
